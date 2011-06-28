#!/bin/bash

# copies the ucm configuration files /usr/share/alsa/ucm to here to be used
# before a git commit and push

# needs to be run as root
cp -ap /usr/share/alsa/ucm/* .

