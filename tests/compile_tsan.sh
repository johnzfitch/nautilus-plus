#!/bin/bash
# Compile tests with ThreadSanitizer

set -e

echo "Compiling test_fuse_dedup with TSan..."
gcc -fsanitize=thread -g -O1 \
    -I../src -I../build-tsan \
    $(pkg-config --cflags glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    test_fuse_dedup.c \
    ../build-tsan/src/libnautilus.a \
    $(pkg-config --libs glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    -lpthread -fsanitize=thread \
    -o test_fuse_dedup_tsan

echo "✓ TSan test compiled: test_fuse_dedup_tsan"
