# Nautilus-Plus Complete Installation Guide

## What You're Getting

**Nautilus-Plus** is an enhanced version of GNOME Files (Nautilus) with:
- ✅ User-configurable search blacklist (exclude large directories)
- ✅ Depth-limited search (lazy loading for blacklisted directories)
- ✅ Larger thumbnails (up to 512px for media files)
- ✅ Integration with search-cache for sub-millisecond search

**Recommended:** Install both nautilus-plus AND search-cache for best performance.

## Installation Methods

### Method 1: Full Installation (Recommended)

Install both packages for optimal performance:

```bash
# Install nautilus-plus (enhanced file manager)
yay -S nautilus-plus

# Install search-cache (fast trigram indexer)
yay -S search-cache
```

### Method 2: Nautilus-Plus Only

If you only want the blacklist and thumbnail features without the search daemon:

```bash
yay -S nautilus-plus
```

## Initial Configuration

### 1. Configure Search Blacklist

Choose patterns based on your needs:

**For Media Libraries:**
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/bigboy', \
  '/home/*/Media', \
  '/home/*/Videos' \
]"
```

**For Developers:**
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/node_modules', \
  '/home/*/build', \
  '/home/*/dist', \
  '/home/*/.venv', \
  '/home/*/.cargo/registry' \
]"
```

**For Gamers:**
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/.local/share/Steam', \
  '/home/*/.wine', \
  '/home/*/Games' \
]"
```

**General System Cleanup:**
```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/.cache', \
  '/home/*/.thumbnails', \
  '/tmp/*' \
]"
```

### 2. Configure search-cache (if installed)

```bash
# Add directories to index (your frequently searched locations)
sc config add-dir ~/Documents
sc config add-dir ~/Downloads
sc config add-dir ~/Projects

# Remove blacklisted directories from index
sc config remove-dir ~/bigboy
sc config remove-dir ~/.cache

# Build initial index
sc rebuild

# Start the daemon (will auto-start on login)
systemctl --user enable --now search-cache
```

**Check daemon status:**
```bash
sc status
```

### 3. Enable Larger Thumbnails (Optional)

For media-heavy workflows:

```bash
# 512px thumbnails (great for photography/video)
gsettings set org.gnome.nautilus.icon-view default-zoom-level 'gigantic'

# 384px thumbnails (middle ground)
gsettings set org.gnome.nautilus.icon-view default-zoom-level 'huge'

# Reset to default (256px)
gsettings reset org.gnome.nautilus.icon-view default-zoom-level
```

## Verifying Installation

```bash
# Check Nautilus version
nautilus --version

# Verify blacklist is set
gsettings get org.gnome.nautilus.preferences search-blacklist

# Check search-cache status (if installed)
sc status

# View indexed file count
sc stats
```

## Usage

### Search Behavior

**Searching from home directory:**
- Blacklisted directories are skipped
- Fast results from non-blacklisted locations
- No UI freezes

**Searching FROM a blacklisted directory (e.g., ~/bigboy):**
- Shows immediate folders only (depth 1)
- Navigate into folder → shows files in that folder
- Prevents overwhelming UI with millions of results

**Searching with search-cache:**
- Sub-millisecond results for indexed locations
- Automatic index updates via inotify
- Falls back to simple search for non-indexed locations

### Adjusting Configuration

```bash
# View current blacklist
gsettings get org.gnome.nautilus.preferences search-blacklist

# Add more patterns
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/bigboy', \
  '/home/*/new-pattern' \
]"

# Reset to empty (no blacklist)
gsettings reset org.gnome.nautilus.preferences search-blacklist
```

## Performance Comparison

### Before Nautilus-Plus

Searching from `~` with large media directory:
- **CPU:** 14-127% sustained
- **File operations:** 2,767 openat() calls in 3 seconds
- **Result:** UI freeze, slow response

### After Nautilus-Plus

Same search with blacklist configured:
- **CPU:** Normal usage
- **File operations:** Large directories skipped
- **Result:** Instant, responsive UI

### With search-cache

Indexed locations:
- **Search time:** <1ms for 2.15M files
- **CPU:** Minimal
- **Updates:** Real-time via inotify

## Troubleshooting

### Blacklist not working?

```bash
# 1. Verify configuration
gsettings get org.gnome.nautilus.preferences search-blacklist

