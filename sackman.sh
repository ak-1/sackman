#!/bin/bash

export SACKMAN_PREFIX="$( dirname "$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )" )"

source $SACKMAN_PREFIX/lib/sackman/util.sh

usage() {
    cat <<EOF >/dev/stderr
usage: $(basename "$0") run [flags] IMAGE [COMMAND [ARG...]]"

NOTE: For options that appear prior to the image name:
      Do not put a space in front of option-values.
      Use --option-name=VALUE or -oVALUE instead.
      Otherwise sackman may not identify the image name correctly.
EOF
    exit 1
}

[[ -z "$1" || "$1" == "-h" || "$1" == "--help" ]] && usage

SACKMAN_MOUNT=$SACKMAN_PREFIX/lib/sackman/fuse-overlayfs.sh
SACKMAN_RSYNC=$SACKMAN_PREFIX/lib/sackman/rsync.sh

if [[ -n "$FILE_LOG" ]]
then
    export SACKMAN_OUTFILE="$(realpath "$FILE_LOG")"
else
    export SACKMAN_OUTFILE="/tmp/sackman.$USER.$$"
fi

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

[[ -z "$FILE_LOG" ]] && call rm $SACKMAN_OUTFILE || exit
