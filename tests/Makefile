# Makefile for Nautilus FUSE Test Suite

CC = gcc
PKG_CONFIG = pkg-config
CFLAGS = -Wall -g -O1 -I../src -I../build $(shell $(PKG_CONFIG) --cflags glib-2.0 gio-2.0 gtk4 libadwaita-1)
LDFLAGS = ../build/src/libnautilus.a $(shell $(PKG_CONFIG) --libs glib-2.0 gio-2.0 gtk4 libadwaita-1)

# Test programs
TESTS = test_fuse_timeout test_fuse_dedup test_fuse_pool_limit

all: $(TESTS)

test_fuse_timeout: test_fuse_timeout.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_fuse_dedup: test_fuse_dedup.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lpthread

test_fuse_pool_limit: test_fuse_pool_limit.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TESTS) *.o

# Run tests (requires blackhole.py to be running)
run-test1: test_fuse_timeout
	@echo "Ensure blackhole.py is running: ./blackhole.py /tmp/test_mnt"
	LD_LIBRARY_PATH=../build/src:$$LD_LIBRARY_PATH ./test_fuse_timeout

run-test2: test_fuse_dedup
	@echo "Ensure blackhole.py is running: ./blackhole.py /tmp/test_mnt"
	LD_LIBRARY_PATH=../build/src:$$LD_LIBRARY_PATH ./test_fuse_dedup

run-test3: test_fuse_pool_limit
	@echo "Ensure 4 blackhole mounts are running"
	LD_LIBRARY_PATH=../build/src:$$LD_LIBRARY_PATH ./test_fuse_pool_limit

.PHONY: all clean run-test1 run-test2 run-test3
