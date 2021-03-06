# sackman

[![Build Status](https://travis-ci.org/ak-1/sackman.svg?branch=master)](https://travis-ci.org/ak-1/sackman)

## Description

Sackman shrinks containers by throwing out unused files.

The C code is based on Remi Flament's LoggedFS (https://github.com/rflament/loggedfs).

## Usage example

Call sackman like you would call `podman run`:

    $ DST_IMAGE=test sackman run docker.io/library/ubuntu:14.04 cat /etc/lsb-release

**NOTE**: For options that appear prior to the image name:
Do *not* put a space in front of option-values. Use `--option-name=VALUE` or `-oVALUE` instead.
Otherwise sackman may not identify the image name correctly.

While the container is running sackman will record all accessed file paths.
Finally a squashed copy of the origial image will be created containing only the accessed files.

You can configure sackman with the following environment variables:

    DST_IMAGE - The name of the final image.
    FILE_LOG - Accessed paths are written to this file.
    RSYNC_FILES - A path to a file containing additional files to copy.
    RSYNC_OPTS - Can be set to "-v" to output all copied files.

## Installation

    $ make
    $ sudo make install
or

    $ PREFIX=$HOME/.local make -e install

Sackman has the following dependencies:

    podman
    buildah
    fuse
    rsync

You also need the usual build tools to compile sackman.

Install fuse on Fedora with:

    dnf install fuse-devel

Install fuse on Depian/Ubuntu with:

    apt install libfuse-dev

