#!/bin/bash

fuse-overlayfs "$@"

$SACKMAN_PREFIX/lib/sackman/sackman.bin -p "${@: -1}"
