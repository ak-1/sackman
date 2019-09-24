#!/bin/bash

export SACKMAN_PREFIX="$( dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )" )"

source $SACKMAN_PREFIX/lib/sackman/util.sh

usage() {
    echo "usage: $(basename "$0") run [flags] IMAGE [COMMAND [ARG...]]" >/dev/stderr
    exit 1
}

[[ -z "$1" || "$1" == "-h" || "$1" == "--help" ]] && usage

SACKMAN_MOUNT=$SACKMAN_PREFIX/lib/sackman/fuse-overlayfs.sh
SACKMAN_RSYNC=$SACKMAN_PREFIX/lib/sackman/rsync.sh

export SACKMAN_OUTFILE="/tmp/sackman.$USER.$$"

for arg in "$@"
do
    [[ "$arg" =~ ^- ]] || {
        if [[ -z "$cmd" ]]
        then
            cmd="$arg"
        else
            SRC_IMAGE="$arg"
            break
        fi
    }
done

[[ "$cmd" == "run" ]] || error "Invalid command \"$cmd\"."

[[ -n "$SRC_IMAGE" ]] || error "Could not find image name in command."

call podman \
    --storage-driver overlay \
    --storage-opt "overlay.mount_program=$SACKMAN_MOUNT" \
    "$@"

[[ -z "$DST_IMAGE" ]] && read -p "Enter the new image name: " DST_IMAGE

export SRC_CONT=$(call buildah from "$SRC_IMAGE") || exit

export DST_CONT=$(call buildah from scratch) || exit

call buildah unshare $SACKMAN_RSYNC || exit

call buildah rm $SRC_CONT || exit

call buildah commit --squash $DST_CONT "$DST_IMAGE" || exit

call buildah rm $DST_CONT

call rm $SACKMAN_OUTFILE || exit
