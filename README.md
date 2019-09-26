# sackman

[![Build Status](https://travis-ci.org/ak-1/sackman.svg?branch=master)](https://travis-ci.org/ak-1/sackman)

## Description

sackman helps you reducing the size of your containers to a minimum.

The C code is based on Remi Flament's LoggedFS (https://github.com/rflament/loggedfs).

## Usage example

Call sackman like you would call podman run:

    $ DST_IMAGE=test sackman run docker.io/library/ubuntu:14.04 cat /etc/lsb-release

NOTE: sackman passes all arguments through to podman.
It only interprets the first two arguments that do not start with a '-'.
The first always has to be "run". The second is interpreted as the image name.
Use the "--option-name=ARG" notation for podman options with arguments that appear before the image name.

While the container is running sackman will record all accessed file paths.
Finally a copy of the origial image will be created containing only the accessed files.

You can configure sackman with the following environment variables:

    DST_IMAGE - The name of the final image.
    RSYNC_FILES - A path to a file containing additional files to copy.
    RSYNC_OPTS - Can be set to "-v" to output all copied files.

## Installation

    $ make
    $ sudo make install
or

    $ PREFIX=$HOME/.local make -e install

sackman has the following dependencies:

    podman
    buildah
    fuse
    rsync

You also need the usual build tools to compile sackman.

Install fuse on Fedora with:

    dnf install fuse-devel

Install fuse on Depian/Ubuntu with:

    apt install libfuse-dev
