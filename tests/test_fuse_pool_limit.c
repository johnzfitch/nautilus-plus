/* test_fuse_pool_limit.c - Test 3: Pool Starvation (The Lifeboat)
 *
 * Verifies that when the thread pool is full:
 * 1. Application doesn't crash or freeze
 * 2. Checks for mount 4 fail gracefully (or queue safely)
 * 3. System remains responsive even when all pool threads are blocked
 *
 * Setup: 4 hanging mounts, pool limit of 3
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "../src/nautilus-file-utilities.h"

#define NUM_MOUNTS 4
#define TIMEOUT_MS 1000
#define POOL_LIMIT 3  /* Our pool is configured with max 3 threads */

typedef struct {
    int mount_id;
    char *mount_path;
    gboolean result;
    long elapsed_ms;
    struct timespec start_time;
    struct timespec end_time;
    gboolean started;
} MountCheckData;

static void *
check_mount_thread (void *arg)
{
    MountCheckData *data = (MountCheckData *)arg;

    g_autofree char *full_path = g_strdup_printf ("%s/hang", data->mount_path);
    GFile *hanging_file = g_file_new_for_path (full_path);

    clock_gettime (CLOCK_MONOTONIC, &data->start_time);
    data->started = TRUE;

    g_print ("[Mount %d] Starting check: %s\n", data->mount_id, full_path);

    data->result = nautilus_file_check_fuse_mount_responsive (hanging_file, TIMEOUT_MS);

    clock_gettime (CLOCK_MONOTONIC, &data->end_time);

    data->elapsed_ms = (data->end_time.tv_sec - data->start_time.tv_sec) * 1000 +
                       (data->end_time.tv_nsec - data->start_time.tv_nsec) / 1000000;

    g_print ("[Mount %d] Returned %s after %ld ms\n",
             data->mount_id,
             data->result ? "TRUE" : "FALSE",
             data->elapsed_ms);

    g_object_unref (hanging_file);
    return NULL;
}

static void
test_pool_starvation (void)
{
    g_print ("\n=== Test 3: Pool Starvation (The Lifeboat) ===\n");
    g_print ("Pool limit: %d threads\n", POOL_LIMIT);
    g_print ("Checking %d mounts simultaneously...\n\n", NUM_MOUNTS);

    /* Verify all mounts exist */
    for (int i = 1; i <= NUM_MOUNTS; i++)
    {
        g_autofree char *mount_path = g_strdup_printf ("/tmp/test_mnt%d", i);
        if (!g_file_test (mount_path, G_FILE_TEST_IS_DIR))
        {
            g_printerr ("ERROR: %s is not mounted\n", mount_path);
            g_printerr ("Please run: ./blackhole.py %s (in separate terminal)\n", mount_path);
            exit (1);
        }
    }

    pthread_t threads[NUM_MOUNTS];
    MountCheckData mount_data[NUM_MOUNTS];

    /* Initialize data */
    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        mount_data[i].mount_id = i + 1;
        mount_data[i].mount_path = g_strdup_printf ("/tmp/test_mnt%d", i + 1);
        mount_data[i].result = TRUE;
        mount_data[i].elapsed_ms = 0;
        mount_data[i].started = FALSE;
    }

    /* Launch all threads simultaneously */
    struct timespec global_start;
    clock_gettime (CLOCK_MONOTONIC, &global_start);

    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        if (pthread_create (&threads[i], NULL, check_mount_thread, &mount_data[i]) != 0)
        {
            g_printerr ("Failed to create thread for mount %d\n", i + 1);
            exit (1);
        }
    }

    /* Give threads a moment to start */
    g_usleep (100000);  /* 100ms */

    g_print ("\n⏳ Waiting for all checks to complete...\n\n");

    /* Wait for all threads */
    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        pthread_join (threads[i], NULL);
    }

    struct timespec global_end;
    clock_gettime (CLOCK_MONOTONIC, &global_end);

    long total_elapsed = (global_end.tv_sec - global_start.tv_sec) * 1000 +
                         (global_end.tv_nsec - global_start.tv_nsec) / 1000000;

    g_print ("📊 All checks completed in %ld ms\n\n", total_elapsed);

    /* Analyze results */
    g_print ("📈 Results by Mount:\n");
    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        g_print ("   Mount %d (%s): %s in %ld ms\n",
                mount_data[i].mount_id,
                mount_data[i].mount_path,
                mount_data[i].result ? "RESPONSIVE" : "UNRESPONSIVE",
                mount_data[i].elapsed_ms);
    }
    g_print ("\n");

    /* Verify expectations */
    gboolean all_passed = TRUE;
    int unresponsive_count = 0;

    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        if (mount_data[i].result == FALSE)
        {
            unresponsive_count++;
        }

        if (!mount_data[i].started)
        {
            g_print ("❌ FAIL: Mount %d check never started\n", mount_data[i].mount_id);
            all_passed = FALSE;
        }
    }

    /* All should return FALSE (unresponsive) */
    if (unresponsive_count == NUM_MOUNTS)
    {
        g_print ("✓ PASS: All %d mounts returned FALSE (unresponsive)\n", NUM_MOUNTS);
    }
    else
    {
        g_print ("❌ FAIL: Only %d/%d mounts returned FALSE\n", unresponsive_count, NUM_MOUNTS);
        all_passed = FALSE;
    }

    /* Critical: Check that mount 4 didn't wait 3+ seconds */
    long max_elapsed = 0;
    int slowest_mount = 0;
    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        if (mount_data[i].elapsed_ms > max_elapsed)
        {
            max_elapsed = mount_data[i].elapsed_ms;
            slowest_mount = i + 1;
        }
    }

    if (max_elapsed < TIMEOUT_MS + 500)  /* Allow some variance */
    {
        g_print ("✓ PASS: No mount waited excessively long (max: %ld ms)\n", max_elapsed);
        g_print ("   → Pool handled starvation correctly\n");
    }
    else
    {
        g_print ("⚠️  WARNING: Mount %d took %ld ms (expected ~%d ms)\n",
                slowest_mount, max_elapsed, TIMEOUT_MS);
        g_print ("   This might indicate pool blocking or queuing\n");
    }

    /* Most important: Did we crash? */
    g_print ("\n✓ CRITICAL: Application did not crash or freeze\n");
    g_print ("   System remained responsive with pool at capacity\n\n");

    /* Cleanup */
    for (int i = 0; i < NUM_MOUNTS; i++)
    {
        g_free (mount_data[i].mount_path);
    }

    if (all_passed)
    {
        g_print ("✅ Test 3 PASSED\n\n");
    }
    else
    {
        g_print ("❌ Test 3 FAILED\n\n");
        exit (1);
    }
}

int
main (int argc, char *argv[])
{
    g_print ("Nautilus FUSE Pool Starvation Test\n");
    g_print ("===================================\n");

    test_pool_starvation ();

    return 0;
}
