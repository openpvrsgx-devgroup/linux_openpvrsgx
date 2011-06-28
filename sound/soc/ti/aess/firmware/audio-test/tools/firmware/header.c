#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define ABE_COEFF_MAGIC	0xABEABE00
#define ABE_COEFF_VERSION	1
#define ABE_FIRMWARE_VERSION	(firmware[0])

#define NAME_SIZE	20
#define TEXT_SIZE	20
#define NUM_TEXTS	10

#define MAX_PROFILES 	8  /* Number of supported profiles */
#define MAX_COEFFS 	25      /* Number of coefficients for profiles */


#define MAX_COEFF_SIZE	(MAX_PROFILES * MAX_COEFFS * sizeof(int32_t))
#define MAX_FW_SIZE (sizeof(firmware) + MAX_COEFF_SIZE)
#define MAX_FILE_SIZE (MAX_FW_SIZE + sizeof (struct header))

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))
#define NUM_COEFFS(x)	(sizeof(x[0]) / sizeof(x[0][0]))
#define NUM_PROFILES(x)	(sizeof(x) / sizeof(x[0]))

uint32_t firmware[] = {
#include "abe_firmware.c"
};

 /* ABE CONST AREA FOR EQUALIZER COEFFICIENTS
 *
 * TODO: These coefficents are for demonstration purposes. Set of
 * coefficients can be computed for specific needs.
 */


/*
 * Coefficients for DL1EQ
 */

#define DL1_COEFF		25

const int32_t dl1_equ_coeffs[][DL1_COEFF] = {
/* Flat response with Gain =1 */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 1  */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
				708210, -708206, 7554225, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.25 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.1 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},
};

/*
 * Coefficients for DL2EQ_L
 */

#define DL2L_COEFF	25

const int32_t dl2l_equ_coeffs[][DL2L_COEFF] = {
/* Flat response with Gain =1 */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 1 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
				708210, -708206, 7554225, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.25 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.1 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},
};

/*
 * Coefficients for DL2_EQ_R
 */

#define DL2R_COEFF	25

const int32_t dl2r_equ_coeffs[][DL2R_COEFF] = {
/* Flat response with Gain =1 */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 1 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
				708210, -708206, 7554225, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/*800Hz cut-off frequency and Gain = 0.25 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.1 */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},
};

/*
 * Coefficients for DMICEQ
 */

#define DMIC_COEFF	19

const int32_t dmic_equ_coeffs[][DMIC_COEFF] = {
/* 20kHz cut-off frequency and Gain = 1 */
				{-4119413, -192384, -341428, -348088,
				-151380, 151380, 348088, 341428, 192384,
				4119419, 1938156, -6935719, 775202,
				-1801934, 2997698, -3692214, 3406822,
				-2280190, 1042982},

/* 20kHz cut-off frequency and Gain = 0.25 */
				{-1029873, -3078121, -5462817, -5569389,
				-2422069, 2422071, 5569391, 5462819,
				3078123, 1029875, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},

/* 20kHz cut-off frequency and Gain = 0.125 */
				{-514937, -1539061, -2731409, -2784693,
				-1211033, 1211035, 2784695, 2731411,
				1539063, 514939, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},
};

/*
 * Coefficients for AMICEQ
 */

#define AMIC_COEFF	19

const int32_t amic_equ_coeffs[][AMIC_COEFF] = {
/* 20kHz cut-off frequency and Gain = 1 */
				{-4119413, -192384, -341428, -348088,
				-151380, 151380, 348088, 341428, 192384,
				4119419, 1938156, -6935719, 775202,
				-1801934, 2997698, -3692214, 3406822,
				-2280190, 1042982},

/* 20kHz cut-off frequency and Gain = 0.25 */
				{-1029873, -3078121, -5462817, -5569389,
				-2422069, 2422071, 5569391, 5462819,
				3078123, 1029875, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},

/* 20kHz cut-off frequency and Gain = 0.125 */
				{-514937, -1539061, -2731409, -2784693,
				-1211033, 1211035, 2784695, 2731411,
				1539063, 514939, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},
};


/*
 * Coefficients for SDTEQ
 */

#define SDT_COEFF	9

