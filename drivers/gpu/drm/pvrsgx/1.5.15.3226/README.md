Intel® Embedded Media and Graphics Driver (Intel® EMGD) 1.18
============================================================

Hosting the binaries and source code of Intel(R)'s EMGD driver here to collect patches for these drivers here.
Feel free to fork and send patches.

If you find more binaries also feel free to send them with a pull request.


Quick start guide (no VA-API support)
----------------------------------

1. Add this PPA: https://launchpad.net/~thopiekar/+archive/emgd 

```
sudo add-apt-repository lp:~thopiekar/emgd
```

      Packages hosted there say that their version is 1.16, but they include 1.18!


2. Install the emgd driver ("emgd-drm-dkms" and "xserver-xorg-1.9-video-emgd" are minimum)

3. Create a xorg.conf on your own xorg.conf 

    3.0 (emgd-xorg-conf which I wrote in the past does not work with EMGD 1.10+ [at least on my Asus T91])

    3.1 You can find templetes at /usr/share/emgd-driver/xorg.conf/ or https://github.com/EMGD-Community/intel-binaries-linux/tree/master/config/xorg.conf

    3.2 don't forget to set there your resolution and color depth


Status
------

feature       | status
------------- | -------------
2D Accel      | works very well
OpenGL        | works, but some applications/libraries might depend on a newer API, so you get freezes or seg. faults
OpenGL ES     | works, but unstable (completly provided by EMGD)
VA-API        | broken/unknown - testing is needed here

Kernel
------

version (mainline)  | status
------------------- | -----------------------------------------------
3.1                 | should work (builds against it, but not tested)
3.2                 | should work (builds against it, but not tested)
3.3                 | should work (builds against it, but not tested)
3.4                 | should work (builds against it, but not tested)
3.5                 | should work (builds against it, but not tested)
3.6                 | should work (builds against it, but not tested)
3.7                 | should work (builds against it, but not tested)
3.8                 | should work (builds against it, but not tested)
3.9                 | should work (builds against it, but not tested)
3.10                | should work (builds against it, but not tested)
3.11                | works (incl. backlight, suspend)
3.12                | should work (builds against it, but not tested)
3.13                | works (Ubuntu Trusty)
3.14                | should work (builds against it, but not tested)
3.15                | works (Ubuntu Trusty using mainline kernel)
3.16                | should work (builds against it, but not tested)
3.17                | should work (builds against it, but not tested)
3.18                | should work (builds against it, but not tested)
3.19                | works (Ubuntu Vivid using Ubuntu kernel)
4.0                 | should work (builds against it, but not tested)

Xserver requirements
--------------------
https://github.com/EMGD-Community/xserver-xorg

distribution  | Xserver video ABI
------------- | -----------------
fedora14      | 9  (Xserver  9)
meego1.2      | 9  (Xserver  9)
meego-wayland | 10 (Xserver 10)
tizen1.0      | 9  (Xserver  9)

Xserver requirements
--------------------
https://github.com/EMGD-Community/mesa

version  | status
-------- | -------------
7.9      | works
8.1      | used in Meego - never released really - untested
9.0-rc   | used in Tizen IVI - untested


libVA requirements
------------------
https://github.com/EMGD-Community/libva

version          | status                                                                   | build recipe 
---------------- | -------------------------------------------------------------------------|---------------
1.0.10           | build passes, but encoding broken (tested on vainfo)                     | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.10-emgd-daily
1.0.11           | build passes, but encoding broken (tested on vainfo)                     | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.11-emgd-daily
1.0.12           | build passes and works                                                   | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.12-emgd-daily
1.0.13           | build passes and works                                                   | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.13-emgd-daily
1.0.14           | build does not pass                                                      | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.14-emgd-daily
1.0.15           | build does not pass                                                      | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.15-emgd-daily
1.0.16           | build passes and works                                                   | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.16-emgd-daily
1.0.17           | build passes, but not tested                                             | https://code.launchpad.net/~thopiekar/+recipe/libva-1.0.17-emgd-daily
1.1.x (upstream) | broken (tested on vainfo - get seg.fault)                                | 
1.2.0 (Tizen IVI)| build passes, but not tested                                             | 
1.2.1 (upstream) | build passes, but not tested                                             | 

application      | status 
---------------- | -------------------------
VideoLAN client  | raises segmentation fault
XBMC             | works
MPlayer          | untested

GL/GLes info
------------

