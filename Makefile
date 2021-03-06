
PREFIX=/usr/local
CC=g++
CFLAGS=-Wall -ansi -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -std=c++11
LDLIBS=-lfuse -lpthread
builddir=build

all: $(builddir) $(builddir)/sackman

$(builddir):
	mkdir $(builddir)

sackman.bin: $(builddir)/sackman.o
	$(CC) -o $(builddir)/sackman.bin $(builddir)/sackman.o $(LDLIBS)

$(builddir)/sackman.o: sackman.cpp
	$(CC) $(CFLAGS) -o $(builddir)/sackman.o -c sackman.cpp

install: $(builddir) sackman.bin
	install -v -D sackman.sh ${PREFIX}/bin/sackman
	install -v -D fuse-overlayfs.sh ${PREFIX}/lib/sackman/fuse-overlayfs.sh
	install -v -D rsync.sh ${PREFIX}/lib/sackman/rsync.sh
	install -v -D util.sh ${PREFIX}/lib/sackman/util.sh
	install -v -D $(builddir)/sackman.bin ${PREFIX}/lib/sackman/sackman.bin

uninstall:
	rm -rvf ${PREFIX}/lib/sackman/ ${PREFIX}/bin/sackman

clean:
	rm -rf $(builddir)/

clean_podman:
	buildah rm -a
	podman container rm -af
	podman system prune -f
