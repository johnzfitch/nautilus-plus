# Upstream Sync Specification

Generated: 2026-01-01 01:17:06 UTC
Upstream Commit: 12c90eebcb1439ca518e9a5cac246a5561fc8720
Current Commit: e01f6a82c4c28f67e2a63f784541e65983a700a9

## Upstream Changes

* 12c90eebc Drop deprecated Nautilus.FileOperations interface
* 6f0b19a05 general: Correct AdwDialog parameter type
* 0a832d326 operations-ui: Focus password entry on extraction
* 464c560fc operations-ui: Use AdwAlertDialog for extracting
* 65ad5b7bc Update Japanese translation
* 9b51509cc window-slot: Use autocleanups over goto
* f5914d6ec window-slot: Use g_autoptr for FindMountData
* dd2898c08 window-slot: Simplify display_view_selection_failure()
* 7dd09c321 directory-async: Use autocleanups over goto
* 6b3a2f4ff search-directory: Linearize callback check over goto
* 4fea54068 files-view: Use g_autoptr for custom types over goto
* c051a583d file: Linearize file icon checking
* 4244dc38f file: Factor out common GDrive checking code
* a2e05538b file: Use direct returns over goto
* eb960adc5 file: Use autoptr for GDrives
* 8539c5bd2 autorun-software: Use direct return over goto
* 5582b8688 autorun-software: Use else if over goto
* fa015c7ca file-operations: Use autocleanups over goto
* 325eb2959 file-operations: Indicate file skipping with return value
* 87a9d54cd file-operations: Use autocleanup for GErrors
* e8e00982d file-operations: Remove retry gotos
* 35dfbd498 file-operations: Prepare retry goto removal
* d24ccd35c image-prop: Drop GdkPixbuf usage
* 2ffaf3552 image-prop: Inline some variable declarations
* 8785b5b02 image-prop: Clarify heigh/width with orientation
* 627f9eec0 files-view: Simplify template menu filtering
* 5b6447d0f file: Simplify list filtering
* ea3b09ac6 files-view: Inline single-use hidden file filter
* a3300cc1a files-view: Separate extract/activate via helper
* 515001067 files-view: Invert adding/debuting file list logic
* 55792e825 directory: Invert tentative file list logic
* ac46e5cae file-operations: Autocleanup move destination
* cf68d134b filename-validator: Strip all UTF-8 whitespaces
* 14ab35955 filename-validator: Fix name leak
* 0ca2c3a1a test/thumbnails: Fix cancellable leak when skipping test
* 49545a2be metainfo: Use supports for controls
* e22349bc7 file-operations: avoid redundant folder name in completion toast

