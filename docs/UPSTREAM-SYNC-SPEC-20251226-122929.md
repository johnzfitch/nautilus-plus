# Upstream Sync Specification

Generated: 2025-12-26 12:29:29 UTC
Upstream Commit: 1e786d16b75c91ff8c0e3804d25178c92b39e816
Current Commit: 0971d682151f2273da9101a5a23f68f067f58f69

## Upstream Changes

* 1e786d16b directory: Use g_set_object for location setting
* 9803f60b5 directory: Fix iterating moved directories
* 4f67e4d6c Fix leaks of nautilus_file_get_by_uri
* 850f08a9b mime-actions: Fix closing AdwDialog
* 85620479b mime-actions: Simplify unhandled URI logic
* dffb91b2a general: Always try packagekit option
* 2242f24ee Revert "gschema: Revert accidental new translatable strings"
* b919a8867 mime-actions: Use generic-icon-name for video test
* 0a8ca475c video-mime-types: Drop non-video types
* 0fe5b558e mime-actions: Unify video type testing
* 330e3dd9d av-properties: Drop unknown mime types
* 116ed4e66 av-properties: Drop unused mime type lists
* e01c5f896 file-operations: Pass const strings to run_dialog
* 5d43b81bb file-operations: Adjust responese dialog data struct
* 4548264db file-operations: Use AdwAlertDialog
* c0b4cb115 file-operations: Use simple parent window pointer
* 9efb450b6 file-operations: Replace deprecated timestamp window call
* bbf2c0ea9 file-operations: Unify skippable dialogs
* 47b08dc81 file-operations: Unify dialog creation methods
* 6dd8a7469 file-operations: Use ui-util dialog for confirming
* 6e9365065 file-operations: Drop superfluous string
* 2f7e6c82f ui-util: Allow calling show_ok_dialog from any thread
* e50d507ab ui-util: Use GtkWidget as parent in show_ok_dialog
* c7fca4701 application: Simplify home directory check
* f2e68e70b ui-util: Port dialog to AdwAlertDialog
* f167c8114 ui-util: Simplify show_dialog signature
* 767e330ae ui-util: Show dialog on active window as fallback
* 3823daadd file-operations: Skip only one file for respective response
* c063516c3 file-operations: Drop incorrect comment
* ca11dd9e8 file-operations: Make response order consistent
* 70fde086d file-operations: Remove dead code
* 1b6f695c6 file-operations: Remove unused *_ALL defines
* 081ec6195 properties-window: Adjust open symlink parent tooltip
* e58ddbb4d style: Use media queries for high contrast

