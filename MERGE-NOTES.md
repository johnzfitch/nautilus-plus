# Upstream Merge Notes - 605ea49d

## Summary

Successfully merged upstream GNOME Nautilus commit 605ea49d (2025-12-14) into nautilus-plus.

## Custom Features Status

### ✅ Fully Restored

1. **Search-cache Integration**
   - Added `nautilus-search-engine-searchcache.c/h` to meson.build
   - Integrated searchcache provider in `nautilus-search-engine.c/h`
   - Added `NAUTILUS_SEARCH_TYPE_SEARCHCACHE` enum
   - Restored search settings in gschema.xml:
     - `search-results-limit` (default: 1000)
     - `search-show-hidden-files` (default: true)

2. **FUSE Mount Detection**
   - Restored `nautilus_file_check_fuse_mount_responsive()` in file-utilities.c
   - Restored `nautilus_file_is_on_fuse_mount()` in file-utilities.c
   - Added function declarations to file-utilities.h
   - 1-second timeout prevents UI hangs on stale remote mounts

3. **Custom Preferences**
   - Restored `NAUTILUS_PREFERENCES_ANIMATED_THUMBNAILS` setting
   - Restored `NAUTILUS_PREFERENCES_SEARCH_RESULTS_LIMIT` setting
   - Restored `NAUTILUS_PREFERENCES_SEARCH_SHOW_HIDDEN_FILES` setting

### ⚠️ Partially Restored - Needs Integration

**Animated WebP/GIF/APNG Thumbnails**

Status: Source files exist but not fully integrated due to upstream architectural changes.

Files present:
- `src/nautilus-animated-thumbnail.c/h`
- `src/nautilus-animated-paintable.c/h`
- Settings in gschema.xml (`AnimatedThumbnailMode` enum)

**Issue:** Upstream refactored the thumbnail system significantly:
- Introduced new `NautilusImage` widget (commit c5a69c6f4)
- Changed thumbnail loading to async (commit c5a69c6f4)
- Refactored grid-cell and name-cell to use NautilusImage

**Next Steps:**
1. Integrate animated thumbnail support into the new `NautilusImage` widget
2. Update grid-cell.c to use animated thumbnails via NautilusImage
3. Update name-cell.c similarly
4. Test with WebP/GIF/APNG files

**Workaround:** The files compile but animations won't play until integrated with NautilusImage.

## Upstream Changes Applied

- 165 commits from upstream
- Major refactorings:
  - Thumbnail system (NautilusImage widget)
  - Search engine provider architecture
  - Application window management
  - File operations and batch rename
- Translation updates for 8 languages

## Files Modified

### Added to meson.build
- nautilus-search-engine-searchcache.c/h
- nautilus-animated-thumbnail.c/h
- nautilus-animated-paintable.c/h

### Modified
- src/nautilus-search-engine.c/h (searchcache integration)
- src/nautilus-global-preferences.h (custom settings)
- src/nautilus-file-utilities.c/h (FUSE detection)
- data/org.gnome.nautilus.gschema.xml (custom settings)

## Testing Needed

1. **Search Functionality**
   - Verify search-cache daemon integration works
   - Test search results limit enforcement
   - Test hidden file filtering in search

2. **FUSE Detection**
   - Test with SSHFS/FUSE mounts
   - Verify 1-second timeout prevents hangs
   - Test with stale mounts

3. **Animated Thumbnails** (after integration)
   - Test with WebP/GIF/APNG files
   - Verify animation modes (never/hover/select/always)
   - Performance testing with large directories

## Known Issues

None - all custom features compile cleanly. Animated thumbnails need integration work.

## Build Status

- Merge completed successfully
- No conflicts in source files
- All custom files included in build system
