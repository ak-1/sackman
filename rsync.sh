#!/bin/bash

source $SACKMAN_PREFIX/lib/sackman/util.sh

do_rsync() {
    ENV_NAME="$1"
    FILES="$(eval "echo \$$ENV_NAME")"
    [[ -n "$FILES" ]] && {
        if [[ -f "$FILES" ]]
        then
            info "> rsync -a $RSYNC_OPTS --ignore-missing-args --files-from=$FILES $SRC_DIR $DST_DIR"
            rsync -a $RSYNC_OPTS --ignore-missing-args "--files-from=$FILES" $SRC_DIR $DST_DIR || exit
        else
            error "$ENV_NAME file \"$FILES\" does not exist."
        fi
    }
}

SRC_DIR=$(call buildah mount $SRC_CONT) || exit
DST_DIR=$(call buildah mount $DST_CONT) || exit

do_rsync "SACKMAN_OUTFILE"
do_rsync "RSYNC_FILES"

call buildah umount $SRC_CONT $DST_CONT || exit
