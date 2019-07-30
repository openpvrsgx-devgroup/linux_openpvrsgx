/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This file contains a list of Good Display epaper display model numbers
 * listing their color mode (B/W, B/W/Red, B/W/Yellow) as well as their physical
 * dimensions and resolution.
 *
 * This data was manually extracted from the displays' specifications pages on
 * the mfg website.
 *
 * The following displays use third-party driver chips with command sets this
 * driver can't handle:
 *   GDEH0154D67
 *   GDEH0213B73
 *   GDE060B3
 *   GDE060F3
 *   GDE060BA, GDE060BAT, GDE060BAFL GDE060BAFLT
 *
 * To add a display, insert the appropriate descriptor line below, then link it
 * into the table at the bottom of this file. This driver should handle any
 * Good Display driver chip (IL*, GD*) with minor modifications.
 *
 * Copyright 2019 Jan Sebastian Goette
 */

#define DEF_EPD_TYPE(type, col, w_mm, h_mm, w_px, h_px) \
	static const struct gdepaper_type_descriptor gddsp_ ## type ## _data = \
	{col, w_mm, h_mm, w_px, h_px}

DEF_EPD_TYPE(gdew0154z04t,    GDEPAPER_COL_BW_RED,     28,  28,  200, 200); /*    1.54"    b/w/r  IL0376F     */
DEF_EPD_TYPE(gdew0154z04fl,   GDEPAPER_COL_BW_RED,     28,  28,  200, 200); /*    1.54"    b/w/r  IL0376F     */
DEF_EPD_TYPE(gdew0154z04,     GDEPAPER_COL_BW_RED,     28,  28,  200, 200); /*    1.54"    b/w/r  IL0376F     */
DEF_EPD_TYPE(gdewl0154z17fl,  GDEPAPER_COL_BW_RED,     28,  28,  152, 152); /*    1.54"    b/w/r  IL0373      */
DEF_EPD_TYPE(gdew0154z17,     GDEPAPER_COL_BW_RED,     28,  28,  152, 152); /*    1.54"    b/w/r  IL0373      */
DEF_EPD_TYPE(gdew0154c39fl,   GDEPAPER_COL_BW_YELLOW,  28,  28,  152, 152); /*    1.54"    b/w/y              */
DEF_EPD_TYPE(gdew0154c39,     GDEPAPER_COL_BW_YELLOW,  28,  28,  152, 152); /*    1.54"    b/w/y              */
DEF_EPD_TYPE(gdew0213z16,     GDEPAPER_COL_BW_RED,     49,  24,  212, 104); /*    2.13"    b/w/r  IL0373      */
DEF_EPD_TYPE(gdew0213c38,     GDEPAPER_COL_BW_YELLOW,  49,  24,  212, 104); /*    2.13"    b/w/y  IL0373      */
DEF_EPD_TYPE(gdew026z39,      GDEPAPER_COL_BW_RED,     60,  31,  296, 152); /*    2.6"     b/w/r  IL0373      */
DEF_EPD_TYPE(gdew027c44t,     GDEPAPER_COL_BW_RED,     57,  38,  264, 176); /*    2.7"     b/w/r  IL91874     */
DEF_EPD_TYPE(gdew027c44,      GDEPAPER_COL_BW_RED,     57,  38,  264, 176); /*    2.7"     b/w/r  IL91874     */
DEF_EPD_TYPE(gdew029z10,      GDEPAPER_COL_BW_RED,     67,  29,  296, 128); /*    2.9"     b/w/r  IL0373      */
DEF_EPD_TYPE(gdew029c32,      GDEPAPER_COL_BW_YELLOW,  67,  29,  296, 128); /*    2.9"     b/w/y  IL0373      */
DEF_EPD_TYPE(gdew0371z80,     GDEPAPER_COL_BW_RED,     82,  47,  416, 240); /*    3.71"    b/w/r  GD8102      */
DEF_EPD_TYPE(gdew042z15,      GDEPAPER_COL_BW_RED,     85,  64,  400, 300); /*    4.2"     b/w/r  IL0398      */
DEF_EPD_TYPE(gdew042c37,      GDEPAPER_COL_BW_YELLOW,  85,  64,  400, 300); /*    4.2"     b/w/y  IL0398      */
DEF_EPD_TYPE(gdew0583c64,     GDEPAPER_COL_BW_YELLOW, 119,  88,  600, 448); /*    5.83"    b/w/y  IL0371      */
DEF_EPD_TYPE(gdew0583z21,     GDEPAPER_COL_BW_RED,    119,  88,  600, 448); /*    5.83"    b/w/r  IL0371      */
DEF_EPD_TYPE(gdew075z08,      GDEPAPER_COL_BW_RED,    163,  98,  800, 480); /*    7.5"     b/w/r  GD7965      */
DEF_EPD_TYPE(gdew075z09,      GDEPAPER_COL_BW_RED,    163,  98,  640, 384); /*    7.5"     b/w/r  IL0371      */
DEF_EPD_TYPE(gdew075c21,      GDEPAPER_COL_BW_YELLOW, 163,  98,  640, 384); /*    7.5"     b/w/y  IL0371      */
/* These are not yet supported since they use multiple driver chips in cascade mode */
/* DEF_EPD_TYPE(gdew1248z95,     GDEPAPER_COL_BW_RED,    253, 191, 1304, 984); */ /*   12.48"    b/w/r  IL0326 (4x) */
/* DEF_EPD_TYPE(gdew1248c63,     GDEPAPER_COL_BW_YELLOW, 253, 191, 1304, 984); */ /*   12.48"    b/w/y  IL0326 (4x) */
/* DEF_EPD_TYPE(gdew1248t3,      GDEPAPER_COL_BW,        253, 191, 1304, 984); */ /*   12.48"    b/w    IL0326 (4x) */
DEF_EPD_TYPE(gdew0102i4f,     GDEPAPER_COL_BW,         22,  14,  128,  80); /*    1.02"    b/w    IL0323      */
DEF_EPD_TYPE(gdew0102i4fc,    GDEPAPER_COL_BW,         22,  14,  128,  80); /*    1.02"    b/w                */
DEF_EPD_TYPE(gdep014tt1,      GDEPAPER_COL_BW,         14,  33,  128, 296); /*    1.43"    b/w                */
DEF_EPD_TYPE(gdep015oc1,      GDEPAPER_COL_BW,         28,  28,  200, 200); /*    1.54"    b/w    IL3829      */
DEF_EPD_TYPE(gdew0154t8t,     GDEPAPER_COL_BW,         28,  28,  152, 152); /*    1.54"    b/w    IL0373      */
DEF_EPD_TYPE(gdew0154t8fl,    GDEPAPER_COL_BW,         28,  28,  152, 152); /*    1.54"    b/w    IL0373      */
DEF_EPD_TYPE(gdew0154t8,      GDEPAPER_COL_BW,         28,  28,  152, 152); /*    1.54"    b/w    IL0373      */
DEF_EPD_TYPE(gdeh0154d27t,    GDEPAPER_COL_BW,         28,  28,  200, 200); /*    1.54"    b/w                */
DEF_EPD_TYPE(gdem0154e97lt,   GDEPAPER_COL_BW,         28,  28,  152, 152); /*    1.54"    b/w    IL3895      */
DEF_EPD_TYPE(gdew0154i9f,     GDEPAPER_COL_BW,         28,  28,  152, 152); /*    1.54"    b/w                */
DEF_EPD_TYPE(gdew0213t5,      GDEPAPER_COL_BW,         24,  49,  212, 104); /*    2.13"    b/w    IL0373      */
DEF_EPD_TYPE(gdew0213i5f,     GDEPAPER_COL_BW,         49,  24,  212, 104); /*    2.13"    b/w                */
DEF_EPD_TYPE(gdeh0213d30lt,   GDEPAPER_COL_BW,         23,  48,  212, 104); /*    2.13"    b/w    IL3820      */
DEF_EPD_TYPE(gdew0213v7lt,    GDEPAPER_COL_BW,         24,  49,  212, 104); /*    2.13"    b/w    IL0373      */
DEF_EPD_TYPE(gdth0213zhft34,  GDEPAPER_COL_BW,         24,  49,  250, 122); /*    2.13"    b/w                */
DEF_EPD_TYPE(gdew026t0,       GDEPAPER_COL_BW,         60,  31,  296, 152); /*    2.6"     b/w    IL0373      */
DEF_EPD_TYPE(gdew027w3t,      GDEPAPER_COL_BW,         57,  38,  264, 176); /*    2.7"     b/w    IL91874     */
DEF_EPD_TYPE(gdew027w3,       GDEPAPER_COL_BW,         57,  38,  264, 176); /*    2.7"     b/w    IL91874     */
DEF_EPD_TYPE(gdeh029a1,       GDEPAPER_COL_BW,         67,  29,  296, 128); /*    2.9"     b/w    IL3820      */
DEF_EPD_TYPE(gdew029t5,       GDEPAPER_COL_BW,         67,  29,  296, 128); /*    2.9"     b/w    IL0373      */
DEF_EPD_TYPE(gdeh029d56lt,    GDEPAPER_COL_BW,         67,  29,  296, 128); /*    2.9"     b/w                */
DEF_EPD_TYPE(gdew029i6f,      GDEPAPER_COL_BW,         67,  29,  296, 128); /*    2.9"     b/w                */
DEF_EPD_TYPE(gdew0371w7,      GDEPAPER_COL_BW,         82,  47,  416, 240); /*    3.71"    b/w    GP8102      */
DEF_EPD_TYPE(gdew042t2,       GDEPAPER_COL_BW,         83,  64,  400, 300); /*    4.2"     b/w    IL0398      */
DEF_EPD_TYPE(gdep043zf3,      GDEPAPER_COL_BW,         56,  94,  800, 480); /*    4.3"     b/w                */
DEF_EPD_TYPE(gde043a2t,       GDEPAPER_COL_BW,         88,  66,  800, 600); /*    4.3"     b/w                */
DEF_EPD_TYPE(gde043a2,        GDEPAPER_COL_BW,         88,  66,  800, 600); /*    4.3"     b/w                */
DEF_EPD_TYPE(gdew0583t7,      GDEPAPER_COL_BW,        119,  88,  600, 448); /*    5.83"    b/w    IL0371      */
DEF_EPD_TYPE(gdew075t8,       GDEPAPER_COL_BW,        163,  98,  640, 384); /*    7.5"     b/w    IL0371      */
DEF_EPD_TYPE(gdew075t7,       GDEPAPER_COL_BW,        163,  98,  800, 480); /*    7.5"     b/w    GD7965      */
DEF_EPD_TYPE(gdew080t5,       GDEPAPER_COL_BW,        122, 163, 1024, 768); /*    8"       b/w                */
DEF_EPD_TYPE(gdep097tc2,      GDEPAPER_COL_BW,        203, 139, 1200, 825); /*    9.7"     b/w                */

