# Nautilus Crash Analysis - 2025-12-14

## Crash Summary

**Date:** 2025-12-14 13:23
**Crashes:** 3 consecutive crashes within 30 seconds
**Signal:** SIGSEGV (Segmentation Fault)
**Location:** Search engine (nautilus-search-engine-simple.c)

### Crash Timeline

1. **13:23:08** - Process 54449 crashed
2. **13:23:15** - Process 66948 crashed (auto-restarted)
3. **13:23:35** - Process 67201 crashed (auto-restarted)

## Root Cause Analysis

### Primary Issue: Race Condition in FUSE Mount Check

**File:** `src/nautilus-file-utilities.c`
**Function:** `nautilus_file_check_fuse_mount_responsive()`

**Problem 1: Thread Cleanup on Timeout**
```c
Line 1164-1173:
if (!g_cond_wait_until (&data.cond, &data.mutex, end_time))
{
    /* Timeout - mount is unresponsive */
    g_mutex_unlock (&data.mutex);
    g_debug ("Mount check timeout for %s after %u ms", path, timeout_ms);
    /* Thread will eventually complete and clean up */
    g_thread_unref (thread);  // ❌ WRONG: Thread still running!
    g_mutex_clear (&data.mutex);  // ❌ Thread may still be using this!
    g_cond_clear (&data.cond);    // ❌ Thread may still be using this!
    return FALSE;
}
```

**Issue:** When timeout occurs, the function:
1. Unrefs the thread without joining it
2. Clears mutex and condition variables while thread is still running
3. Returns, allowing stack-allocated `MountCheckData` to go out of scope
4. Background thread accesses freed memory → SEGFAULT

**Problem 2: Unsynchronized Cache Access**
```c
Line 1188-1233:
static GList *cached_fuse_mounts = NULL;  // ❌ Not thread-safe!

static void
refresh_fuse_mount_cache (void)
{
    // Called from multiple search threads simultaneously
    g_list_free_full (cached_fuse_mounts, g_free);  // ❌ Race condition!
    cached_fuse_mounts = NULL;
    // ... rebuild list ...
}
```

**Issue:** Multiple search threads can call `nautilus_file_is_on_fuse_mount()` concurrently, all calling `refresh_fuse_mount_cache()`, causing:
- Concurrent modifications to `cached_fuse_mounts`
- Use-after-free when one thread frees while another reads
- Memory corruption

### Secondary Issue: GObject Reference Counting

**Crash location:** `visit_directory()` trying to unref an already-freed object

This is a consequence of the memory corruption from the race conditions above.

## Stack Traces

### Crash #1 (Search Thread)
```
#0  g_type_check_instance_is_fundamentally_a
#1  g_object_unref
#4  visit_directory (/usr/local/bin/nautilus + 0xa93c6)
#5  search_thread_func
```

### Crash #2 (Main Thread)
```
#6  nautilus_search_hit_compute_scores
#7  search_engine_hits_added
#19 nautilus_search_provider_hits_added
#20 search_thread_process_hits_idle
```

## Impact

- **Severity:** HIGH - Crashes during any search operation
- **Trigger:** Opening directories or searching while FUSE mounts exist
- **Frequency:** Very high - crashes within seconds of searching
- **Data Loss:** None (crashes before completing operations)

## Recommended Fixes

### Fix 1: Proper Thread Cleanup on Timeout

```c
gboolean
nautilus_file_check_fuse_mount_responsive (GFile *file, guint timeout_ms)
{
    // ... existing code ...

    while (!data.done)
    {
        if (!g_cond_wait_until (&data.cond, &data.mutex, end_time))
        {
            /* Timeout - mark as timed out and wait for thread */
            data.timed_out = TRUE;
            g_mutex_unlock (&data.mutex);

            /* Must join thread to avoid use-after-free */
            g_thread_join (thread);

            g_mutex_clear (&data.mutex);
            g_cond_clear (&data.cond);
            return FALSE;
        }
    }
    // ... rest unchanged ...
}
```

Also update `MountCheckData` struct to include `gboolean timed_out` field, and have the thread check this flag.

### Fix 2: Thread-Safe Cache

```c
static GMutex fuse_cache_mutex;
static GList *cached_fuse_mounts = NULL;
static gint64 fuse_mount_cache_time = 0;

static void
refresh_fuse_mount_cache (void)
{
    g_mutex_lock (&fuse_cache_mutex);

    gint64 now = g_get_monotonic_time () / 1000;
    if (cached_fuse_mounts != NULL &&
        (now - fuse_mount_cache_time) < FUSE_MOUNT_CACHE_TTL_MS)
    {
        g_mutex_unlock (&fuse_cache_mutex);
        return;
    }

    // ... rest of function ...

    g_mutex_unlock (&fuse_cache_mutex);
}
```

### Fix 3: Alternative - Disable FUSE Check Temporarily

Quick workaround for immediate stability:

```bash
cd ~/dev/nautilus-plus/src
# Comment out the FUSE mount check in visit_directory
# This will restore stability at the cost of potential hangs on stale mounts
```

## Monitoring Data

Watchdog captured the crash:
- CPU spiked from 3.9% to 24.2%
- Memory dropped from 278MB to 255MB (process restart)
- Thread count changed from 15 → 22 → 23 → 26

## Next Steps

1. **Immediate:** Apply fixes to FUSE mount checking code
2. **Test:** Run with valgrind to verify race conditions are fixed
3. **Verify:** Ensure searches complete without crashes
4. **Monitor:** Continue watchdog monitoring for any new issues

## Files to Modify

- `src/nautilus-file-utilities.c` - Fix thread cleanup and add mutex
- `src/nautilus-file-utilities.h` - Update MountCheckData struct if needed
- `src/nautilus-search-engine-simple.c` - Potentially disable FUSE check temporarily

## Related Issues

- This is why the search results limit was added - crashes during large searches
- FUSE mount detection was added to prevent hangs, but introduced new crashes
- The race condition is worse under load (multiple search threads)
