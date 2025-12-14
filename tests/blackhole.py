#!/usr/bin/env python3
"""
BlackHole FUSE - A Toxic FUSE Mount for Testing

This filesystem intentionally hangs forever on certain paths to simulate
a dead SSHFS mount or network partition.

Usage: ./blackhole.py /tmp/test_mnt
"""
import time
import errno
import stat
import fuse
from fuse import Fuse

fuse.fuse_python_api = (0, 2)


class BlackHoleFS(Fuse):
    """
    A FUSE filesystem that simulates an unresponsive mount.

    - /hang: Blocks forever in D-state
    - /: Normal directory
    - Everything else: ENOENT
    """

    def getattr(self, path):
        """
        Get file attributes. Hangs forever on /hang.
        """
        print(f"[BlackHole] getattr({path})")

        if path == "/hang":
            # Simulate the D-State / Infinite Hang
            # The calling thread will block here forever
            print(f"[BlackHole] stat({path}) - Entering infinite hang...")
            while True:
                time.sleep(10)

        # Normal behavior for root
        if path == "/":
            st = fuse.Stat()
            st.st_mode = stat.S_IFDIR | 0o755
            st.st_nlink = 2
            st.st_size = 4096
            st.st_atime = int(time.time())
            st.st_mtime = int(time.time())
            st.st_ctime = int(time.time())
            return st

        print(f"[BlackHole] getattr({path}) - ENOENT")
        return -errno.ENOENT

    def readdir(self, path, offset):
        """List directory contents."""
        if path == "/":
            for entry in ['.', '..', 'hang']:
                yield fuse.Direntry(entry)
        else:
            return -errno.ENOENT


def main():
    usage = """
BlackHole FUSE: Toxic filesystem for testing

Usage: %prog <mountpoint>

Test paths:
  <mountpoint>/hang  (will hang forever)
  <mountpoint>/      (normal)
"""
    server = BlackHoleFS(version="%prog " + fuse.__version__,
                         usage=usage,
                         dash_s_do='setsingle')

    server.parse(errex=1)
    print(f"[BlackHole] Mounting toxic filesystem at {server.fuse_args.mountpoint}")
    print(f"[BlackHole] Press Ctrl+C to unmount\n")
    server.main()


if __name__ == '__main__':
    main()
