#!/bin/bash
# Setup script for Test 3: Pool Starvation
# Creates 4 blackhole FUSE mounts

set -e

echo "Setting up Test 3: Pool Starvation"
echo "==================================="
echo ""

# Create mount points
for i in 1 2 3 4; do
    MOUNT="/tmp/test_mnt$i"
    mkdir -p "$MOUNT"
    echo "✓ Created $MOUNT"
done

echo ""
echo "Starting 4 BlackHole FUSE mounts..."
echo ""

# Start blackhole processes in background
for i in 1 2 3 4; do
    MOUNT="/tmp/test_mnt$i"

    # Check if already mounted
    if mount | grep -q "$MOUNT"; then
        echo "⚠️  $MOUNT already mounted, skipping"
        continue
    fi

    # Start blackhole
    ./blackhole.py "$MOUNT" > "/tmp/blackhole_$i.log" 2>&1 &
    PID=$!
    echo "✓ Started blackhole on $MOUNT (PID: $PID)"

    # Give it a moment to mount
    sleep 0.5
done

echo ""
echo "Verifying mounts..."
echo ""

# Verify all mounts are up
ALL_OK=true
for i in 1 2 3 4; do
    MOUNT="/tmp/test_mnt$i"
    if mount | grep -q "$MOUNT"; then
        echo "✓ $MOUNT is mounted"
    else
        echo "❌ $MOUNT failed to mount"
        ALL_OK=false
    fi
done

echo ""

if $ALL_OK; then
    echo "✅ All 4 mounts ready for Test 3"
    echo ""
    echo "Run the test:"
    echo "  make run-test3"
    echo ""
    echo "To cleanup:"
    echo "  ./cleanup_test3.sh"
else
    echo "❌ Setup failed - check /tmp/blackhole_*.log for errors"
    exit 1
fi
