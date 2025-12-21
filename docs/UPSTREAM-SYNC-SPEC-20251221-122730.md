# Upstream Sync Specification

Generated: 2025-12-21 12:27:30 UTC
Upstream Commit: 339c168ec212ee3ae6382394d5b2e4f90f3f71cc
Current Commit: 0971d682151f2273da9101a5a23f68f067f58f69

## Upstream Changes


## Files Changed

 .github/assets/icons/document.png                  | Bin 948 -> 0 bytes
 .github/assets/icons/filter.png                    | Bin 1373 -> 0 bytes
 .github/assets/icons/folder.png                    | Bin 1279 -> 0 bytes
 .github/assets/icons/medium-speed.png              | Bin 1860 -> 0 bytes
 .github/assets/icons/search.png                    | Bin 1646 -> 0 bytes
 .github/assets/icons/tick.png                      | Bin 922 -> 0 bytes
 .github/assets/icons/warning.png                   | Bin 789 -> 0 bytes
 .github/workflows/aur-publish.yml                  | 245 ---------------
 .github/workflows/build-and-release.yml            | 238 --------------
 .github/workflows/upstream-sync-with-ai-agents.yml | 207 ------------
 .github/workflows/upstream-sync.yml                | 215 -------------
 .gitignore                                         |  56 ----
 CRASH-ANALYSIS.md                                  | 188 -----------
 INSTALLATION.md                                    |  29 --
 PERFORMANCE-MONITORING.md                          | 235 --------------
 README.md                                          | 173 ++---------
 analyze-performance.sh                             | 149 ---------
 data/org.gnome.nautilus.gschema.xml                |  24 +-
 docs/installation.md                               | 346 ---------------------
 docs/search-blacklist.md                           | 220 -------------
 monitor-nautilus.sh                                | 250 ---------------
 nautilus-plus.install                              |  24 --
 nautilus-watchdog.sh                               |  96 ------
 performance-logs/alerts.log                        |   1 -
 performance-logs/metrics_20251214.csv              |  78 -----
 performance-logs/watchdog.out                      | 314 -------------------
 src/meson.build                                    |   7 -
 src/nautilus-animated-paintable.c                  | 216 -------------
 src/nautilus-animated-paintable.h                  |  29 --
 src/nautilus-animated-thumbnail.c                  | 299 ------------------
 src/nautilus-animated-thumbnail.h                  |  68 ----
 src/nautilus-enums.h                               |   4 -
 src/nautilus-file-utilities-fuse.c                 | 312 -------------------
 src/nautilus-file-utilities.c                      | 117 -------
 src/nautilus-file-utilities.h                      |  23 --
 src/nautilus-global-preferences.h                  |   7 -
 src/nautilus-grid-cell.c                           |  45 ---
 src/nautilus-grid-view.c                           |  14 +-
 src/nautilus-list-view.c                           |   3 -
 src/nautilus-name-cell.c                           | 115 +------
 src/nautilus-name-cell.h                           |   1 -
 src/nautilus-preferences-dialog.c                  | 292 -----------------
 src/nautilus-query-editor.c                        |  58 +---
 src/nautilus-query.c                               |  20 --
 src/nautilus-query.h                               |   4 -
 src/nautilus-search-directory.c                    |   8 +-
 src/nautilus-search-engine-localsearch.c           |  44 +--
 src/nautilus-search-engine-searchcache.c           | 345 --------------------
 src/nautilus-search-engine-searchcache.h           |  24 --
 src/nautilus-search-engine-simple.c                |  59 +---
 src/nautilus-search-engine.c                       |  12 -
 src/nautilus-search-engine.h                       |   5 +-
 src/nautilus-search-hit.c                          |  21 +-
 src/nautilus-sidebar.c                             | 175 +----------
 src/nautilus-sidebar.h                             |   3 -
 src/nautilus-thumbnails.c                          |   2 -
 src/nautilus-thumbnails.h                          |   1 +
 src/nautilus-view-item.c                           |   8 -
 src/nautilus-view-model.c                          |   1 -
 src/nautilus-window.c                              |  70 -----
 src/nautilus-window.h                              |   4 -
 src/resources/style.css                            |  32 --
 src/resources/ui/nautilus-name-cell.ui             |  82 ++---
 src/resources/ui/nautilus-preferences-dialog.ui    |  54 ----
 src/resources/ui/nautilus-sidebar-row.ui           |   3 -
 src/resources/ui/nautilus-window.ui                |  15 +-
 status.sh                                          |  73 -----
 67 files changed, 74 insertions(+), 5689 deletions(-)

## AI Agent Tasks

1. @copilot: Analyze breaking changes and compatibility
2. @claude: Implement merge and resolve conflicts
3. @codex: Code review and QA
