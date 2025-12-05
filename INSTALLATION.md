# Nautilus-Plus Installation Guide

## Quick Install

```bash
# Install both packages for complete experience
yay -S nautilus-plus search-cache
```

## Configuration

### Search Blacklist

```bash
gsettings set org.gnome.nautilus.preferences search-blacklist "[ \
  '/home/*/node_modules', \
  '/home/*/.cache' \
]"
```

### Search Cache

```bash
sc config add-dir ~/Documents ~/Downloads
sc rebuild
systemctl --user enable --now search-cache
```

See full documentation in `docs/` directory.
