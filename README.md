# Nautilus-Plus

Enhanced GNOME Files (Nautilus) with performance optimizations and extended features.

**Repository:** https://github.com/johnzfitch/nautilus-plus
**AUR Package:** [nautilus-plus](https://aur.archlinux.org/packages/nautilus-plus)
**Based on:** GNOME Nautilus 50.alpha

## Features

- **Fast Search** - Sub-millisecond search via search-cache integration (2.3M+ files indexed)
- **Larger Thumbnails** - HUGE (384px) and GIGANTIC (512px) zoom levels for media files
- **Search Results Limit** - Configurable limit (default 1000) prevents memory exhaustion
- **FUSE Mount Detection** - 1-second timeout prevents UI hangs on stale remote mounts
- **Crash Prevention** - Lifecycle management fixes prevent use-after-free crashes

## Installation (Arch Linux)

### Option 1: AUR Package (Recommended)

```bash
yay -S nautilus-plus search-cache
systemctl --user enable --now search-cache
```

### Option 2: Build from Source

```bash
git clone https://github.com/johnzfitch/nautilus-plus
cd nautilus-plus
meson setup build
ninja -C build
sudo ninja -C build install
```

## ⚠️ IMPORTANT: search-cache Requirement

**Nautilus-Plus requires search-cache daemon to be running for fast search.**

Without search-cache:
- ❌ Search will be **very slow** (recursively scans every file)
- ❌ High CPU usage during search
- ❌ Fans may spin up

With search-cache:
- ✅ **Sub-millisecond search** (uses trigram index)
- ✅ Low CPU usage
- ✅ Instant results

### Verify search-cache is Running

```bash
systemctl --user status search-cache
```

Should show: `Active: active (running)`

If not running:
```bash
systemctl --user start search-cache
systemctl --user enable search-cache  # Auto-start on boot
```

### Check Indexed Files

```bash
sc stats
```

Should show your file count (e.g., "Files indexed: 2313792")

## Configuration

### Search Results Limit

Open **Preferences → Performance** and adjust "Search Results Limit"
- Default: 1000
- Range: 0-10000 (0 = unlimited, not recommended)

Or via command line:
```bash
gsettings set org.gnome.nautilus.preferences search-results-limit 2000
```

### Larger Thumbnails

Use the zoom slider or:
```bash
# Set to GIGANTIC (512px)
gsettings set org.gnome.nautilus.icon-view default-zoom-level 6
```

Zoom levels:
- 0 = Small (48px)
- 1 = Small Plus (64px)
- 2 = Medium (96px)
- 3 = Large (168px)
- 4 = Extra Large (256px)
- 5 = HUGE (384px) ⭐ NEW
- 6 = GIGANTIC (512px) ⭐ NEW

## Performance

**Memory Usage:**
- Small directory (100 files): ~3.8 MB
- Medium directory (1,000 files): ~3.8 MB
- Large directory (10,000 files): ~3.8 MB

**Search Performance** (with search-cache):
- 2.3M files: <1ms
- No CPU spike
- Instant results

## Troubleshooting

### Search is slow / high CPU usage

**Cause:** search-cache daemon not running

**Fix:**
```bash
systemctl --user start search-cache
systemctl --user enable search-cache
```

### No search results

**Cause:** search-cache index needs rebuilding

**Fix:**
```bash
sc rebuild
```

### Search-cache not starting

Check the service logs:
```bash
journalctl --user -u search-cache -n 50
```

Common issues:
- Index corruption → Run `sc rebuild`
- Permission issues → Check `~/.local/share/search-cache/`
- Out of memory → Reduce index size or increase swap

## Documentation

- [Installation Guide](INSTALLATION.md)
- [Search Blacklist Guide](docs/search-blacklist.md)
- [Detailed Installation](docs/installation.md)

## Upstream

Nautilus-Plus is a fork of [GNOME Nautilus](https://gitlab.gnome.org/GNOME/nautilus).

For upstream issues, use the [GNOME issue tracker](https://gitlab.gnome.org/GNOME/nautilus/issues).

For nautilus-plus specific issues, use [GitHub Issues](https://github.com/johnzfitch/nautilus-plus/issues).

## License

GPL-3.0-or-later (same as upstream GNOME Nautilus)
=======
# nautilus
[![Pipeline status](https://gitlab.gnome.org/GNOME/nautilus/badges/main/pipeline.svg)](https://gitlab.gnome.org/GNOME/nautilus/commits/main)
[![coverage report](https://gitlab.gnome.org/GNOME/nautilus/badges/main/coverage.svg)](https://gitlab.gnome.org/GNOME/nautilus/commits/main) 

This is the project of the [Files](https://apps.gnome.org/Nautilus/) app, a file browser for
GNOME, internally known by its historical name `nautilus`.

## Supported version
Only the latest version of Files as provided upstream is supported. Try out the [Flatpak nightly](https://welcome.gnome.org/en/app/Nautilus/#installing-a-nightly-build) installation before filling issues to ensure the installation is reproducible and doesn't have downstream changes on it. In case you cannot reproduce in the nightly installation, don't hesitate to file an issue in your distribution. This is to ensure the issue is well triaged and reaches the proper people.

## Runtime dependencies
- [Bubblewrap](https://github.com/projectatomic/bubblewrap) installed. Used for security reasons.
- [LocalSearch](https://gitlab.gnome.org/GNOME/localsearch) properly set up and with all features enabled. Used for fast search and metadata extraction, starred files and batch renaming.
- [xdg-user-dirs-gtk](https://gitlab.gnome.org/GNOME/xdg-user-dirs-gtk) installed.  Used to create the default bookmarks and update localization.

## Discourse

For more informal discussion we use [GNOME Discourse](https://discourse.gnome.org/tags/nautilus) in the Applications category with the `nautilus` tag. Feel free to open a topic there.

## Extensions

Documentation for the libnautilus-extension API is available [here](https://gnome.pages.gitlab.gnome.org/nautilus/).  Also, if you are interested in developing a Nautilus extension in Python you should refer to the [nautilus-python](https://gnome.pages.gitlab.gnome.org/nautilus-python/) documentation.

## How to report issues

Report issues to the GNOME [issue tracking system](https://gitlab.gnome.org/GNOME/nautilus/issues).

## Feature requests

Files is a core compoment of the GNOME desktop experience. As such, any changes in behavior or appearance only happen in accordance with the [GNOME design team][design-team].

For major changes, it is best to start a discussion on [discourse] and reach out on the [#gnome-design matrix room][design-room], and only involve the issue tracker once agreement has been reached.

In particular mockups must be approved by the design team to be considered for implementation.

For enhancements that are limited in scope and well-defined, it is acceptable to directly open an issue using the shortcoming template.

[design-team]: https://gitlab.gnome.org/Teams/Design
[discourse]: https://discourse.gnome.org/tag/nautilus
[design-room]: https://matrix.to/#/#gnome-design:gnome.org
>>>>>>> upstream/main
