#!/bin/bash

fuse-overlayfs "$@"

$SACKMAN_PREFIX/lib/sackman/sackman.bin "${@: -1}"