#undef DEF_EPD_TYPE

#define EPD_OF_ENTRY(type) \
	{ .compatible = "gooddisplay," #type, .data = &gddsp_ ## type ## _data }

static const struct of_device_id gdepaper_of_match[] = {
	{ .compatible = "gooddisplay,generic_epaper",	.data = NULL },
	EPD_OF_ENTRY(gdew0154z04t),
	EPD_OF_ENTRY(gdew0154z04fl),
	EPD_OF_ENTRY(gdew0154z04),
	EPD_OF_ENTRY(gdewl0154z17fl),
	EPD_OF_ENTRY(gdew0154z17),
	EPD_OF_ENTRY(gdew0154c39fl),
	EPD_OF_ENTRY(gdew0154c39),
	EPD_OF_ENTRY(gdew0213z16),
	EPD_OF_ENTRY(gdew0213c38),
	EPD_OF_ENTRY(gdew026z39),
	EPD_OF_ENTRY(gdew027c44t),
	EPD_OF_ENTRY(gdew027c44),
	EPD_OF_ENTRY(gdew029z10),
	EPD_OF_ENTRY(gdew029c32),
	EPD_OF_ENTRY(gdew0371z80),
	EPD_OF_ENTRY(gdew042z15),
	EPD_OF_ENTRY(gdew042c37),
	EPD_OF_ENTRY(gdew0583c64),
	EPD_OF_ENTRY(gdew0583z21),
	EPD_OF_ENTRY(gdew075z08),
	EPD_OF_ENTRY(gdew075z09),
	EPD_OF_ENTRY(gdew075c21),
	/* see comment above. */
	/* EPD_OF_ENTRY(gdew1248z95), */
	/* EPD_OF_ENTRY(gdew1248c63), */
	/* EPD_OF_ENTRY(gdew1248t3), */
	EPD_OF_ENTRY(gdew0102i4f),
	EPD_OF_ENTRY(gdew0102i4fc),
	EPD_OF_ENTRY(gdep014tt1),
	EPD_OF_ENTRY(gdep015oc1),
	EPD_OF_ENTRY(gdew0154t8t),
	EPD_OF_ENTRY(gdew0154t8fl),
	EPD_OF_ENTRY(gdew0154t8),
	EPD_OF_ENTRY(gdeh0154d27t),
	EPD_OF_ENTRY(gdem0154e97lt),
	EPD_OF_ENTRY(gdew0154i9f),
	EPD_OF_ENTRY(gdew0213t5),
	EPD_OF_ENTRY(gdew0213i5f),
	EPD_OF_ENTRY(gdeh0213d30lt),
	EPD_OF_ENTRY(gdew0213v7lt),
	EPD_OF_ENTRY(gdth0213zhft34),
	EPD_OF_ENTRY(gdew026t0),
	EPD_OF_ENTRY(gdew027w3t),
	EPD_OF_ENTRY(gdew027w3),
	EPD_OF_ENTRY(gdeh029a1),
	EPD_OF_ENTRY(gdew029t5),
	EPD_OF_ENTRY(gdeh029d56lt),
	EPD_OF_ENTRY(gdew029i6f),
	EPD_OF_ENTRY(gdew0371w7),
	EPD_OF_ENTRY(gdew042t2),
	EPD_OF_ENTRY(gdep043zf3),
	EPD_OF_ENTRY(gde043a2t),
	EPD_OF_ENTRY(gde043a2),
	EPD_OF_ENTRY(gdew0583t7),
	EPD_OF_ENTRY(gdew075t8),
	EPD_OF_ENTRY(gdew075t7),
	EPD_OF_ENTRY(gdew080t5),
	EPD_OF_ENTRY(gdep097tc2),
	{}
};

#undef EPD_OF_ENTRY
