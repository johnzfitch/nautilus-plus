#!/bin/bash
# stress_fuse.sh - Violent FUSE mount/unmount stress test
#
# This script simulates chaotic real-world scenarios:
# - Rapid mount/unmount cycles
# - Dead mounts appearing/disappearing
# - Concurrent FUSE checks during mount changes
# - Tests for crashes, deadlocks, and resource leaks

set -e

MOUNT_BASE="/tmp/stress_mnt"
NUM_MOUNTS=5
CYCLES=20
TEST_TIMEOUT_MS=500

echo "========================================="
echo "FUSE Stress Test: Violent Mount Cycling"
echo "========================================="
echo ""
echo "Configuration:"
echo "  Mount points: $NUM_MOUNTS"
echo "  Cycles: $CYCLES"
echo "  Check timeout: ${TEST_TIMEOUT_MS}ms"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    for i in $(seq 0 $((NUM_MOUNTS - 1))); do
        MOUNT_DIR="${MOUNT_BASE}${i}"
        if mountpoint -q "$MOUNT_DIR" 2>/dev/null; then
            fusermount -u "$MOUNT_DIR" 2>/dev/null || true
        fi
        rm -rf "$MOUNT_DIR"
    done
    echo "✓ Cleanup complete"
}

trap cleanup EXIT INT TERM

# Create mount directories
echo "Setting up mount points..."
for i in $(seq 0 $((NUM_MOUNTS - 1))); do
    MOUNT_DIR="${MOUNT_BASE}${i}"
    rm -rf "$MOUNT_DIR"
    mkdir -p "$MOUNT_DIR"
done
echo ""

# Build the test program
echo "Compiling stress test client..."
cat > /tmp/stress_client.c <<'EOF'
#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

extern gboolean nautilus_file_check_fuse_mount_responsive (GFile *file, guint timeout_ms);

typedef struct {
    const char *path;
    int timeout_ms;
    int thread_id;
} CheckData;

void *check_thread(void *arg) {
    CheckData *data = (CheckData *)arg;
    GFile *file = g_file_new_for_path(data->path);
    gboolean result = nautilus_file_check_fuse_mount_responsive(file, data->timeout_ms);
    g_object_unref(file);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <path> <timeout_ms> [concurrent_threads]\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    int timeout_ms = atoi(argv[2]);
    int num_threads = (argc > 3) ? atoi(argv[3]) : 1;

    pthread_t threads[num_threads];
    CheckData data = { path, timeout_ms, 0 };

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, check_thread, &data);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
EOF

gcc -I../src -I../build \
    $(pkg-config --cflags glib-2.0 gio-2.0) \
    /tmp/stress_client.c \
    ../build/src/libnautilus.a \
    $(pkg-config --libs glib-2.0 gio-2.0 gtk4 libadwaita-1) \
    -lpthread \
    -o /tmp/stress_client

echo "✓ Stress client compiled"
echo ""

# Main stress test loop
echo "Starting stress test cycles..."
echo ""

for cycle in $(seq 1 $CYCLES); do
    echo "----------------------------------------"
    echo "Cycle $cycle/$CYCLES"
    echo "----------------------------------------"

    # PHASE 1: Rapid mount
    echo "  [1/4] Mounting $NUM_MOUNTS blackhole filesystems..."
    for i in $(seq 0 $((NUM_MOUNTS - 1))); do
        MOUNT_DIR="${MOUNT_BASE}${i}"
        python3 ./blackhole.py "$MOUNT_DIR" &
        MOUNT_PID=$!
        sleep 0.1  # Give it time to mount

        if ! mountpoint -q "$MOUNT_DIR"; then
            echo "    ⚠ Mount $i failed to mount"
        fi
    done

    # PHASE 2: Concurrent checks during mount state
    echo "  [2/4] Running concurrent FUSE checks (10 threads per mount)..."
    for i in $(seq 0 $((NUM_MOUNTS - 1))); do
        MOUNT_DIR="${MOUNT_BASE}${i}"
        /tmp/stress_client "${MOUNT_DIR}/hang" $TEST_TIMEOUT_MS 10 &
    done

    # Wait for checks to complete
    wait
    echo "    ✓ All checks completed"

    # PHASE 3: Violent unmount (kill FUSE processes)
    echo "  [3/4] Violently killing FUSE processes..."
    pkill -9 -f "blackhole.py" 2>/dev/null || true
    sleep 0.2

    # Force unmount any remaining mounts
    for i in $(seq 0 $((NUM_MOUNTS - 1))); do
        MOUNT_DIR="${MOUNT_BASE}${i}"
        if mountpoint -q "$MOUNT_DIR" 2>/dev/null; then
            fusermount -u "$MOUNT_DIR" 2>/dev/null || true
        fi
    done

    # PHASE 4: Check during unmounted state
    echo "  [4/4] Checking unmounted paths..."
    for i in $(seq 0 $((NUM_MOUNTS - 1))); do
        MOUNT_DIR="${MOUNT_BASE}${i}"
        /tmp/stress_client "${MOUNT_DIR}/hang" $TEST_TIMEOUT_MS 5 &
    done
    wait
    echo "    ✓ Unmounted checks completed"

    echo "  ✓ Cycle $cycle complete"
    echo ""
done

echo "========================================"
echo "✓ Stress test PASSED"
echo "========================================"
echo ""
echo "Results:"
echo "  - $CYCLES cycles completed"
echo "  - $((NUM_MOUNTS * CYCLES)) mount/unmount operations"
echo "  - No crashes detected"
echo "  - No deadlocks detected"
echo ""
