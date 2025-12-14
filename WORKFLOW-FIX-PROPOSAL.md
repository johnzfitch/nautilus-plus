# Workflow Fix Proposal

## Problem

The current upstream sync workflow creates a PR and asks Copilot to merge `upstream/main`, but Copilot runs in a sandboxed environment that cannot access gitlab.gnome.org. This makes the merge impossible to complete.

## Solution

Modify `.github/workflows/upstream-sync-with-ai-agents.yml` to push the upstream commits to the repository before creating the PR.

## Proposed Changes

Add this step after "Add upstream remote" and before "Create feature branch and spec":

```yaml
      - name: Mirror upstream to repository
        env:
          BRANCH: ${{ env.UPSTREAM_BRANCH }}
        run: |
          # Push upstream to a mirror branch in our repository
          git push origin "upstream/$BRANCH:upstream-mirror/$BRANCH" --force
```

Then update the "Create Claude task file" step to reference the mirrored branch:

```yaml
      - name: Create Claude task file
        run: |
          {
            printf '# Task: Implement Upstream Sync\n\n'
            printf '## Steps\n\n'
            printf '1. Merge upstream-mirror/main: git merge upstream-mirror/main\n'  # Changed
            printf '2. Resolve conflicts preserving custom features\n'
            printf '3. Run tests: meson setup build && ninja -C build test\n'
            printf '4. Update documentation\n'
            printf '5. Commit and push\n\n'
            printf '## Custom Features to Preserve\n\n'
            printf '%s\n' '- Animated WebP/GIF/APNG thumbnails'
            printf '%s\n' '- Search-cache integration'
            printf '%s\n' '- Search debounce and preview results'
            printf '%s\n' '- FUSE mount detection'
          } > .claude-task.md
```

And update the PR body to reference the correct branch:

```yaml
      - name: Create Pull Request
        env:
          GH_TOKEN: ${{ secrets.GH_PAT }}
          PR_BRANCH: ${{ steps.create_branch.outputs.pr_branch }}
          UPSTREAM_COMMIT: ${{ needs.check-upstream.outputs.upstream_commit }}
          CURRENT_COMMIT: ${{ needs.check-upstream.outputs.current_commit }}
          SPEC_FILE: ${{ steps.create_branch.outputs.spec_file }}
        run: |
          {
            printf '## Automated Upstream Sync\n\n'
            printf 'Merging upstream GNOME Nautilus changes.\n\n'
            printf '%s\n' "- Upstream: ${UPSTREAM_COMMIT:0:8}"
            printf '%s\n\n' "- Current: ${CURRENT_COMMIT:0:8}"
            printf '### Spec\n\n'
            printf 'See: %s\n\n' "$SPEC_FILE"
            printf '### Custom Features to Preserve\n\n'
            printf '%s\n' '- Animated WebP/GIF/APNG thumbnails'
            printf '%s\n' '- Search-cache integration'
            printf '%s\n\n' '- FUSE mount detection'
            printf '@copilot please merge upstream-mirror/main, resolve any conflicts while preserving our custom features listed above, then mark this PR ready for review.\n'  # Changed
          } > .pr-body.md
          
          gh pr create \
            --base main \
            --head "$PR_BRANCH" \
            --title "Upstream Sync: GNOME Nautilus ${UPSTREAM_COMMIT:0:8}" \
            --body-file .pr-body.md \
            --draft
```

And update the comment that requests Copilot assistance:

```yaml
      - name: Request Copilot assistance
        env:
          GH_TOKEN: ${{ secrets.GH_PAT }}
          PR_BRANCH: ${{ steps.create_branch.outputs.pr_branch }}
        run: |
          PR_NUM=$(gh pr view "$PR_BRANCH" --json number -q .number)
          gh pr comment "$PR_NUM" --body "@copilot merge upstream-mirror/main into this branch, resolve conflicts preserving our custom features (animated thumbnails, search-cache, FUSE detection), then mark ready for review."
```

## Benefits

1. **Fixes the blocker**: Copilot can now access the upstream commits via `upstream-mirror/main` branch
2. **Maintains audit trail**: The upstream mirror branch shows exactly what was merged
3. **Enables future syncs**: Pattern works for all future upstream sync operations
4. **No workflow changes needed**: Copilot's task remains the same, just with accessible commits

## Alternative: Perform Merge in Actions

If you prefer to have GitHub Actions do the entire merge (since it has network access), you could instead:

1. Add a merge step in the workflow:
```yaml
      - name: Attempt upstream merge
        env:
          BRANCH: ${{ env.UPSTREAM_BRANCH }}
        run: |
          git merge "upstream/$BRANCH" --no-commit --no-ff || {
            echo "merge_has_conflicts=true" >> "$GITHUB_OUTPUT"
          }
```

2. Update Copilot's task to resolve conflicts rather than perform the merge:
```
@copilot the merge from upstream has been started. Please resolve any conflicts preserving our custom features, test the build, and mark ready for review.
```

This approach puts the merge operation (which requires network access) in Actions, and leaves only conflict resolution (which doesn't) to Copilot.
