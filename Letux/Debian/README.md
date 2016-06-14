# This is a deb build tree for the Linux Kernel, utilizing debhelper.

## Cross-Compiling for armhf:
1.  Install Cross-Compiler for armhf from emdebian:
    [https://wiki.debian.org/CrossToolchains#Installation](https://wiki.debian.org/CrossToolchains#Installation)
2.  install build dependencies:
    `sudo apt-get install debhelper bc lzop lzop:armhf`
3.  Get the Code:
    `git clone https://github.com/mxOBS/deb-pkg_kernel-xyz.git`
    `cd deb-pkg_kernel-xyz; git submodule update --init`
4.  build deb:
    `cd deb-pkg_kernel-xyz; dpkg-buildpackage -a armhf -b`

## Building natively on armhf:
1.  install build dependencies:
    `sudo apt-get install debhelper bc lzop`
2.  Get the Code:
    `git clone https://github.com/mxOBS/deb-pkg_kernel-xyz.git`
    `cd deb-pkg_kernel-xyz; git submodule update --init`
3.  build deb:
    `cd deb-pkg_kernel-xyz; dpkg-buildpackage -a armhf -b`
