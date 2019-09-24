# sackman

## Description

sackman helps you reducing the size of your containers to a minimum.

The C code is based on Remi Flament's LoggedFS (https://github.com/rflament/loggedfs).

## Installation

    $ make
    $ sudo make install
or

    $ PREFIX=$HOME/.local make -e install

sackman has the following dependencies:

    fuse

## Usage example

    $ DST_IMAGE=test sackman run docker.io/library/ubuntu:14.04 cat /etc/lsb-release
