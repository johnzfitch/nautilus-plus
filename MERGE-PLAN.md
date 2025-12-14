# Upstream Merge Plan

## Situation

This PR was created to merge GNOME Nautilus upstream commit `605ea49d` into nautilus-plus. However, the upstream commits are not available in this repository, and the sandboxed environment blocks access to gitlab.gnome.org.

## What Would Be Required

If the upstream commits were made available in this repository (e.g., via a branch like `upstream-mirror/main`), the merge process would involve:

### 1. Pre-Merge Analysis

According to the spec file (`docs/UPSTREAM-SYNC-SPEC-20251214-182010.md`), the upstream has:
- 165 commits since our last sync point
- Changes to 137 files
- Major changes include:
  - Thumbnail loading system rewrite (new NautilusImage widget)
  - Search engine refactoring
  - Application/window location tracking rework
  - File operations improvements
  - Many test additions

### 2. Custom Features to Preserve

nautilus-plus has custom features that must be preserved during merge:

1. **Animated WebP/GIF/APNG thumbnails**
   - Files: `src/nautilus-animated-*.c/h`
   - Integration points: thumbnail generation, grid/list cells
   
2. **Search-cache integration**
   - Files: `src/nautilus-search-engine-searchcache.c/h`
   - Integration: search engine provider system
   
3. **FUSE mount detection**
   - Files: `src/nautilus-file-utilities.c`
   - Purpose: 1-second timeout to prevent UI hangs

4. **Search debounce and result limiting**
   - Files: `src/nautilus-files-view.c`, schema files
   - Purpose: Performance optimization

5. **Larger thumbnail sizes**
   - Schema changes for HUGE (384px) and GIGANTIC (512px) zoom levels

### 3. Potential Conflicts

Based on the spec file, these upstream changes are likely to conflict with our custom features:

#### High Conflict Risk:
- **Thumbnail system rewrite** (new NautilusImage widget)
  - Will conflict with animated thumbnail implementation
  - Files: `src/nautilus-image.c`, `src/nautilus-grid-cell.c`, `src/nautilus-name-cell.c`
  
- **Search engine refactoring**
  - Will conflict with search-cache integration
  - Files: `src/nautilus-search-engine.c`, `src/nautilus-search-*.c`

#### Medium Conflict Risk:
- **File utilities changes**
  - May conflict with FUSE detection code
  - Files: `src/nautilus-file-utilities.c`

- **Application/window changes**
  - May affect our workflow modifications
  - Files: `src/nautilus-application.c`, `src/nautilus-window.c`

### 4. Merge Strategy

1. **Create backup branch** of current state
2. **Attempt automatic merge**: `git merge upstream/main`
3. **Resolve conflicts** preserving custom features:
   - For each conflict, determine if:
     - Custom code can be reapplied on top of new upstream code
     - Upstream changes need to be adapted to work with custom features
     - Custom features need refactoring to work with new upstream architecture
4. **Test thoroughly**:
   - Build: `meson setup build && ninja -C build`
   - Run tests: `ninja -C build test`
   - Manual testing of custom features
5. **Update documentation** if APIs changed

## Current Blocker

**Cannot proceed**: Upstream commits not accessible in repository. The GitHub Actions workflow needs to be modified to:

1. Fetch upstream in Actions environment (which has network access)
2. Push upstream commits to a branch in this repository (e.g., `upstream-mirror/main`)
3. Then create PR asking agent to merge from that branch

## Alternative Approach

If modifying the workflow is not feasible, an alternative would be to:

1. Have the Actions workflow perform the entire merge (it has network access)
2. Push the merge result to the PR branch
3. Ask the agent only to review and test the merge, not perform it

This would require modifying `.github/workflows/upstream-sync-with-ai-agents.yml` to include merge steps before creating the PR.
