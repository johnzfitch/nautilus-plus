/* test_asan_uaf.c - AddressSanitizer Use-After-Free Detection Test
 *
 * This test verifies that worker threads don't access freed memory
 * when timeouts occur. ASan will catch any use-after-free bugs.
 *
 * Scenario:
 * 1. Start FUSE check with short timeout (100ms)
 * 2. Worker thread hangs on dead FUSE mount
 * 3. Main thread times out and returns
 * 4. Sleep for a moment to give worker time to potentially access freed memory
 * 5. ASan will report if worker accesses the job after it's freed
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>
#include "../src/nautilus-file-utilities.h"

#define SHORT_TIMEOUT_MS 100  /* Very short timeout to force early return */
#define GRACE_PERIOD_SEC 2    /* Give worker time to wake up and touch memory */

int
main (int argc, char *argv[])
{
    g_print ("AddressSanitizer: Use-After-Free Detection Test\n\n");

    GFile *hanging_file = g_file_new_for_path ("/tmp/test_mnt/hang");

    g_print ("Starting FUSE check with %dms timeout...\n", SHORT_TIMEOUT_MS);
    gboolean result = nautilus_file_check_fuse_mount_responsive (hanging_file, SHORT_TIMEOUT_MS);

    if (result == FALSE)
    {
        g_print ("✓ Check timed out as expected (returned FALSE)\n");
    }
    else
    {
        g_print ("❌ UNEXPECTED: Check returned TRUE\n");
        g_object_unref (hanging_file);
        return 1;
    }

    g_print ("Main thread returned. Worker may still be running in background...\n");
    g_print ("Sleeping %ds to give worker time to access memory (ASan will catch UAF)...\n",
             GRACE_PERIOD_SEC);

    /* CRITICAL: If the worker thread accesses `job` after we've freed it,
     * AddressSanitizer will detect and report the use-after-free */
    sleep (GRACE_PERIOD_SEC);

    g_print ("✓ Grace period complete. If ASan didn't report errors, memory is safe!\n");

    g_object_unref (hanging_file);
    return 0;
}
