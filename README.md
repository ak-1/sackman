# sackman

## Description

sackman helps you reducing the size of your containers to a minimum.

The C code is based on Remi Flament's LoggedFS (https://github.com/rflament/loggedfs).

## Usage example

Call sackman like you would call podman run:

    $ DST_IMAGE=test sackman run docker.io/library/ubuntu:14.04 cat /etc/lsb-release

While the container is running sackman will record all accessed file paths.
Finally a copy of the origial image will be created containing only the accessed files.

You can control the final image name via the DST_IMAGE environment variable.

## Installation

    $ make
    $ sudo make install
or

    $ PREFIX=$HOME/.local make -e install

sackman has the following dependencies:

    fuse
