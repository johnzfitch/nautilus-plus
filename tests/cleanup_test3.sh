#!/bin/bash
# Cleanup script for Test 3: Pool Starvation
# Unmounts all 4 blackhole FUSE mounts

echo "Cleaning up Test 3 mounts..."
echo ""

# Kill all blackhole processes
pkill -f "blackhole.py /tmp/test_mnt" && echo "✓ Killed blackhole processes"

# Force unmount all test mounts
for i in 1 2 3 4; do
    MOUNT="/tmp/test_mnt$i"
    if mount | grep -q "$MOUNT"; then
        fusermount -u "$MOUNT" 2>/dev/null && echo "✓ Unmounted $MOUNT" || echo "⚠️  Failed to unmount $MOUNT"
    fi
done

echo ""
echo "✅ Cleanup complete"
