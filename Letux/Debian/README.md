# This is a deb build tree for the Linux Kernel, utilizing debhelper.

## Cross-Compiling for armhf:
1.  Install Cross-Compiler for armhf from emdebian:
    [https://wiki.debian.org/CrossToolchains#Installation](https://wiki.debian.org/CrossToolchains#Installation)
2.  install build dependencies:
    `sudo apt-get install debhelper bc lzop u-boot-tools lzop:armhf`
3.  Get the Code:
    `git clone git://git.goldelico.com/qtmoko2-kernel.git`
    `cd qtmoko2-kernel; git submodule update --init`
4.  build deb:
    `cd qtmoko2-kernel; dpkg-buildpackage -a armhf -b`

## Building natively on armhf:
1.  install build dependencies:
    `sudo apt-get install debhelper bc lzop u-boot-tools`
2.  Get the Code:
    `git clone git://git.goldelico.com/qtmoko2-kernel.git`
    `cd qtmoko2-kernel; git submodule update --init`
3.  build deb:
    `cd qtmoko2-kernel; dpkg-buildpackage -a armhf -b`
