# Nautilus Search Blacklist Configuration

## Overview

Nautilus-Plus supports configurable search blacklists to exclude large directories from recursive search. This improves performance and prevents UI freezes when searching from parent directories.

## How It Works

**Key behaviors:**
- Blacklisted directories are **skipped during recursive search** from parent directories
- You can still **search FROM within** a blacklisted directory (it becomes the search root)
- Supports **glob patterns** with wildcards for flexible matching
- When searching from a blacklisted directory, **depth-limited search** applies (shows folders first, files when navigating into folders)

## Configuration

### Using gsettings (Command Line)

```bash
# View current blacklist
gsettings get org.gnome.nautilus.preferences search-blacklist

# Add directories to blacklist (array of patterns)
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/bigboy', \
  '/home/*/.cache', \
  '/home/*/node_modules', \
  '/home/*/build', \
  '/home/*/.local/share/Steam' \
]"

# Reset to empty (no blacklist)
gsettings reset org.gnome.nautilus.preferences search-blacklist
```

### Pattern Syntax

Patterns use simple glob matching (similar to shell wildcards):

| Pattern | Matches | Example |
|---------|---------|---------|
| `/home/*/bigboy` | Any user's bigboy folder | `/home/zack/bigboy`, `/home/alice/bigboy` |
| `/home/zack/.cache` | Specific user's cache | `/home/zack/.cache` only |
| `/home/*/node_modules` | Any user's node_modules | `/home/zack/projects/myapp/node_modules` |
| `*/build` | Any directory named "build" | `/home/zack/project/build`, `/opt/build` |
| `/mnt/storage/media` | Specific absolute path | `/mnt/storage/media` only |

**Important:** Always use absolute paths, not relative paths.

## Common Use Cases

### 1. Large Media Libraries
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/Media', \
  '/home/*/Videos', \
  '/home/*/bigboy' \
]"
```

### 2. Development Environments
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/node_modules', \
  '/home/*/build', \
  '/home/*/dist', \
  '/home/*/.venv', \
  '/home/*/.cargo', \
  '/home/*/.rustup' \
]"
```

### 3. System Caches and Temporary Files
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/.cache', \
  '/home/*/.thumbnails', \
  '/tmp/*', \
  '/var/tmp/*' \
]"
```

### 4. Gaming Directories
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/.local/share/Steam', \
  '/home/*/.wine', \
  '/home/*/Games' \
]"
```

## Search Behavior Examples

### Example 1: Searching from Home Directory
```
Current location: /home/zack/
Search query: "photo"
Blacklist: ['/home/*/bigboy']

Result: Searches all of /home/zack/ EXCEPT /home/zack/bigboy
```

### Example 2: Searching FROM Blacklisted Directory
```
Current location: /home/zack/bigboy/
Search query: "video"
Blacklist: ['/home/*/bigboy']

Result:
  - Shows immediate folders in bigboy (depth 0 → 1)
  - Navigate into folder → shows files in that folder only
  - Does NOT recursively scan all subdirectories
```

### Example 3: Multiple Patterns
```
Current location: /home/zack/
Search query: "readme"
Blacklist: ['/home/*/node_modules', '/home/*/.cache', '/home/*/build']

Result: Searches home directory, skipping:
  - All node_modules folders
  - .cache directory
  - All build folders
```

## Performance Impact

**Before blacklist:**
- Searching from ~ with large media folder: 2,767 file opens in 3 seconds
- CPU usage: 14-127% sustained
- UI freezes during search

**After blacklist:**
- Large directories skipped automatically
- Normal CPU usage
- Responsive UI during search
- Instant results from fast locations

## Depth-Limited Search

When searching FROM a blacklisted directory, depth-limited search applies:

1. **Initial view:** Shows only immediate subdirectories (folders), not their contents
2. **Navigate into folder:** Shows files in that specific folder only
3. **No recursive descent:** Prevents loading millions of files at once

This is perfect for large media libraries where you want to browse by folder, not see every file.

## Troubleshooting

### Blacklist not working?
1. Verify syntax: `gsettings get org.gnome.nautilus.preferences search-blacklist`
2. Ensure patterns use absolute paths (start with `/`)
3. Restart Nautilus: `nautilus -q && nautilus ~`
4. Check pattern matching: patterns are case-sensitive

### Still seeing blacklisted directories?
- If you're searching FROM the blacklisted directory, that's expected behavior
- Blacklist only applies to recursive descent from parent directories
- To completely avoid a directory, don't navigate into it

### Pattern not matching?
- Use `*` for wildcards, not regex
- Pattern `/home/*/bigboy` matches `/home/zack/bigboy` but NOT `/home/zack/stuff/bigboy`
- For nested matches, use: `*/bigboy` or `**/bigboy` (both work the same)

## Related Settings

```bash
# Limit total search results (prevents memory issues)
gsettings set org.gnome.nautilus.preferences search-results-limit 1000

# Control where recursive search is enabled
gsettings set org.gnome.nautilus.preferences recursive-search 'local-only'
# Options: 'always', 'local-only', 'never'
```

## Integration with sc-daemon (Search Cache)

If you're using the search-cache daemon:

```bash
# Remove directory from sc-daemon index
sc config remove-dir ~/bigboy

# View indexed directories
sc config list-dirs

# Rebuild index after changes
sc rebuild --incremental
```

**Best practice:** Blacklist in BOTH Nautilus AND sc-daemon for optimal performance.

## Default Configuration

By default, the blacklist is **empty** (`[]`). Users must configure it based on their specific needs.

Suggested starter configuration for most users:
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/.cache', \
  '/home/*/node_modules' \
]"
```

## Technical Details

- **Implementation:** `src/nautilus-search-engine-simple.c`
- **GSettings key:** `org.gnome.nautilus.preferences.search-blacklist`
- **Type:** Array of strings (`as` in GSettings terminology)
- **Pattern matching:** Uses `g_pattern_match_simple()` (glob-style wildcards)
- **Depth tracking:** Hash table mapping directory paths to depth integers

## See Also

- Search results limit: Controls maximum results per search provider
- Recursive search setting: Controls where recursive search is enabled
- sc-daemon configuration: `/home/USER/.config/search-cache/config.toml`