```
$ glxinfo
name of display: :0
display: :0  screen: 0
direct rendering: Yes
server glx vendor string: SGI
server glx version string: 1.4
server glx extensions:
    GLX_ARB_multisample, GLX_EXT_import_context, GLX_EXT_texture_from_pixmap, 
    GLX_EXT_visual_info, GLX_EXT_visual_rating, GLX_INTEL_swap_event, 
    GLX_MESA_copy_sub_buffer, GLX_OML_swap_method, GLX_SGIS_multisample, 
    GLX_SGIX_fbconfig, GLX_SGIX_pbuffer, GLX_SGIX_visual_select_group
client glx vendor string: Mesa Project and SGI
client glx version string: 1.4
client glx extensions:
    GLX_ARB_get_proc_address, GLX_ARB_multisample, GLX_EXT_import_context, 
    GLX_EXT_texture_from_pixmap, GLX_EXT_visual_info, GLX_EXT_visual_rating, 
    GLX_INTEL_swap_event, GLX_MESA_copy_sub_buffer, GLX_MESA_swap_control, 
    GLX_OML_swap_method, GLX_OML_sync_control, GLX_SGIS_multisample, 
    GLX_SGIX_fbconfig, GLX_SGIX_pbuffer, GLX_SGIX_visual_select_group, 
    GLX_SGI_make_current_read, GLX_SGI_swap_control, GLX_SGI_video_sync
GLX version: 1.4
GLX extensions:
    GLX_ARB_get_proc_address, GLX_ARB_multisample, GLX_EXT_import_context, 
    GLX_EXT_texture_from_pixmap, GLX_EXT_visual_info, GLX_EXT_visual_rating, 
    GLX_INTEL_swap_event, GLX_MESA_copy_sub_buffer, GLX_MESA_swap_control, 
    GLX_OML_swap_method, GLX_OML_sync_control, GLX_SGIS_multisample, 
    GLX_SGIX_fbconfig, GLX_SGIX_pbuffer, GLX_SGIX_visual_select_group, 
    GLX_SGI_make_current_read, GLX_SGI_video_sync
OpenGL vendor string: Intel Corporation
OpenGL renderer string: EMGD on PowerVR SGX535
OpenGL version string: 2.1
OpenGL shading language version string: 1.20
OpenGL extensions:
    GL_ARB_depth_texture, GL_ARB_draw_buffers, GL_ARB_fragment_program, 
    GL_ARB_fragment_shader, GL_ARB_half_float_pixel, GL_ARB_matrix_palette, 
    GL_ARB_multisample, GL_ARB_multitexture, GL_ARB_occlusion_query, 
    GL_ARB_pixel_buffer_object, GL_ARB_point_parameters, GL_ARB_point_sprite, 
    GL_ARB_shader_objects, GL_ARB_shading_language_100, GL_ARB_shadow, 
    GL_ARB_shadow_ambient, GL_ARB_texture_border_clamp, 
    GL_ARB_texture_compression, GL_ARB_texture_cube_map, 
    GL_ARB_texture_env_add, GL_ARB_texture_env_combine, 
    GL_ARB_texture_env_crossbar, GL_ARB_texture_env_dot3, 
    GL_ARB_texture_float, GL_ARB_texture_mirrored_repeat, 
    GL_ARB_texture_non_power_of_two, GL_ARB_texture_rectangle, 
    GL_ARB_transpose_matrix, GL_ARB_vertex_blend, GL_ARB_vertex_buffer_object, 
    GL_ARB_vertex_program, GL_ARB_vertex_shader, GL_ARB_window_pos, 
    GL_EXT_abgr, GL_EXT_bgra, GL_EXT_blend_color, 
    GL_EXT_blend_equation_separate, GL_EXT_blend_func_separate, 
    GL_EXT_blend_minmax, GL_EXT_blend_subtract, GL_EXT_compiled_vertex_array, 
    GL_EXT_draw_range_elements, GL_EXT_fog_coord, GL_EXT_framebuffer_blit, 
    GL_EXT_framebuffer_object, GL_EXT_multi_draw_arrays, GL_EXT_packed_pixels, 
    GL_EXT_rescale_normal, GL_EXT_secondary_color, 
    GL_EXT_separate_specular_color, GL_EXT_shadow_funcs, 
    GL_EXT_stencil_two_side, GL_EXT_stencil_wrap, GL_EXT_texture3D, 
    GL_EXT_texture_compression_s3tc, GL_EXT_texture_cube_map, 
    GL_EXT_texture_edge_clamp, GL_EXT_texture_env_add, 
    GL_EXT_texture_env_combine, GL_EXT_texture_env_dot3, 
    GL_EXT_texture_filter_anisotropic, GL_EXT_texture_lod_bias, 
    GL_EXT_texture_object, GL_EXT_vertex_array, 
    GL_IMG_texture_compression_pvrtc, GL_NV_blend_square, 
    GL_NV_texgen_reflection, GL_NV_texture_rectangle, GL_S3_s3tc, 
    GL_SGIS_generate_mipmap

16 GLX Visuals
    visual  x   bf lv rg d st  colorbuffer  sr ax dp st accumbuffer  ms  cav
  id dep cl sp  sz l  ci b ro  r  g  b  a F gb bf th cl  r  g  b  a ns b eat
----------------------------------------------------------------------------
0x021 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x022 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x055 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x056 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x057 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x058 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x059 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x05a 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x05b 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x05c 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x05d 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x05e 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x05f 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x060 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x061 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x044 32 tc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None

16 GLXFBConfigs:
    visual  x   bf lv rg d st  colorbuffer  sr ax dp st accumbuffer  ms  cav
  id dep cl sp  sz l  ci b ro  r  g  b  a F gb bf th cl  r  g  b  a ns b eat
----------------------------------------------------------------------------
0x045 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x046 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x047 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x048 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x049 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x04a 24 tc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x04b 24 tc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x04c 32 tc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x04d 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x04e 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x04f 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  0 0 None
0x050 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0  0  0  0  0  0  0  4 1 None
0x051 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x052 24 dc  0  32  0 r  . .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
0x053 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  0 0 None
0x054 24 dc  0  32  0 r  y .   8  8  8  8 .  .  0 24  8  0  0  0  0  4 1 None
```
```
$ es2_info
EGL_VERSION: 1.4 build 1.5.15.3226
EGL_VENDOR: Intel Corporation
EGL_EXTENSIONS:
    EGL_KHR_image, EGL_KHR_image_base, EGL_KHR_image_pixmap, 
    EGL_KHR_gl_texture_2D_image, EGL_KHR_gl_texture_cubemap_image, 
    EGL_KHR_gl_renderbuffer_image, EGL_KHR_vg_parent_image, 
    EGL_WL_bind_wayland_display, EGL_KHR_surfaceless_gles2, 
    EGL_KHR_surfaceless_opengl, EGL_NOK_image_shared
EGL_CLIENT_APIS: OpenGL_ES OpenVG 
GL_VERSION: OpenGL ES 2.0
GL_RENDERER: EMGD on PowerVR SGX 535
GL_EXTENSIONS:
    GL_OES_rgb8_rgba8, GL_OES_depth24, GL_OES_vertex_half_float, 
    GL_OES_texture_float, GL_OES_texture_half_float, 
    GL_OES_element_index_uint, GL_OES_mapbuffer, 
    GL_OES_fragment_precision_high, GL_OES_compressed_ETC1_RGB8_texture, 
    GL_OES_EGL_image, GL_OES_required_internalformat, GL_OES_depth_texture, 
    GL_OES_get_program_binary, GL_OES_packed_depth_stencil, 
    GL_OES_standard_derivatives, GL_OES_vertex_array_object, 
    GL_EXT_multi_draw_arrays, GL_EXT_texture_format_BGRA8888, 
    GL_EXT_discard_framebuffer, GL_IMG_shader_binary, 
    GL_IMG_texture_compression_pvrtc, GL_IMG_texture_stream2, 
    GL_IMG_texture_npot, GL_OES_texture_npot, GL_IMG_texture_format_BGRA8888, 
    GL_IMG_read_format, GL_IMG_program_binary, 
    GL_EXT_multisampled_render_to_texture
```

