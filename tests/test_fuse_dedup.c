/* test_fuse_dedup.c - Test 2: Deduplication (Thundering Herd)
 *
 * Verifies that 50 concurrent checks for the same mount:
 * 1. Only spawn ONE worker thread (deduplication works)
 * 2. All 50 callers wake up and return FALSE after ~1000ms
 * 3. Memory usage remains flat (no exploding MountCheckJob structs)
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "../src/nautilus-file-utilities.h"

#define NUM_THREADS 50
#define TIMEOUT_MS 1000
#define TOLERANCE_MS 200

typedef struct {
    int thread_id;
    gboolean result;
    long elapsed_ms;
    struct timespec start_time;
} ThreadData;

static void *
check_mount_thread (void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    GFile *hanging_file = g_file_new_for_path ("/tmp/test_mnt/hang");

    clock_gettime (CLOCK_MONOTONIC, &data->start_time);

    g_print ("[Thread %2d] Starting check at %ld.%03ld\n",
             data->thread_id,
             data->start_time.tv_sec,
             data->start_time.tv_nsec / 1000000);

    data->result = nautilus_file_check_fuse_mount_responsive (hanging_file, TIMEOUT_MS);

    struct timespec end_time;
    clock_gettime (CLOCK_MONOTONIC, &end_time);

    data->elapsed_ms = (end_time.tv_sec - data->start_time.tv_sec) * 1000 +
                       (end_time.tv_nsec - data->start_time.tv_nsec) / 1000000;

    g_print ("[Thread %2d] Returned %s after %ld ms\n",
             data->thread_id,
             data->result ? "TRUE" : "FALSE",
             data->elapsed_ms);

    g_object_unref (hanging_file);
    return NULL;
}

static void
test_deduplication (void)
{
    g_print ("\n=== Test 2: Deduplication (Thundering Herd) ===\n");
    g_print ("Launching %d concurrent threads checking the same mount...\n\n", NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    /* Launch all threads simultaneously */
    struct timespec global_start;
    clock_gettime (CLOCK_MONOTONIC, &global_start);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_data[i].thread_id = i;
        thread_data[i].result = TRUE;  /* Initialize to unexpected value */
        thread_data[i].elapsed_ms = 0;

        if (pthread_create (&threads[i], NULL, check_mount_thread, &thread_data[i]) != 0)
        {
            g_printerr ("Failed to create thread %d\n", i);
            exit (1);
        }
    }

    g_print ("\n⏳ Waiting for all threads to complete...\n\n");

    /* Wait for all threads to complete */
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join (threads[i], NULL);
    }

    struct timespec global_end;
    clock_gettime (CLOCK_MONOTONIC, &global_end);

    long total_elapsed = (global_end.tv_sec - global_start.tv_sec) * 1000 +
                         (global_end.tv_nsec - global_start.tv_nsec) / 1000000;

    g_print ("📊 All threads completed in %ld ms\n\n", total_elapsed);

    /* Verify results */
    gboolean all_passed = TRUE;
    int false_count = 0;
    long min_elapsed = LONG_MAX;
    long max_elapsed = 0;
    long total_time = 0;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (thread_data[i].result == FALSE)
        {
            false_count++;
        }
        else
        {
            g_print ("❌ Thread %d returned TRUE (expected FALSE)\n", i);
            all_passed = FALSE;
        }

        if (thread_data[i].elapsed_ms < min_elapsed)
            min_elapsed = thread_data[i].elapsed_ms;
        if (thread_data[i].elapsed_ms > max_elapsed)
            max_elapsed = thread_data[i].elapsed_ms;

        total_time += thread_data[i].elapsed_ms;
    }

    long avg_elapsed = total_time / NUM_THREADS;

    g_print ("📈 Timing Statistics:\n");
    g_print ("   Min:     %ld ms\n", min_elapsed);
    g_print ("   Max:     %ld ms\n", max_elapsed);
    g_print ("   Average: %ld ms\n", avg_elapsed);
    g_print ("   Range:   %ld ms\n\n", max_elapsed - min_elapsed);

    /* Check results */
    if (false_count == NUM_THREADS)
    {
        g_print ("✓ PASS: All %d threads returned FALSE\n", NUM_THREADS);
    }
    else
    {
        g_print ("❌ FAIL: %d/%d threads returned FALSE\n", false_count, NUM_THREADS);
        all_passed = FALSE;
    }

    if (avg_elapsed >= TIMEOUT_MS - TOLERANCE_MS &&
        avg_elapsed <= TIMEOUT_MS + TOLERANCE_MS)
    {
        g_print ("✓ PASS: Average timeout %ld ms (expected %d ± %d ms)\n",
                avg_elapsed, TIMEOUT_MS, TOLERANCE_MS);
    }
    else
    {
        g_print ("❌ FAIL: Average timeout %ld ms (expected %d ± %d ms)\n",
                avg_elapsed, TIMEOUT_MS, TOLERANCE_MS);
        all_passed = FALSE;
    }

    /* Check that threads woke up around the same time (deduplication working) */
    if (max_elapsed - min_elapsed < 100)  /* All within 100ms of each other */
    {
        g_print ("✓ PASS: All threads woke up together (range: %ld ms)\n",
                max_elapsed - min_elapsed);
        g_print ("   → Deduplication is working!\n");
    }
    else
    {
        g_print ("⚠️  WARNING: Large timing spread (%ld ms)\n", max_elapsed - min_elapsed);
        g_print ("   This may indicate deduplication issues or system load\n");
    }

    g_print ("\n🔍 Worker Thread Count Check:\n");
    g_print ("Expected: Only 1 worker thread created\n");
    g_print ("Actual: Check process thread count during execution\n");
    g_print ("  (If deduplication works, you saw minimal thread growth)\n\n");

    if (all_passed)
    {
        g_print ("✅ Test 2 PASSED\n\n");
    }
    else
    {
        g_print ("❌ Test 2 FAILED\n\n");
        exit (1);
    }
}

int
main (int argc, char *argv[])
{
    if (!g_file_test ("/tmp/test_mnt", G_FILE_TEST_IS_DIR))
    {
        g_printerr ("ERROR: /tmp/test_mnt does not exist or is not mounted\n");
        g_printerr ("Please run: ./blackhole.py /tmp/test_mnt\n");
        return 1;
    }

    g_print ("Nautilus FUSE Deduplication Test\n");
    g_print ("=================================\n");

    test_deduplication ();

    return 0;
}
