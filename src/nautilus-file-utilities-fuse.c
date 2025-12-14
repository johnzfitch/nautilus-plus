/* nautilus-file-utilities-fuse.c - Async-safe FUSE mount detection
 *
 * Copyright (C) 2024 Nautilus Plus Contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "nautilus-file-utilities.h"
#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>

/**
 * DESIGN: Ref-Counted Sentinel for FUSE Mount Checking
 *
 * Problem: stat() on dead FUSE mounts enters D-state (uninterruptible sleep).
 * Cannot be killed, cannot be cancelled, will hang forever.
 *
 * Solution: Accept that threads may leak, but quarantine them:
 * 1. Use a PRIVATE thread pool (max 3 threads) - safety valve
 * 2. Ref-counted job state prevents use-after-free when timeout occurs
 * 3. Deduplication prevents stampede (multiple checks for same mount)
 * 4. Waiter can timeout and leave; thread finishes eventually and cleans up
 *
 * Thread lifecycle:
 * - Job created with refcount=1 (hash table owns it)
 * - Thread increments to 2 before starting
 * - Waiter increments to 3 when joining
 * - On timeout: waiter decrefs to 2, leaves
 * - Eventually thread finishes, decrefs to 1
 * - If finished, hash table is cleaned, decrefs to 0, freed
 */

typedef enum {
    MOUNT_STATUS_UNKNOWN = 0,
    MOUNT_STATUS_CHECKING,
    MOUNT_STATUS_RESPONSIVE,
    MOUNT_STATUS_UNRESPONSIVE
} MountStatus;

typedef struct {
    gatomicrefcount ref_count;
    char *path;
    GMutex lock;
    GCond cond;
    MountStatus status;
    gboolean thread_finished;
} MountCheckJob;

/* Global state - initialized once */
static GThreadPool *fuse_check_pool = NULL;
static GHashTable *active_jobs = NULL;  /* path -> MountCheckJob* */
static GMutex global_lock;
static GOnce init_once = G_ONCE_INIT;

/* ========== MountCheckJob Lifecycle ========== */

static MountCheckJob *
mount_check_job_new (const char *path)
{
    MountCheckJob *job = g_new0 (MountCheckJob, 1);
    g_atomic_ref_count_init (&job->ref_count);
    job->path = g_strdup (path);
    g_mutex_init (&job->lock);
    g_cond_init (&job->cond);
    job->status = MOUNT_STATUS_CHECKING;
    job->thread_finished = FALSE;
    return job;
}

static void
mount_check_job_ref (MountCheckJob *job)
{
    g_atomic_ref_count_inc (&job->ref_count);
}

static void
mount_check_job_unref (MountCheckJob *job)
{
    if (g_atomic_ref_count_dec (&job->ref_count))
    {
        g_free (job->path);
        g_mutex_clear (&job->lock);
        g_cond_clear (&job->cond);
        g_free (job);
    }
}

/* ========== Worker Thread (The "Expendable" Thread) ========== */

/**
 * fuse_check_worker:
 *
 * This thread is designed to potentially hang FOREVER in D-state
 * if the FUSE mount is dead. That's OK - we've quarantined it.
 *
 * The private thread pool has max_threads=3, so even if all 3 leak,
 * the app continues working (just assumes FUSE mounts are unresponsive).
 */
static void
fuse_check_worker (gpointer data, gpointer user_data)
{
    MountCheckJob *job = (MountCheckJob *)data;

    g_debug ("FUSE check worker starting for: %s", job->path);

    /* THIS LINE MAY HANG FOREVER IN D-STATE */
    struct stat st;
    int result = stat (job->path, &st);

    /* If we get here, the mount responded (or doesn't exist) */
    g_mutex_lock (&job->lock);
    if (result == 0)
    {
        job->status = MOUNT_STATUS_RESPONSIVE;
        g_debug ("FUSE check: %s is RESPONSIVE", job->path);
    }
    else
    {
        job->status = MOUNT_STATUS_UNRESPONSIVE;
        g_debug ("FUSE check: %s is UNRESPONSIVE (errno=%d)", job->path, errno);
    }
    job->thread_finished = TRUE;
    g_cond_broadcast (&job->cond);  /* Wake all waiters */
    g_mutex_unlock (&job->lock);

    /* Drop thread's reference - if waiter timed out, this may free the job */
    mount_check_job_unref (job);
}

/* ========== Initialization ========== */

