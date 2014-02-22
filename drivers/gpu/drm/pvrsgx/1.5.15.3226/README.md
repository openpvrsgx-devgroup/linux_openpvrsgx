Intel® Embedded Media and Graphics Driver (Intel® EMGD)
=======================================================

Hosting the binaries and source code of Intel(R)'s EMGD driver here to collect patches for these drivers here.
Feel free to fork and send patches.

If you find more binaries also feel free to send them with a pull request.

Status
------

              | feature
------------- | -------------
2D Accel      | works (MeeGO binaries, others untested)
OpenGL        | works (MeeGO binaries, others untested)
OpenGL ES     | broken
VA-API        | needs 1.0.x, but packages depend on newer 1.1.x

version       | status
------------- | -------------
3.11          | works incl. backlight
3.12          | should work (builds against it, but not tested)
3.13          | should work (builds against it, but not tested)

Quick start guide
-----------------

1. Add this PPA: https://launchpad.net/~thopiekar/+archive/emgd

2. Install the emgd driver (emgd-drm-dkms and xorg-module are minimum)

3. create a xorg.conf on your own xorg.conf 

    3.1 (emgd-xorg-conf which I wrote in the past does not work with EMGD 1.10+ [at least on my Asus T91])

    3.2 You can find templetes at /usr/share/doc/emgd/emgd-[cb/rv].conf

    3.3 don't forget to set there your resolution and color depth

4. Make sure Xserver-xorg v1.9 and mesa v7.9 are properly installed
