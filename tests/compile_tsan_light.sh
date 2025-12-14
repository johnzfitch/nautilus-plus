#!/bin/bash
# Compile lightweight TSan test

set -e

echo "Compiling test_tsan_light..."
gcc -fsanitize=thread -g -O1 \
    -I../src -I../build-tsan \
    $(pkg-config --cflags glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    test_tsan_light.c \
    ../build-tsan/src/libnautilus.a \
    $(pkg-config --libs glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    -lpthread -fsanitize=thread \
    -o test_tsan_light

echo "✓ TSan light test compiled"