## Files Changed

 .github/assets/icons/document.png                  |  Bin 948 -> 0 bytes
 .github/assets/icons/filter.png                    |  Bin 1373 -> 0 bytes
 .github/assets/icons/folder.png                    |  Bin 1279 -> 0 bytes
 .github/assets/icons/medium-speed.png              |  Bin 1860 -> 0 bytes
 .github/assets/icons/search.png                    |  Bin 1646 -> 0 bytes
 .github/assets/icons/tick.png                      |  Bin 922 -> 0 bytes
 .github/assets/icons/warning.png                   |  Bin 789 -> 0 bytes
 .github/workflows/aur-publish.yml                  |  245 ----
 .github/workflows/build-and-release.yml            |  238 ----
 .github/workflows/upstream-sync-with-ai-agents.yml |  207 ----
 .github/workflows/upstream-sync.yml                |  215 ----
 .gitignore                                         |   56 -
 CRASH-ANALYSIS.md                                  |  188 ---
 INSTALLATION.md                                    |   29 -
 PERFORMANCE-MONITORING.md                          |  235 ----
 README.md                                          |  173 +--
 analyze-performance.sh                             |  149 ---
 data/org.gnome.nautilus.gschema.xml                |   33 +-
 docs/installation.md                               |  346 ------
 docs/search-blacklist.md                           |  220 ----
 .../audio-video-properties/totem-mime-types.h      |  161 ---
 meson.build                                        |    1 -
 meson_options.txt                                  |    3 +-
 monitor-nautilus.sh                                |  250 ----
 nautilus-plus.install                              |   24 -
 nautilus-watchdog.sh                               |   96 --
 performance-logs/alerts.log                        |    1 -
 performance-logs/metrics_20251214.csv              |   78 --
 performance-logs/watchdog.out                      |  314 -----
 po/POTFILES.in                                     |    1 +
 src/meson.build                                    |   10 +-
 src/nautilus-animated-paintable.c                  |  216 ----
 src/nautilus-animated-paintable.h                  |   29 -
 src/nautilus-animated-thumbnail.c                  |  299 -----
 src/nautilus-animated-thumbnail.h                  |   68 --
 src/nautilus-application.c                         |   67 +-
 src/nautilus-batch-rename-utilities.c              |    3 +-
 src/nautilus-dbus-launcher.c                       |   14 +-
 src/nautilus-dbus-manager.c                        |    4 +-
 src/nautilus-dialog-utilities.c                    |  187 +++
 src/nautilus-dialog-utilities.h                    |   46 +
 src/nautilus-directory.c                           |   35 +-
 src/nautilus-enums.h                               |    4 -
 src/nautilus-error-reporting.c                     |   26 +-
 src/nautilus-file-operations-dbus-data.c           |    9 -
 src/nautilus-file-operations-dbus-data.h           |    2 -
 src/nautilus-file-operations.c                     | 1242 ++++++--------------
 src/nautilus-file-utilities-fuse.c                 |  312 -----
 src/nautilus-file-utilities.c                      |  124 +-
 src/nautilus-file-utilities.h                      |   23 -
 src/nautilus-file.c                                |   47 +-
 src/nautilus-files-view-dnd.c                      |   10 +-
 src/nautilus-files-view.c                          |   72 +-
 src/nautilus-global-preferences.h                  |   10 -
 src/nautilus-grid-cell.c                           |   45 -
 src/nautilus-grid-view.c                           |   14 +-
 src/nautilus-image.c                               |   44 +-
 src/nautilus-list-view.c                           |    3 -
 src/nautilus-mime-actions.c                        |  125 +-
 src/nautilus-mime-actions.h                        |    1 +
 src/nautilus-name-cell.c                           |  115 +-
 src/nautilus-name-cell.h                           |    1 -
 src/nautilus-preferences-dialog.c                  |  292 -----
 src/nautilus-program-choosing.c                    |   41 +-
 src/nautilus-program-choosing.h                    |    2 +-
 src/nautilus-properties-window.c                   |   21 +-
 src/nautilus-query-editor.c                        |   58 +-
 src/nautilus-query.c                               |   20 -
 src/nautilus-query.h                               |    4 -
 src/nautilus-search-directory.c                    |    8 +-
 src/nautilus-search-engine-localsearch.c           |   44 +-
 src/nautilus-search-engine-searchcache.c           |  345 ------
 src/nautilus-search-engine-searchcache.h           |   24 -
 src/nautilus-search-engine-simple.c                |   59 +-
 src/nautilus-search-engine.c                       |   12 -
 src/nautilus-search-engine.h                       |    5 +-
 src/nautilus-search-hit.c                          |   21 +-
 src/nautilus-sidebar.c                             |  182 +--
 src/nautilus-sidebar.h                             |    3 -
 src/nautilus-thumbnails.c                          |    2 -
 src/nautilus-thumbnails.h                          |    1 +
 src/nautilus-ui-utilities.c                        |   43 +-
 src/nautilus-ui-utilities.h                        |    8 +-
 src/nautilus-video-mime-types.h                    |   73 --
 src/nautilus-view-item.c                           |    8 -
 src/nautilus-view-model.c                          |    1 -
 src/nautilus-window-slot.c                         |    2 +-
 src/nautilus-window.c                              |   70 --
 src/nautilus-window.h                              |    4 -
 src/resources/nautilus.gresource.xml.in            |    1 -
 src/resources/style-hc.css                         |   23 -
 src/resources/style.css                            |   61 +-
 src/resources/ui/nautilus-name-cell.ui             |   82 +-
 src/resources/ui/nautilus-preferences-dialog.ui    |   54 -
 src/resources/ui/nautilus-properties-window.ui     |    2 +-
 src/resources/ui/nautilus-sidebar-row.ui           |    3 -
 src/resources/ui/nautilus-window.ui                |   15 +-
 status.sh                                          |   73 --
 98 files changed, 962 insertions(+), 7175 deletions(-)

## AI Agent Tasks

1. @copilot: Analyze breaking changes and compatibility
2. @claude: Implement merge and resolve conflicts
3. @codex: Code review and QA