Benchmarks
----------

```
$ glxgears 
Running synchronized to the vertical refresh.  The framerate should be
approximately the same as the monitor refresh rate.
2143 frames in 5.0 seconds = 428.570 FPS [upper right corner]
2108 frames in 5.0 seconds = 421.590 FPS
2369 frames in 5.0 seconds = 473.727 FPS [upper left corner]
2370 frames in 5.0 seconds = 473.961 FPS
2826 frames in 5.0 seconds = 565.103 FPS [lower right corner]
3466 frames in 5.0 seconds = 693.165 FPS
3418 frames in 5.0 seconds = 683.148 FPS
2236 frames in 5.0 seconds = 447.150 FPS [lower left corner]
2309 frames in 5.0 seconds = 461.613 FPS
```
```
$ es2gears
EGL_VERSION = 1.4 build 1.5.15.3226
vertex shader info: Success.

fragment shader info: Success.

info: 
1361 frames in 5.0 seconds = 272.037 FPS [upper right corner]
1493 frames in 5.0 seconds = 298.600 FPS
1433 frames in 5.0 seconds = 286.600 FPS [upper left corner]
1449 frames in 5.0 seconds = 289.568 FPS
1455 frames in 5.0 seconds = 291.000 FPS 
1714 frames in 5.0 seconds = 342.731 FPS [lower right corner]
1771 frames in 5.0 seconds = 353.988 FPS
1499 frames in 5.0 seconds = 299.800 FPS [lower left corner]
1496 frames in 5.0 seconds = 298.961 FPS
```
```
$ glmark2
=======================================================
    glmark2 2012.08
=======================================================
    OpenGL Information
    GL_VENDOR:     Intel Corporation
    GL_RENDERER:   EMGD on PowerVR SGX535
    GL_VERSION:    2.1
=======================================================
[build] use-vbo=false: FPS: 109 FrameTime: 9.174 ms
=======================================================
                                  glmark2 Score: 109 
=======================================================
```
```
$ glmark2
=======================================================
    glmark2 2012.08
=======================================================
    OpenGL Information
    GL_VENDOR:     Intel Corporation
    GL_RENDERER:   EMGD on PowerVR SGX535
    GL_VERSION:    2.1
=======================================================
[build] use-vbo=false: FPS: 118 FrameTime: 8.475 ms
[build] use-vbo=true: FPS: 125 FrameTime: 8.000 ms
[texture] texture-filter=nearest: FPS: 142 FrameTime: 7.042 ms
[texture] texture-filter=linear: FPS: 143 FrameTime: 6.993 ms
[texture] texture-filter=mipmap: FPS: 143 FrameTime: 6.993 ms
[shading] shading=gouraud: FPS: 98 FrameTime: 10.204 ms
[shading] shading=blinn-phong-inf: FPS: 90 FrameTime: 11.111 ms
[shading] shading=phong: FPS: 56 FrameTime: 17.857 ms
[bump] bump-render=high-poly: FPS: 45 FrameTime: 22.222 ms
[bump] bump-render=normals: FPS: 104 FrameTime: 9.615 ms
[bump] bump-render=height: FPS: 87 FrameTime: 11.494 ms
[effect2d] kernel=0,1,0;1,-4,1;0,1,0;: FPS: 20 FrameTime: 50.000 ms
[effect2d] kernel=1,1,1,1,1;1,1,1,1,1;1,1,1,1,1;: FPS: 5 FrameTime: 200.000 ms
[pulsar] light=false:quads=5:texture=false: FPS: 141 FrameTime: 7.092 ms
[desktop] blur-radius=5:effect=blur:passes=1:separable=true:windows=4: FPS: 7 FrameTime: 142.857 ms
[desktop] effect=shadow:windows=4: FPS: 8 FrameTime: 125.000 ms
[buffer] columns=200:interleave=false:update-dispersion=0.9:update-fraction=0.5:update-method=map: FPS: 49 FrameTime: 20.408 ms
[buffer] columns=200:interleave=false:update-dispersion=0.9:update-fraction=0.5:update-method=subdata: FPS: 48 FrameTime: 20.833 ms
[buffer] columns=200:interleave=true:update-dispersion=0.9:update-fraction=0.5:update-method=map: FPS: 51 FrameTime: 19.608 ms
[ideas] speed=duration: FPS: 83 FrameTime: 12.048 ms
[jellyfish] <default>: FPS: 17 FrameTime: 58.824 ms
[terrain] <default>: FPS: 0 FrameTime: inf ms
[conditionals] fragment-steps=0:vertex-steps=0: FPS: 136 FrameTime: 7.353 ms
[conditionals] fragment-steps=5:vertex-steps=0: FPS: 33 FrameTime: 30.303 ms
[conditionals] fragment-steps=0:vertex-steps=5: FPS: 128 FrameTime: 7.812 ms
[function] fragment-complexity=low:fragment-steps=5: FPS: 82 FrameTime: 12.195 ms
[function] fragment-complexity=medium:fragment-steps=5: FPS: 40 FrameTime: 25.000 ms
[loop] fragment-loop=false:fragment-steps=5:vertex-steps=5: FPS: 71 FrameTime: 14.085 ms
[loop] fragment-steps=5:fragment-uniform=false:vertex-steps=5: FPS: 16 FrameTime: 62.500 ms
[loop] fragment-steps=5:fragment-uniform=true:vertex-steps=5: FPS: 11 FrameTime: 90.909 ms
=======================================================
                                  glmark2 Score: 69 
=======================================================
```
```
$ glmemperf 
GLMemPerf v0.13 - OpenGL ES 2.0 memory performance benchmark
Copyright (C) 2010 Nokia Corporation. GLMemPerf comes with ABSOLUTELY
NO WARRANTY; This is free software, and you are welcome to redistribute
it under certain conditions.

clear:                                   75 fps #########################
blit_tex_rgba8888_800x480:               73 fps #########################
blit_tex_rgb888_800x480:                 73 fps #########################
blit_tex_rgba8888_864x480:               73 fps #########################
blit_tex_rgb888_864x480:                 73 fps #########################
blit_tex_rgba8888_1024x512:              71 fps ########################
blit_tex_rgb565_800x480:                 75 fps #########################
blit_tex_rgb565_1024x512:                74 fps #########################
blit_tex_rgb_pvrtc4_1024x512:            76 fps ##########################
blit_tex_rgb_pvrtc2_1024x512:            77 fps ##########################
blit_tex_rgb_etc1_1024x512:              76 fps ##########################
blit_tex_r8_800x480:                     76 fps ##########################
blit_tex_r8_1024x512:                    76 fps ##########################
blit_pixmap_32bpp_800x480:               70 fps ########################
blit_pixmap_32bpp_800x480:               70 fps ########################
blit_fbo_rgba8888_800x480:               73 fps #########################
blit_fbo_rgba8888_1024x512:              71 fps ########################
blit_fbo_rgb565_800x480:                 75 fps #########################
blit_fbo_rgb565_1024x512:                75 fps #########################
blit_tex_rot90_rgba8888_480x800:         72 fps ########################
blit_tex_rot90_rgb888_480x800:           72 fps ########################
blit_tex_rot90_rgba8888_480x864:         71 fps ########################
blit_tex_rot90_rgb888_480x864:           71 fps ########################
blit_tex_rot90_rgba8888_512x1024:        73 fps #########################
blit_tex_rot90_rgb565_480x800:           74 fps #########################
blit_tex_rot90_rgb565_512x1024:          74 fps #########################
blit_tex_rot90_rgb_pvrtc4_512x1024:      77 fps ##########################
blit_tex_rot90_rgb_pvrtc2_512x1024:      77 fps ##########################
blit_tex_rot90_rgb_etc1_512x1024:        76 fps ##########################
blit_pixmap_rot90_32bpp_480x800:         67 fps #######################
blit_pixmap_rot90_32bpp_480x800:         67 fps #######################
blit_fbo_rot90_rgba8888_800x480:         72 fps ########################
blit_fbo_rot90_rgba8888_1024x512:        73 fps #########################
blit_fbo_rot90_rgb565_800x480:           74 fps #########################
blit_fbo_rot90_rgb565_1024x512:          75 fps #########################
blit_tex_rgba4444_128x128:               76 fps ##########################
blit_tex_rgba8888_127x127:               75 fps #########################
blit_tex_rgba8888_128x128:               75 fps #########################
blit_tex_rgb565_127x127:                 77 fps ##########################
blit_tex_rgb565_128x128:                 77 fps ##########################
blit_tex_rgba_pvrtc4_128x128:            76 fps ##########################
blit_tex_rgba_pvrtc2_128x128:            78 fps ##########################
blit_tex_rgb_etc1_128x128:               77 fps ##########################
blit_shader_mask_128x128:                76 fps ##########################
blit_tex_rot90_rgba8888_127x127:         73 fps #########################
blit_tex_rot90_rgba8888_128x128:         75 fps #########################
blit_tex_rot90_rgb565_127x127:           75 fps #########################
blit_tex_rot90_rgb565_128x128:           76 fps ##########################
blit_tex_rot90_rgba_pvrtc4_128x128:      77 fps ##########################
blit_tex_rot90_rgba_pvrtc2_128x128:      78 fps ##########################
blit_tex_rot90_rgb_etc1_128x128:         77 fps ##########################
blit_shader_const_800x480:               77 fps ##########################
blit_shader_lingrad_800x480:             74 fps #########################
blit_shader_radgrad_800x480:             74 fps #########################
blit_shader_palette_800x480:             71 fps ########################
blit_shader_blur_800x480:                48 fps ################
blit_cpu_shmimage_16bpp_2x1024x600:     Assertion failed: visualCount > 0
blit_cpu_shmimage_32bpp_2x1024x600:      51 fps #################
blit_cpu_texupload_16bpp_2x1024x600:     57 fps ###################
blit_cpu_texupload_32bpp_2x1024x600:     46 fps ################
blit_cpu_locksurf_16bpp_2x1024x600:     EGL_KHR_lock_surface2 not supported
blit_cpu_locksurf_32bpp_2x1024x600:     EGL_KHR_lock_surface2 not supported
```