# 2. Restart Nautilus
nautilus -q && nautilus ~

# 3. Check pattern syntax (must be absolute paths with wildcards)
# ✅ Good: '/home/*/bigboy'
# ❌ Bad: '~/bigboy' or 'bigboy'
```

### search-cache not indexing?

```bash
# Check daemon status
systemctl --user status search-cache

# View logs
journalctl --user -u search-cache -f

# Rebuild index
sc rebuild

# Check which directories are indexed
sc config list-dirs
```

### Thumbnails not showing at larger sizes?

```bash
# Verify setting
gsettings get org.gnome.nautilus.icon-view default-zoom-level

# Increase thumbnail size limit (if needed)
gsettings set org.gnome.nautilus.preferences thumbnail-limit 512

# Clear thumbnail cache and regenerate
rm -rf ~/.cache/thumbnails
nautilus -q && nautilus ~
```

## Uninstallation

### Remove Nautilus-Plus

```bash
# Remove package
yay -R nautilus-plus

# Install vanilla Nautilus
sudo pacman -S nautilus

# Reset settings (optional)
gsettings reset org.gnome.nautilus.preferences search-blacklist
```

### Remove search-cache

```bash
# Stop daemon
systemctl --user stop search-cache
systemctl --user disable search-cache

# Remove package
yay -R search-cache

# Remove index data (optional)
rm -rf ~/.local/share/search-cache
```

## Advanced Configuration

### Custom Blacklist Patterns

```bash
# Wildcard examples
'/home/*/pattern'        # Matches any user
'/path/to/specific/dir'  # Exact path only
'*/node_modules'         # Any node_modules directory
'/mnt/*/media'          # External drives
```

### search-cache Advanced Options

```bash
# Exclude file types from index
sc config set-exclude-patterns "*.tmp,*.log,*.cache"

# Set index update interval
sc config set-update-interval 300  # seconds

# Limit indexed file size
sc config set-max-file-size 10485760  # 10MB
```

### Integration with Other Tools

**With fzf for terminal search:**
```bash
# Add to ~/.bashrc
alias scs='sc search'
```

**With ripgrep for content search:**
```bash
# Exclude blacklisted directories
rg --glob '!bigboy/**' --glob '!node_modules/**' 'pattern'
```

## Resources

- **Nautilus-Plus GitHub:** https://github.com/johnzfitch/nautilus-fork
- **search-cache Documentation:** (link to search-cache repo)
- **Configuration Guide:** `/tmp/nautilus-search-blacklist-guide.md`
- **Issue Tracker:** https://github.com/johnzfitch/nautilus-fork/issues

## Contributing

Found a bug or have a feature request? Open an issue on GitHub!

Want to contribute code? PRs welcome for:
- Additional zoom levels
- UI preferences panel for blacklist configuration
- Performance optimizations
- Documentation improvements

## FAQ

**Q: Will this conflict with vanilla Nautilus?**
A: nautilus-plus replaces nautilus. Uninstall vanilla Nautilus first.

**Q: Do I need search-cache?**
A: No, but highly recommended. Nautilus-Plus works standalone, but search-cache provides sub-millisecond search for indexed locations.

**Q: Can I use this on other distros?**
A: Currently packaged for Arch Linux (AUR). For other distros, you can build from source (see GitHub).

**Q: Will this be upstreamed to GNOME?**
A: The configurable blacklist feature is a good candidate for upstream. We may propose it to GNOME developers.

**Q: How do I report performance issues?**
A: Open a GitHub issue with:
  - Your blacklist configuration
  - Output of `sc stats` (if using search-cache)
  - Steps to reproduce
  - System specs

## License

Nautilus-Plus follows the same GPL-3.0 license as upstream Nautilus.
