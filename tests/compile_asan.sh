#!/bin/bash
# Compile ASan use-after-free detection test

set -e

echo "Compiling test_asan_uaf with AddressSanitizer..."
gcc -fsanitize=address -g -O1 \
    -I../src -I../build-asan \
    $(pkg-config --cflags glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    test_asan_uaf.c \
    ../build-asan/src/libnautilus.a \
    $(pkg-config --libs glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    -lpthread -fsanitize=address \
    -o test_asan_uaf

echo "✓ ASan test compiled successfully"
