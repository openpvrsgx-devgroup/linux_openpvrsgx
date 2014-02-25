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

kernel version       | status
-------------------- | -------------
3.11                 | works incl. backlight
3.12                 | should work (builds against it, but not tested)
3.13                 | should work (builds against it, but not tested)

VA-API version   | status
---------------- | ------
1.0.15           | works (tested on vainfo)
1.0.16           | needs testing/packaging [version suggested by yoctoproject]
1.0.17           | broken (tested on vainfo - get seg.fault)
1.1.x (upstream) | broken (tested on vainfo - get seg.fault)



Quick start guide (OpenGL support)
----------------------------------

1. Add this PPA: https://launchpad.net/~thopiekar/+archive/emgd

2. Install the emgd driver ("emgd-drm-dkms" and "xserver-xorg-1.9-video-emgd" are minimum)

3. Prepare the emgd module to be loaded

    3.1 Create a file in /etc/modprobe.d/ (e.g. blacklist-emgd.conf) and fill it with the following content

    ```
    blacklist psb_gfx
    ```

    3.2 Add "emgd" to /etc/modules as it is not automaticly loaded at boot-time (fix for that is welcome!)

4. Create a xorg.conf on your own xorg.conf 

    4.1 (emgd-xorg-conf which I wrote in the past does not work with EMGD 1.10+ [at least on my Asus T91])

    4.2 You can find templetes at /usr/share/doc/emgd/emgd-[cb/rv].conf

    4.3 don't forget to set there your resolution and color depth

4. Make sure Xserver-xorg v1.9 and mesa v7.9 are properly installed