## Files Changed

 .github/assets/icons/document.png                  |  Bin 948 -> 0 bytes
 .github/assets/icons/filter.png                    |  Bin 1373 -> 0 bytes
 .github/assets/icons/folder.png                    |  Bin 1279 -> 0 bytes
 .github/assets/icons/medium-speed.png              |  Bin 1860 -> 0 bytes
 .github/assets/icons/search.png                    |  Bin 1646 -> 0 bytes
 .github/assets/icons/tick.png                      |  Bin 922 -> 0 bytes
 .github/assets/icons/warning.png                   |  Bin 789 -> 0 bytes
 .github/workflows/aur-publish.yml                  |  245 --
 .github/workflows/build-and-release.yml            |  238 --
 .github/workflows/upstream-sync-with-ai-agents.yml |  207 --
 .github/workflows/upstream-sync.yml                |  215 --
 .gitignore                                         |   56 -
 CRASH-ANALYSIS.md                                  |  188 --
 INSTALLATION.md                                    |   29 -
 PERFORMANCE-MONITORING.md                          |  235 --
 README.md                                          |  173 +-
 analyze-performance.sh                             |  149 --
 data/dbus-interfaces.xml                           |   59 -
 data/org.gnome.Nautilus.metainfo.xml.in.in         |    7 +-
 data/org.gnome.nautilus.gschema.xml                |   24 +-
 docs/installation.md                               |  346 ---
 docs/search-blacklist.md                           |  220 --
 .../nautilus-image-properties-model.c              |  299 +--
 monitor-nautilus.sh                                |  250 --
 nautilus-plus.install                              |   24 -
 nautilus-watchdog.sh                               |   96 -
 performance-logs/alerts.log                        |    1 -
 performance-logs/metrics_20251214.csv              |   78 -
 performance-logs/watchdog.out                      |  314 ---
 po/ja.po                                           | 1206 +++++-----
 src/meson.build                                    |   15 -
 src/nautilus-animated-paintable.c                  |  216 --
 src/nautilus-animated-paintable.h                  |   29 -
 src/nautilus-animated-thumbnail.c                  |  299 ---
 src/nautilus-animated-thumbnail.h                  |   68 -
 src/nautilus-autorun-software.c                    |   21 +-
 src/nautilus-dbus-manager.c                        |  164 +-
 src/nautilus-directory-async.c                     |   18 +-
 src/nautilus-directory.c                           |   23 +-
 src/nautilus-enums.h                               |    4 -
 src/nautilus-file-operations.c                     | 2536 ++++++++++----------
 src/nautilus-file-utilities-fuse.c                 |  312 ---
 src/nautilus-file-utilities.c                      |  117 -
 src/nautilus-file-utilities.h                      |   23 -
 src/nautilus-file.c                                |  276 +--
 src/nautilus-file.h                                |    6 +-
 src/nautilus-filename-utilities.c                  |   50 +
 src/nautilus-filename-utilities.h                  |    3 +
 src/nautilus-filename-validator.c                  |    5 +-
 src/nautilus-files-view.c                          |  229 +-
 src/nautilus-global-preferences.h                  |    7 -
 src/nautilus-grid-cell.c                           |   45 -
 src/nautilus-grid-view.c                           |   14 +-
 src/nautilus-list-view.c                           |    3 -
 src/nautilus-mime-actions.c                        |    6 +-
 src/nautilus-name-cell.c                           |  115 +-
 src/nautilus-name-cell.h                           |    1 -
 src/nautilus-operations-ui-manager.c               |   16 +-
 src/nautilus-preferences-dialog.c                  |  292 ---
 src/nautilus-query-editor.c                        |   58 +-
 src/nautilus-query.c                               |   20 -
 src/nautilus-query.h                               |    4 -
 src/nautilus-search-directory.c                    |   23 +-
 src/nautilus-search-engine-localsearch.c           |   44 +-
 src/nautilus-search-engine-searchcache.c           |  345 ---
 src/nautilus-search-engine-searchcache.h           |   24 -
 src/nautilus-search-engine-simple.c                |   59 +-
 src/nautilus-search-engine.c                       |   12 -
 src/nautilus-search-engine.h                       |    5 +-
 src/nautilus-search-hit.c                          |   21 +-
 src/nautilus-sidebar.c                             |  175 +-
 src/nautilus-sidebar.h                             |    3 -
 src/nautilus-thumbnails.c                          |    2 -
 src/nautilus-thumbnails.h                          |    1 +
 src/nautilus-view-item.c                           |    8 -
 src/nautilus-view-model.c                          |    1 -
 src/nautilus-window-slot.c                         |  111 +-
 src/nautilus-window.c                              |   79 +-
 src/nautilus-window.h                              |    4 -
 src/resources/style.css                            |   32 -
 src/resources/ui/nautilus-name-cell.ui             |   82 +-
 ...lus-operations-ui-manager-request-passphrase.ui |    2 +-
 src/resources/ui/nautilus-preferences-dialog.ui    |   54 -
 src/resources/ui/nautilus-sidebar-row.ui           |    3 -
 src/resources/ui/nautilus-window.ui                |   15 +-
 status.sh                                          |   73 -
 test/automated/displayless/test-thumbnails.c       |    3 +-
 87 files changed, 2266 insertions(+), 8569 deletions(-)

## AI Agent Tasks

1. @copilot: Analyze breaking changes and compatibility
2. @claude: Implement merge and resolve conflicts
3. @codex: Code review and QA
