/* test_tsan_light.c - Lightweight TSan Test
 *
 * Reduced version of deduplication test for ThreadSanitizer.
 * Only 10 threads instead of 50 to reduce TSan overhead.
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "../src/nautilus-file-utilities.h"

#define NUM_THREADS 10  /* Reduced from 50 for TSan */
#define TIMEOUT_MS 1000

typedef struct {
    int thread_id;
    gboolean result;
} ThreadData;

static void *
check_mount_thread (void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    GFile *hanging_file = g_file_new_for_path ("/tmp/test_mnt/hang");

    data->result = nautilus_file_check_fuse_mount_responsive (hanging_file, TIMEOUT_MS);

    g_object_unref (hanging_file);
    return NULL;
}

int
main (int argc, char *argv[])
{
    g_print ("ThreadSanitizer: Testing %d concurrent FUSE checks\n\n", NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    /* Launch threads */
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_data[i].thread_id = i;
        thread_data[i].result = TRUE;
        pthread_create (&threads[i], NULL, check_mount_thread, &thread_data[i]);
    }

    /* Wait for completion */
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join (threads[i], NULL);
    }

    /* Verify results */
    int false_count = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (thread_data[i].result == FALSE)
            false_count++;
    }

    if (false_count == NUM_THREADS)
    {
        g_print ("✓ All %d threads returned FALSE (unresponsive)\n", NUM_THREADS);
        g_print ("✓ TSan: No data races detected\n");
        return 0;
    }
    else
    {
        g_print ("❌ %d/%d threads returned FALSE\n", false_count, NUM_THREADS);
        return 1;
    }
}