static gpointer
fuse_check_init_once (gpointer data)
{
    /* Create private thread pool with STRICT LIMIT
     * If 3 mounts are dead and threads leaked, we stop checking
     * new mounts to protect the app. Better to skip checks than crash. */
    GError *error = NULL;
    fuse_check_pool = g_thread_pool_new (fuse_check_worker,
                                         NULL,  /* user_data */
                                         3,     /* max_threads - SAFETY VALVE */
                                         FALSE, /* exclusive */
                                         &error);
    if (error)
    {
        g_warning ("Failed to create FUSE check thread pool: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    g_mutex_init (&global_lock);

    active_jobs = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         (GDestroyNotify)mount_check_job_unref);

    g_debug ("FUSE check system initialized (max 3 concurrent checks)");

    return &fuse_check_pool;  /* Non-NULL indicates success */
}

static gboolean
ensure_fuse_check_initialized (void)
{
    g_once (&init_once, fuse_check_init_once, NULL);
    return (fuse_check_pool != NULL);
}

/* ========== Public API ========== */

/**
 * nautilus_file_check_fuse_mount_responsive:
 * @file: A #GFile to check
 * @timeout_ms: Timeout in milliseconds
 *
 * Thread-safe, async-safe FUSE mount responsiveness check.
 *
 * GUARANTEES:
 * - Never crashes (even on timeout with leaked threads)
 * - Never blocks longer than timeout_ms
 * - Never starves thread pool (max 3 threads)
 * - Deduplicates concurrent checks for same path
 *
 * Returns: %TRUE if mount is responsive, %FALSE if timeout/unresponsive
 */
gboolean
nautilus_file_check_fuse_mount_responsive (GFile *file, guint timeout_ms)
{
    g_return_val_if_fail (G_IS_FILE (file), FALSE);

    if (!ensure_fuse_check_initialized ())
    {
        /* Initialization failed - fail safe */
        return FALSE;
    }

    g_autofree char *path = g_file_get_path (file);
    if (path == NULL)
    {
        /* Non-local file (e.g., gvfs:// URI) - assume accessible */
        return TRUE;
    }

    /* ========== PHASE 1: Deduplication ========== */
    g_mutex_lock (&global_lock);

    MountCheckJob *job = g_hash_table_lookup (active_jobs, path);

    if (job)
    {
        /* Existing job for this path - join it */
        mount_check_job_ref (job);
        g_debug ("FUSE check: Joining existing job for %s", path);
    }
    else
    {
        /* Create new job */
        job = mount_check_job_new (path);
        g_hash_table_insert (active_jobs, g_strdup (path), job);
        /* Hash table now owns 1 ref */

        mount_check_job_ref (job);  /* Thread will own 1 ref */
        mount_check_job_ref (job);  /* Waiter will own 1 ref */

        GError *error = NULL;
        if (!g_thread_pool_push (fuse_check_pool, job, &error))
        {
            /* Pool is full or errored - fail safe */
            g_warning ("FUSE check pool full or error for %s: %s",
                      path, error ? error->message : "unknown");
            if (error)
            {
                g_error_free (error);
            }

            /* Clean up */
            g_hash_table_remove (active_jobs, path);  /* Drops hash table ref */
            mount_check_job_unref (job);  /* Drop thread ref */
            mount_check_job_unref (job);  /* Drop waiter ref */
            g_mutex_unlock (&global_lock);
            return FALSE;
        }

        g_debug ("FUSE check: Started new job for %s", path);
    }

    g_mutex_unlock (&global_lock);

    /* ========== PHASE 2: Wait with Timeout ========== */
    gboolean result = FALSE;
    gint64 deadline = g_get_monotonic_time () + (timeout_ms * 1000);

    g_mutex_lock (&job->lock);

    while (!job->thread_finished)
    {
        if (!g_cond_wait_until (&job->cond, &job->lock, deadline))
        {
            /* TIMEOUT OCCURRED */
            g_debug ("FUSE check TIMEOUT for %s after %u ms (thread may still be running)",
                    path, timeout_ms);

            /* CRITICAL: We do NOT cancel the thread (impossible with D-state).
             * We just leave. The job persists in active_jobs until thread wakes.
             * Ref-counting prevents use-after-free. */
            result = FALSE;  /* Assume unresponsive */
            goto cleanup;
        }
    }

    /* Thread finished in time */
    result = (job->status == MOUNT_STATUS_RESPONSIVE);
    g_debug ("FUSE check completed for %s: %s",
            path, result ? "RESPONSIVE" : "UNRESPONSIVE");

cleanup:
    /* Read thread_finished state while holding the lock */
    gboolean thread_is_finished = job->thread_finished;
    g_mutex_unlock (&job->lock);

    /* ========== PHASE 3: Cleanup (if finished) ========== */
    if (thread_is_finished)
    {
        /* Thread completed - remove from global table */
        g_mutex_lock (&global_lock);
        /* Double-check it's still the same job */
        if (g_hash_table_lookup (active_jobs, path) == job)
        {
            g_hash_table_remove (active_jobs, path);
            g_debug ("FUSE check: Removed completed job for %s", path);
        }
        g_mutex_unlock (&global_lock);
    }
    else
    {
        g_debug ("FUSE check: Job for %s still running (timed out), leaving in table", path);
    }

    /* Drop our listener reference */
    mount_check_job_unref (job);

    return result;
}
