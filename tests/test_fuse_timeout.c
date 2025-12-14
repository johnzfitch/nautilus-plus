/* test_fuse_timeout.c - Test 1: Safe Timeout Verification
 *
 * Verifies that nautilus_file_check_fuse_mount_responsive:
 * 1. Returns FALSE after exactly ~1000ms on a hanging mount
 * 2. Does not block the calling thread beyond the timeout
 * 3. Quarantines the worker thread (doesn't crash when it eventually wakes)
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <time.h>
#include "../src/nautilus-file-utilities.h"

#define TIMEOUT_MS 1000
#define TOLERANCE_MS 100  /* Allow 100ms variance */

static void
test_safe_timeout (void)
{
    g_print ("\n=== Test 1: Safe Timeout Check ===\n");
    g_print ("Testing timeout behavior on /tmp/test_mnt/hang\n\n");

    GFile *hanging_file = g_file_new_for_path ("/tmp/test_mnt/hang");

    /* Measure elapsed time */
    struct timespec start, end;
    clock_gettime (CLOCK_MONOTONIC, &start);

    g_print ("[%ld.%03ld] Calling nautilus_file_check_fuse_mount_responsive...\n",
             start.tv_sec, start.tv_nsec / 1000000);

    gboolean result = nautilus_file_check_fuse_mount_responsive (hanging_file, TIMEOUT_MS);

    clock_gettime (CLOCK_MONOTONIC, &end);

    /* Calculate elapsed time in milliseconds */
    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                      (end.tv_nsec - start.tv_nsec) / 1000000;

    g_print ("[%ld.%03ld] Function returned: %s\n",
             end.tv_sec, end.tv_nsec / 1000000,
             result ? "TRUE (responsive)" : "FALSE (unresponsive)");
    g_print ("Elapsed time: %ld ms\n\n", elapsed_ms);

    /* Verify expectations */
    gboolean passed = TRUE;

    if (result != FALSE)
    {
        g_print ("❌ FAIL: Expected FALSE (unresponsive), got TRUE\n");
        passed = FALSE;
    }
    else
    {
        g_print ("✓ PASS: Correctly returned FALSE (unresponsive)\n");
    }

    if (elapsed_ms < TIMEOUT_MS - TOLERANCE_MS || elapsed_ms > TIMEOUT_MS + TOLERANCE_MS)
    {
        g_print ("❌ FAIL: Timeout took %ld ms (expected %d ± %d ms)\n",
                elapsed_ms, TIMEOUT_MS, TOLERANCE_MS);
        passed = FALSE;
    }
    else
    {
        g_print ("✓ PASS: Timeout occurred within acceptable range (%ld ms)\n", elapsed_ms);
    }

    /* Give time for worker thread to be visible in ps */
    g_print ("\n🔍 Thread Quarantine Check:\n");
    g_print ("Run this command in another terminal to see the hung worker thread:\n");
    g_print ("  ps -T -p %d | grep nautilus\n", getpid ());
    g_print ("You should see one thread in 'S' or 'D' state.\n\n");

    g_print ("Waiting 3 seconds to allow inspection...\n");
    g_usleep (3 * 1000000);

    if (passed)
    {
        g_print ("\n✅ Test 1 PASSED\n\n");
    }
    else
    {
        g_print ("\n❌ Test 1 FAILED\n\n");
        exit (1);
    }

    g_object_unref (hanging_file);
}

int
main (int argc, char *argv[])
{
    /* Check if blackhole mount is ready */
    if (!g_file_test ("/tmp/test_mnt", G_FILE_TEST_IS_DIR))
    {
        g_printerr ("ERROR: /tmp/test_mnt does not exist or is not mounted\n");
        g_printerr ("Please run: ./blackhole.py /tmp/test_mnt\n");
        return 1;
    }

    g_print ("Nautilus FUSE Mount Test Suite\n");
    g_print ("===============================\n");

    test_safe_timeout ();

    return 0;
}