const int32_t sdt_equ_coeffs[][SDT_COEFF] = {
/* Flat response with Gain =1 */
				{0, 0, 0, 0, 0x040002, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 1  */
				{0, -7554223, 708210, -708206, 7554225,
				0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.25 */
				{0, -3777112, 5665669, -5665667, 3777112,
				0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = 0.1 */
				{0, -1510844, 4532536, -4532536, 1510844,
				0, 6802833, -682266, 731554}
};

struct config {
	char name[NAME_SIZE];
	uint32_t count;
	uint32_t coeff;
	char texts[NUM_TEXTS][TEXT_SIZE];
};
	

struct header {
	uint32_t magic;	/* magic number */
	uint32_t crc;	/* optional crc */
	uint32_t firmware_size;	/* payload size */
	uint32_t coeff_size;	/* payload size */
	uint32_t coeff_version;	/* coefficent version */
	uint32_t firmware_version;	/* min version of ABE firmware required */
	uint32_t num_equ;
	struct config equ[];
};

#define NUM_EQUALIZERS		5

struct header hdr = {
	.magic		= ABE_COEFF_MAGIC,
	.num_equ 	= NUM_EQUALIZERS,
	.equ	= {{
		.name = "DL1 Equalizer",
		.count = NUM_PROFILES(dl1_equ_coeffs),
		.coeff = NUM_COEFFS(dl1_equ_coeffs),
		.texts	= {
				"Flat response",
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.1dB",
		},},
		{
		.name = "DL2 Left Equalizer",
		.count = NUM_PROFILES(dl2l_equ_coeffs),
		.coeff = NUM_COEFFS(dl2l_equ_coeffs),
		.texts	= {
				"Flat response",
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.1dB",
		},},
		{
		.name = "DL2 Right Equalizer",
		.count = NUM_PROFILES(dl2r_equ_coeffs),
		.coeff = NUM_COEFFS(dl2r_equ_coeffs),
		.texts	= {
				"Flat response",
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.1dB",
		},},
		{
		.name = "AMIC Equalizer",
		.count = NUM_PROFILES(amic_equ_coeffs),
		.coeff = NUM_COEFFS(amic_equ_coeffs),
		.texts	= {
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.125dB",
		},},
		{
		.name = "DMIC Equalizer",
		.count = NUM_PROFILES(dmic_equ_coeffs),
		.coeff = NUM_COEFFS(dmic_equ_coeffs),
		.texts	= {
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.125dB",
		},},
		{
		.name = "Sidetone Equalizer",
		.count = NUM_PROFILES(sdt_equ_coeffs),
		.coeff = NUM_COEFFS(sdt_equ_coeffs),
		.texts	= {
				"Flat response",
				"High-pass 1dB",
				"High-pass 0.25dB",
				"High-pass 0.1dB",
		},},
	},
};

/* OMAP4 ABE Firmware Generation too.
 *
 * Firmware and coefficients are generated using this tool.
 * 
 * Header is at offset 0x0. Coefficients are appended to the end of the header.
 * Firmware is appended to the end of the coefficients.
 */
int main(int argc, char *argv[])
{
	int err, in_fd, out_fd, offset = 0;
	char *buf;

	if (argc != 2) {
		printf("%s: outfile\n", argv[0]);
		return 0;
	}

	buf = malloc(MAX_FILE_SIZE);
	if (buf == NULL)
		return -ENOMEM;
	bzero(buf, MAX_FILE_SIZE);

	/* open output file */
	out_fd = open(argv[1], O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (out_fd < 0) {
		printf("failed to open %s err %d\n",
			argv[1], out_fd);
		return out_fd;
	}

	hdr.firmware_version = ABE_FIRMWARE_VERSION;
	hdr.firmware_size = sizeof(firmware);
	hdr.coeff_version = ABE_COEFF_VERSION;
	offset = sizeof(hdr) + NUM_EQUALIZERS * sizeof(struct config);

	memcpy(buf + offset, dl1_equ_coeffs, sizeof(dl1_equ_coeffs));
	offset += sizeof(dl1_equ_coeffs);

	memcpy(buf + offset, dl2l_equ_coeffs, sizeof(dl2l_equ_coeffs));
	offset += sizeof(dl2l_equ_coeffs);

	memcpy(buf + offset, dl2r_equ_coeffs, sizeof(dl2r_equ_coeffs));
	offset += sizeof(dl2r_equ_coeffs);

	memcpy(buf + offset, amic_equ_coeffs, sizeof(amic_equ_coeffs));
	offset += sizeof(amic_equ_coeffs);

	memcpy(buf + offset, dmic_equ_coeffs, sizeof(dmic_equ_coeffs));
	offset += sizeof(dmic_equ_coeffs);

	memcpy(buf + offset, sdt_equ_coeffs, sizeof(sdt_equ_coeffs));
	offset += sizeof(sdt_equ_coeffs);

	hdr.coeff_size = offset - sizeof(hdr);

	memcpy(buf, &hdr, sizeof(hdr) + NUM_EQUALIZERS * sizeof(struct config));
	memcpy(buf + offset, firmware, sizeof(firmware));

	err = write(out_fd, buf, offset + sizeof(firmware));
	if (err <= 0)
		return err;

	close (out_fd);
}
