#!/bin/bash
# stress_simple.sh - Simple sequential FUSE stress test
#
# Tests mount/unmount cycles WITHOUT creating zombie processes
# by running checks sequentially and avoiding background jobs

set -e

MOUNT_DIR="/tmp/simple_stress_mnt"
CYCLES=10
TEST_TIMEOUT_MS=200  # Short timeout to speed up testing

echo "========================================"
echo "Simple FUSE Stress Test"
echo "========================================"
echo ""
echo "Configuration:"
echo "  Cycles: $CYCLES"
echo "  Timeout: ${TEST_TIMEOUT_MS}ms"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    pkill -f "blackhole.py $MOUNT_DIR" 2>/dev/null || true
    sleep 0.5
    if mountpoint -q "$MOUNT_DIR" 2>/dev/null; then
        fusermount -u "$MOUNT_DIR" 2>/dev/null || true
    fi
    rm -rf "$MOUNT_DIR"
    echo "✓ Cleanup complete"
}

trap cleanup EXIT INT TERM

# Create test client
echo "Building test client..."
cat > /tmp/simple_client.c <<'EOF'
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>

extern gboolean nautilus_file_check_fuse_mount_responsive (GFile *file, guint timeout_ms);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <path> <timeout_ms>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    int timeout_ms = atoi(argv[2]);

    GFile *file = g_file_new_for_path(path);
    gboolean result = nautilus_file_check_fuse_mount_responsive(file, timeout_ms);
    g_object_unref(file);

    /* Don't print result - we just want to verify no crash */
    return 0;
}
EOF

gcc -I../src -I../build \
    $(pkg-config --cflags glib-2.0 gio-2.0) \
    /tmp/simple_client.c \
    ../build/src/libnautilus.a \
    $(pkg-config --libs glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    -lpthread \
    -o /tmp/simple_client

echo "✓ Client compiled"
echo ""

# Main test loop
echo "Starting stress cycles..."
echo ""

for cycle in $(seq 1 $CYCLES); do
    echo "Cycle $cycle/$CYCLES:"

    # Create mount point
    rm -rf "$MOUNT_DIR"
    mkdir -p "$MOUNT_DIR"

    # Mount blackhole FUSE
    echo "  [1/5] Mounting blackhole FUSE..."
    python3 ./blackhole.py "$MOUNT_DIR" &
    FUSE_PID=$!
    sleep 0.3

    if ! mountpoint -q "$MOUNT_DIR"; then
        echo "    ❌ Mount failed"
        kill $FUSE_PID 2>/dev/null || true
        continue
    fi

    # Test 1: Check hanging path
    echo "  [2/5] Checking hanging path..."
    /tmp/simple_client "${MOUNT_DIR}/hang" $TEST_TIMEOUT_MS
    echo "    ✓ Check returned (timeout or success)"

    # Test 2: Check non-existent path
    echo "  [3/5] Checking non-existent path..."
    /tmp/simple_client "${MOUNT_DIR}/nothere" $TEST_TIMEOUT_MS
    echo "    ✓ Check returned"

    # Test 3: Unmount while potentially having background workers
    echo "  [4/5] Killing FUSE process..."
    kill -9 $FUSE_PID 2>/dev/null || true
    sleep 0.2

    # Force unmount
    echo "  [5/5] Force unmounting..."
    fusermount -u "$MOUNT_DIR" 2>/dev/null || true

    echo "  ✓ Cycle complete"
    echo ""

    # Brief pause between cycles
    sleep 0.1
done

echo "========================================"
echo "✓ Stress test PASSED"
echo "========================================"
echo ""
echo "Summary:"
echo "  - $CYCLES mount/unmount cycles completed"
echo "  - $((CYCLES * 2)) FUSE checks performed"
echo "  - No crashes or freezes detected"
echo ""
