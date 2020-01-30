/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/


#ifndef	__RT_CHANNELPLAN_H__
#define __RT_CHANNELPLAN_H__

enum rt_channel_domain_new {

	/* ===== Add new channel plan above this line =============== */

	/* For new architecture we define different 2G/5G CH area for all country. */
	/* 2.4 G only */
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_NULL				= 0x20,
	RT_CHANNEL_DOMAIN_2G_ETSI1_5G_NULL				= 0x21,
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_NULL				= 0x22,
	RT_CHANNEL_DOMAIN_2G_MKK1_5G_NULL				= 0x23,
	RT_CHANNEL_DOMAIN_2G_ETSI2_5G_NULL				= 0x24,
	/* 2.4 G + 5G type 1 */
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC1				= 0x25,
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x26,
	/* RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x27, */
	/* ..... */

	RT_CHANNEL_DOMAIN_MAX_NEW,

};


#if 0
#define DOMAIN_CODE_2G_WORLD \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 13
#define DOMAIN_CODE_2G_ETSI1 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 13
#define DOMAIN_CODE_2G_ETSI2 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 11
#define DOMAIN_CODE_2G_FCC1 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}, 14
#define DOMAIN_CODE_2G_MKK1 \
	{10, 11, 12, 13}, 4

#define DOMAIN_CODE_5G_ETSI1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 19
#define DOMAIN_CODE_5G_ETSI2 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165}, 24
#define DOMAIN_CODE_5G_ETSI3 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 149, 153, 157, 161, 165}, 22
#define DOMAIN_CODE_5G_FCC1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165}, 24
#define DOMAIN_CODE_5G_FCC2 \
	{36, 40, 44, 48, 149, 153, 157, 161, 165}, 9
#define DOMAIN_CODE_5G_FCC3 \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165}, 13
#define DOMAIN_CODE_5G_FCC4 \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161}, 12
#define DOMAIN_CODE_5G_FCC5 \
	{149, 153, 157, 161, 165}, 5
#define DOMAIN_CODE_5G_FCC6 \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define DOMAIN_CODE_5G_FCC7 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define DOMAIN_CODE_5G_IC1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define DOMAIN_CODE_5G_KCC1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 149, 153, 157, 161, 165}, 20
#define DOMAIN_CODE_5G_MKK1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 19
#define DOMAIN_CODE_5G_MKK2 \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define DOMAIN_CODE_5G_MKK3 \
	{100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 11
#define DOMAIN_CODE_5G_NCC1 \
	{56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 24
#define DOMAIN_CODE_5G_NCC2 \
	{56, 60, 64, 149, 153, 157, 161, 165}, 8
#define UNDEFINED \
	{0}, 0
#endif

/*
 *
 *
 *

Countries							"Country Abbreviation"	Domain Code					SKU's	Ch# of 20MHz
															2G			5G						Ch# of 40MHz
"Albaniaªüº¸¤Ú¥§¨È"					AL													Local Test

"Algeriaªüº¸¤Î§Q¨È"					DZ									CE TCF

"Antigua & Barbuda¦w´£¥Ê®q&¤Ú¥¬¹F"	AG						2G_WORLD					FCC TCF

"Argentinaªü®Ú§Ê"					AR						2G_WORLD					Local Test

"Armenia¨È¬ü¥§¨È"					AM						2G_WORLD					ETSI

"Arubaªü¾|¤Ú®q"						AW						2G_WORLD					FCC TCF

"Australia¿D¬w"						AU						2G_WORLD		5G_ETSI2

"Austria¶ø¦a§Q"						AT						2G_WORLD		5G_ETSI1	CE

"Azerbaijanªü¶ë«ô¾Ê"				AZ						2G_WORLD					CE TCF

"Bahamas¤Ú«¢°¨"						BS						2G_WORLD

"Barbados¤Ú¤Ú¦h´µ"					BB						2G_WORLD					FCC TCF

"Belgium¤ñ§Q®É"						BE						2G_WORLD		5G_ETSI1	CE

"Bermuda¦Ê¼}¹F"						BM						2G_WORLD					FCC TCF

"Brazil¤Ú¦è"						BR						2G_WORLD					Local Test

"Bulgaria«O¥[§Q¨È"					BG						2G_WORLD		5G_ETSI1	CE

"Canada¥[®³¤j"						CA						2G_FCC1			5G_FCC7		IC / FCC	IC / FCC

"Cayman Islands¶}°Ò¸s®q"			KY						2G_WORLD		5G_ETSI1	CE

"Chile´¼§Q"							CL						2G_WORLD					FCC TCF

"China¤¤°ê"							CN						2G_WORLD		5G_FCC5		«H³¡?¡i2002¡j353?

"Columbia­ô­Û¤ñ¨È"					CO						2G_WORLD					Voluntary

"Costa Rica­ô´µ¹F¾¤¥["				CR						2G_WORLD					FCC TCF

"Cyprus¶ë®ú¸ô´µ"					CY						2G_WORLD		5G_ETSI1	CE

"Czech ±¶§J"						CZ						2G_WORLD		5G_ETSI1	CE

"Denmark¤¦³Á"						DK						2G_WORLD		5G_ETSI1	CE

"Dominican Republic¦h©ú¥§¥[¦@©M°ê"	DO						2G_WORLD					FCC TCF

"Egypt®J¤Î"	EG	2G_WORLD			CE T												CF

"El SalvadorÂÄº¸¥Ë¦h"				SV						2G_WORLD					Voluntary

"Estonia·R¨F¥§¨È"					EE						2G_WORLD		5G_ETSI1	CE

"FinlandªâÄõ"						FI						2G_WORLD		5G_ETSI1	CE

"Franceªk°ê"						FR										5G_E		TSI1	CE

"Germany¼w°ê"						DE						2G_WORLD		5G_ETSI1	CE

"Greece §ÆÃ¾"						GR						2G_WORLD		5G_ETSI1	CE

"GuamÃö®q"							GU						2G_WORLD

"Guatemala¥Ê¦a°¨©Ô"					GT						2G_WORLD

"Haiti®ü¦a"							HT						2G_WORLD					FCC TCF

"Honduras§»³£©Ô´µ"					HN						2G_WORLD					FCC TCF

"Hungary¦I¤ú§Q"						HU						2G_WORLD		5G_ETSI1	CE

"Iceland¦B®q"						IS						2G_WORLD		5G_ETSI1	CE

"India¦L«×"												2G_WORLD		5G_FCC3		FCC/CE TCF

"Ireland·Rº¸Äõ"						IE						2G_WORLD		5G_ETSI1	CE

"Israel¥H¦â¦C"						IL										5G_F		CC6	CE TCF

"Italy¸q¤j§Q"						IT						2G_WORLD		5G_ETSI1	CE

"Japan¤é¥»"							JP						2G_MKK1			5G_MKK1		MKK	MKK

"KoreaÁú°ê"							KR						2G_WORLD		5G_KCC1		KCC	KCC

"Latvia©Ô²æºû¨È"					LV						2G_WORLD		5G_ETSI1	CE

"Lithuania¥ß³³©{"					LT						2G_WORLD		5G_ETSI1	CE

"Luxembourg¿c´Ë³ù"					LU						2G_WORLD		5G_ETSI1	CE

"Malaysia°¨¨Ó¦è¨È"					MY						2G_WORLD					Local Test

"Malta°¨º¸¥L"						MT						2G_WORLD		5G_ETSI1	CE

"Mexico¾¥¦è­ô"						MX						2G_WORLD		5G_FCC3		Local Test

"Morocco¼¯¬¥­ô"						MA													CE TCF

"Netherlands²üÄõ"					NL						2G_WORLD		5G_ETSI1	CE

"New Zealand¯Ã¦èÄõ"					NZ						2G_WORLD		5G_ETSI2

"Norway®¿«Â"						NO						2G_WORLD		5G_ETSI1	CE

"Panama¤Ú®³°¨ "						PA						2G_FCC1						Voluntary

"Philippinesµá«ß»«"					PH						2G_WORLD					FCC TCF

"PolandªiÄõ"						PL						2G_WORLD		5G_ETSI1	CE

"Portugal¸²µå¤ú"					PT						2G_WORLD		5G_ETSI1	CE

"RomaniaÃ¹°¨¥§¨È"					RO						2G_WORLD		5G_ETSI1	CE

"Russia«XÃ¹´µ"						RU						2G_WORLD		5G_ETSI3	CE TCF

"Saudi Arabia¨F¦aªü©Ô§B"			SA						2G_WORLD					CE TCF

"Singapore·s¥[©Y"					SG						2G_WORLD

"Slovakia´µ¬¥¥ï§J"					SK						2G_WORLD		5G_ETSI1	CE

"Slovenia´µ¬¥ºû¥§¨È"				SI						2G_WORLD		5G_ETSI1	CE

"South Africa«n«D"					ZA						2G_WORLD					CE TCF

"Spain¦è¯Z¤ú"						ES										5G_ETSI1	CE

"Sweden·ç¨å"						SE						2G_WORLD		5G_ETSI1	CE

"Switzerland·ç¤h"					CH						2G_WORLD		5G_ETSI1	CE

"Taiwan»OÆW"						TW						2G_FCC1			5G_NCC1	NCC

"Thailand®õ°ê"						TH						2G_WORLD					FCC/CE TCF

"Turkey¤g¦Õ¨ä"						TR						2G_WORLD

"Ukraine¯Q§JÄõ"						UA						2G_WORLD					Local Test

"United Kingdom­^°ê"				GB						2G_WORLD		5G_ETSI1	CE	ETSI

"United States¬ü°ê"					US						2G_FCC1			5G_FCC7		FCC	FCC

"Venezuela©e¤º·ç©Ô"					VE						2G_WORLD		5G_FCC4		FCC TCF

"Vietnam¶V«n"						VN						2G_WORLD					FCC/CE TCF



*/

/* counter abbervation. */
enum rt_country_name {
	RT_CTRY_AL,				/*	"Albaniaªüº¸¤Ú¥§¨È" */
	RT_CTRY_DZ,             /* "Algeriaªüº¸¤Î§Q¨È" */
	RT_CTRY_AG,             /* "Antigua & Barbuda¦w´£¥Ê®q&¤Ú¥¬¹F" */
	RT_CTRY_AR,             /* "Argentinaªü®Ú§Ê" */
	RT_CTRY_AM,             /* "Armenia¨È¬ü¥§¨È" */
	RT_CTRY_AW,             /* "Arubaªü¾|¤Ú®q" */
	RT_CTRY_AU,             /* "Australia¿D¬w" */
	RT_CTRY_AT,             /* "Austria¶ø¦a§Q" */
	RT_CTRY_AZ,             /* "Azerbaijanªü¶ë«ô¾Ê" */
	RT_CTRY_BS,             /* "Bahamas¤Ú«¢°¨" */
	RT_CTRY_BB,             /* "Barbados¤Ú¤Ú¦h´µ" */
	RT_CTRY_BE,             /* "Belgium¤ñ§Q®É" */
	RT_CTRY_BM,             /* "Bermuda¦Ê¼}¹F" */
	RT_CTRY_BR,             /* "Brazil¤Ú¦è" */
	RT_CTRY_BG,             /* "Bulgaria«O¥[§Q¨È" */
	RT_CTRY_CA,             /* "Canada¥[®³¤j" */
	RT_CTRY_KY,             /* "Cayman Islands¶}°Ò¸s®q" */
	RT_CTRY_CL,             /* "Chile´¼§Q" */
	RT_CTRY_CN,             /* "China¤¤°ê" */
	RT_CTRY_CO,             /* "Columbia­ô­Û¤ñ¨È" */
	RT_CTRY_CR,             /* "Costa Rica­ô´µ¹F¾¤¥[" */
	RT_CTRY_CY,             /* "Cyprus¶ë®ú¸ô´µ" */
	RT_CTRY_CZ,             /* "Czech ±¶§J" */
	RT_CTRY_DK,             /* "Denmark¤¦³Á" */
	RT_CTRY_DO,             /* "Dominican Republic¦h©ú¥§¥[¦@©M°ê" */
	RT_CTRY_CE,             /* "Egypt®J¤Î"	EG	2G_WORLD */
	RT_CTRY_SV,             /* "El SalvadorÂÄº¸¥Ë¦h" */
	RT_CTRY_EE,             /* "Estonia·R¨F¥§¨È" */
	RT_CTRY_FI,             /* "FinlandªâÄõ" */
	RT_CTRY_FR,             /* "Franceªk°ê" */
	RT_CTRY_DE,             /* "Germany¼w°ê" */
	RT_CTRY_GR,             /* "Greece §ÆÃ¾" */
	RT_CTRY_GU,             /* "GuamÃö®q" */
	RT_CTRY_GT,             /* "Guatemala¥Ê¦a°¨©Ô" */
	RT_CTRY_HT,             /* "Haiti®ü¦a" */
	RT_CTRY_HN,             /* "Honduras§»³£©Ô´µ" */
	RT_CTRY_HU,             /* "Hungary¦I¤ú§Q" */
	RT_CTRY_IS,             /* "Iceland¦B®q" */
	RT_CTRY_IN,             /* "India¦L«×" */
	RT_CTRY_IE,             /* "Ireland·Rº¸Äõ" */
	RT_CTRY_IL,             /* "Israel¥H¦â¦C" */
	RT_CTRY_IT,             /* "Italy¸q¤j§Q" */
	RT_CTRY_JP,             /* "Japan¤é¥»" */
	RT_CTRY_KR,             /* "KoreaÁú°ê" */
	RT_CTRY_LV,             /* "Latvia©Ô²æºû¨È" */
	RT_CTRY_LT,             /* "Lithuania¥ß³³©{" */
	RT_CTRY_LU,             /* "Luxembourg¿c´Ë³ù" */
	RT_CTRY_MY,             /* "Malaysia°¨¨Ó¦è¨È" */
	RT_CTRY_MT,             /* "Malta°¨º¸¥L" */
	RT_CTRY_MX,             /* "Mexico¾¥¦è­ô" */
	RT_CTRY_MA,             /* "Morocco¼¯¬¥­ô" */
	RT_CTRY_NL,             /* "Netherlands²üÄõ" */
	RT_CTRY_NZ,             /* "New Zealand¯Ã¦èÄõ" */
	RT_CTRY_NO,             /* "Norway®¿«Â" */
	RT_CTRY_PA,             /* "Panama¤Ú®³°¨ " */
	RT_CTRY_PH,             /* "Philippinesµá«ß»«" */
	RT_CTRY_PL,             /* "PolandªiÄõ" */
	RT_CTRY_PT,             /* "Portugal¸²µå¤ú" */
	RT_CTRY_RO,             /* "RomaniaÃ¹°¨¥§¨È" */
	RT_CTRY_RU,             /* "Russia«XÃ¹´µ" */
	RT_CTRY_SA,             /* "Saudi Arabia¨F¦aªü©Ô§B" */
	RT_CTRY_SG,             /* "Singapore·s¥[©Y" */
	RT_CTRY_SK,             /* "Slovakia´µ¬¥¥ï§J" */
	RT_CTRY_SI,             /* "Slovenia´µ¬¥ºû¥§¨È" */
	RT_CTRY_ZA,             /* "South Africa«n«D" */
	RT_CTRY_ES,             /* "Spain¦è¯Z¤ú" */
	RT_CTRY_SE,             /* "Sweden·ç¨å" */
	RT_CTRY_CH,             /* "Switzerland·ç¤h" */
	RT_CTRY_TW,             /* "Taiwan»OÆW" */
	RT_CTRY_TH,             /* "Thailand®õ°ê" */
	RT_CTRY_TR,             /* "Turkey¤g¦Õ¨ä" */
	RT_CTRY_UA,             /* "Ukraine¯Q§JÄõ" */
	RT_CTRY_GB,             /* "United Kingdom­^°ê" */
	RT_CTRY_US,             /* "United States¬ü°ê" */
	RT_CTRY_VE,             /* "Venezuela©e¤º·ç©Ô" */
	RT_CTRY_VN,             /* "Vietnam¶V«n" */
	RT_CTRY_MAX,

};

/* Scan type including active and passive scan. */
enum rt_scan_type_new {
	SCAN_NULL,
	SCAN_ACT,
	SCAN_PAS,
	SCAN_BOTH,
};


/* Power table sample. */

struct _RT_CHNL_PLAN_LIMIT {
	u16	chnl_start;
	u16	chnl_end;

	u16	freq_start;
	u16	freq_end;
};


/*
 * 2.4G Regulatory Domains
 *   */
enum rt_regulation_2g {
	RT_2G_NULL,
	RT_2G_WORLD,
	RT_2G_ETSI1,
	RT_2G_FCC1,
	RT_2G_MKK1,
	RT_2G_ETSI2

};


/* typedef struct _RT_CHANNEL_BEHAVIOR
 * {
 *	u8	chnl;
 *	enum rt_scan_type_new
 *
 * }RT_CHANNEL_BEHAVIOR, *PRT_CHANNEL_BEHAVIOR; */

/* typedef struct _RT_CHANNEL_PLAN_TYPE
 * {
 *	RT_CHANNEL_BEHAVIOR
 *	u8					Chnl_num;
 * }RT_CHNL_PLAN_TYPE, *PRT_CHNL_PLAN_TYPE; */

/*
 * 2.4G channel number
 * channel definition & number
 *   */
#define CHNL_RT_2G_NULL \
	{0}, 0
#define CHNL_RT_2G_WORLD \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 13
#define CHNL_RT_2G_WORLD_TEST \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 13

#define CHNL_RT_2G_EFSI1 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 13
#define CHNL_RT_2G_FCC1 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 11
#define CHNL_RT_2G_MKK1 \
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}, 14
#define CHNL_RT_2G_ETSI2 \
	{10, 11, 12, 13}, 4

/*
 * 2.4G channel active or passive scan.
 *   */
#define CHNL_RT_2G_NULL_SCAN_TYPE \
	{SCAN_NULL}
#define CHNL_RT_2G_WORLD_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}
#define CHNL_RT_2G_EFSI1_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
#define CHNL_RT_2G_FCC1_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
#define CHNL_RT_2G_MKK1_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
#define CHNL_RT_2G_ETSI2_SCAN_TYPE \
	{1, 1, 1, 1}


/*
 * 2.4G band & Frequency Section
 * Freqency start & end / band number
 *   */
#define FREQ_RT_2G_NULL \
	{0}, 0
/* Passive scan CH 12, 13 */
#define FREQ_RT_2G_WORLD \
	{2412, 2472}, 1
#define FREQ_RT_2G_EFSI1 \
	{2412, 2472}, 1
#define FREQ_RT_2G_FCC1 \
	{2412, 2462}, 1
#define FREQ_RT_2G_MKK1 \
	{2412, 2484}, 1
#define FREQ_RT_2G_ETSI2 \
	{2457, 2472}, 1


/*
 * 5G Regulatory Domains
 *   */
enum rt_regulation_5g {
	RT_5G_NULL,
	RT_5G_WORLD,
	RT_5G_ETSI1,
	RT_5G_ETSI2,
	RT_5G_ETSI3,
	RT_5G_FCC1,
	RT_5G_FCC2,
	RT_5G_FCC3,
	RT_5G_FCC4,
	RT_5G_FCC5,
	RT_5G_FCC6,
	RT_5G_FCC7,
	RT_5G_IC1,
	RT_5G_KCC1,
	RT_5G_MKK1,
	RT_5G_MKK2,
	RT_5G_MKK3,
	RT_5G_NCC1,

};

/*
 * 5G channel number
 *   */
#define CHNL_RT_5G_NULL \
	{0}, 0
#define CHNL_RT_5G_WORLD \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 19
#define CHNL_RT_5G_ETSI1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165}, 24
#define CHNL_RT_5G_ETSI2 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 149, 153, 157, 161, 165}, 22
#define CHNL_RT_5G_ETSI3 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165}, 24
#define CHNL_RT_5G_FCC1 \
	{36, 40, 44, 48, 149, 153, 157, 161, 165}, 9
#define CHNL_RT_5G_FCC2 \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165}, 13
#define CHNL_RT_5G_FCC3 \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161}, 12
#define CHNL_RT_5G_FCC4 \
	{149, 153, 157, 161, 165}, 5
#define CHNL_RT_5G_FCC5 \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define CHNL_RT_5G_FCC6 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_FCC7 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_IC1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_KCC1 \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 19
#define CHNL_RT_5G_MKK1 \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define CHNL_RT_5G_MKK2 \
	{100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 11
#define CHNL_RT_5G_MKK3 \
	{56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 24
#define CHNL_RT_5G_NCC1 \
	{56, 60, 64, 149, 153, 157, 161, 165}, 8

/*
 * 5G channel active or passive scan.
 *   */
#define CHNL_RT_5G_NULL_SCAN_TYPE \
	{SCAN_NULL}
#define CHNL_RT_5G_WORLD_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
#define CHNL_RT_5G_ETSI1_SCAN_TYPE \
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
#define CHNL_RT_5G_ETSI2_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 149, 153, 157, 161, 165}, 22
#define CHNL_RT_5G_ETSI3_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165}, 24
#define CHNL_RT_5G_FCC1_SCAN_TYPE \
	{36, 40, 44, 48, 149, 153, 157, 161, 165}, 9
#define CHNL_RT_5G_FCC2_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165}, 13
#define CHNL_RT_5G_FCC3_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161}, 12
#define CHNL_RT_5G_FCC4_SCAN_TYPE \
	{149, 153, 157, 161, 165}, 5
#define CHNL_RT_5G_FCC5_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define CHNL_RT_5G_FCC6_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_FCC7_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_IC1_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 149, 153, 157, 161, 165}, 20
#define CHNL_RT_5G_KCC1_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 19
#define CHNL_RT_5G_MKK1_SCAN_TYPE \
	{36, 40, 44, 48, 52, 56, 60, 64}, 8
#define CHNL_RT_5G_MKK2_SCAN_TYPE \
	{100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}, 11
#define CHNL_RT_5G_MKK3_SCAN_TYPE \
	{56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165}, 24
#define CHNL_RT_5G_NCC1_SCAN_TYPE \
	{56, 60, 64, 149, 153, 157, 161, 165}, 8

/*
 * Global regulation
 *   */
enum rt_regulation_cmn {
	RT_WORLD,
	RT_FCC,
	RT_MKK,
	RT_ETSI,
	RT_IC,
	RT_CE,
	RT_NCC,

};



/*
 * Special requirement for different regulation domain.
 * For internal test or customerize special request.
 *   */
enum rt_chnlplan_sreq {
	RT_SREQ_NA						= 0x0,
	RT_SREQ_2G_ADHOC_11N			= 0x00000001,
	RT_SREQ_2G_ADHOC_11B			= 0x00000002,
	RT_SREQ_2G_ALL_PASS				= 0x00000004,
	RT_SREQ_2G_ALL_ACT				= 0x00000008,
	RT_SREQ_5G_ADHOC_11N			= 0x00000010,
	RT_SREQ_5G_ADHOC_11AC			= 0x00000020,
	RT_SREQ_5G_ALL_PASS				= 0x00000040,
	RT_SREQ_5G_ALL_ACT				= 0x00000080,
	RT_SREQ_C1_PLAN					= 0x00000100,
	RT_SREQ_C2_PLAN					= 0x00000200,
	RT_SREQ_C3_PLAN					= 0x00000400,
	RT_SREQ_C4_PLAN					= 0x00000800,
	RT_SREQ_NFC_ON					= 0x00001000,
	RT_SREQ_MASK					= 0x0000FFFF,   /* Requirements bit mask */

};


/*
 * enum rt_country_name & enum rt_regulation_2g & enum rt_regulation_5g transfer table
 *
 *   */
struct _RT_CHANNEL_PLAN_COUNTRY_TRANSFER_TABLE {
	/*  */
	/* Define countery domain and corresponding */
	/*  */
	enum rt_country_name		country_enum;
	char				country_name[3];

	/* char		Domain_Name[12]; */
	enum rt_regulation_2g	domain_2g;

	enum rt_regulation_5g	domain_5g;

	RT_CHANNEL_DOMAIN	rt_ch_domain;
	/* u8		Country_Area; */

};


#define		RT_MAX_CHNL_NUM_2G		13
#define		RT_MAX_CHNL_NUM_5G		44

/* Power table sample. */

struct _RT_CHNL_PLAN_PWR_LIMIT {
	u16	chnl_start;
	u16	chnl_end;
	u8	db_max;
	u16	m_w_max;
};


#define		RT_MAX_BAND_NUM			5

struct _RT_CHANNEL_PLAN_MAXPWR {
	/*	STRING_T */
	struct _RT_CHNL_PLAN_PWR_LIMIT	chnl[RT_MAX_BAND_NUM];
	u8				band_useful_num;


};


/*
 * Power By rate Table.
 *   */



struct _RT_CHANNEL_PLAN_NEW {
	/*  */
	/* Define countery domain and corresponding */
	/*  */
	/* char		country_name[36]; */
	/* u8		country_enum; */

	/* char		Domain_Name[12]; */


	struct _RT_CHANNEL_PLAN_COUNTRY_TRANSFER_TABLE		*p_ctry_transfer;

	RT_CHANNEL_DOMAIN		rt_ch_domain;

	enum rt_regulation_2g		domain_2g;

	enum rt_regulation_5g		domain_5g;

	enum rt_regulation_cmn		regulator;

	enum rt_chnlplan_sreq		chnl_sreq;

	/* struct _RT_CHNL_PLAN_LIMIT		RtChnl; */

	u8	chnl_2g[MAX_CHANNEL_NUM];				/* CHNL_RT_2G_WORLD */
	u8	len_2g;
	u8	chnl_2g_scan_tp[MAX_CHANNEL_NUM];			/* CHNL_RT_2G_WORLD_SCAN_TYPE */
	/* u8	Freq2G[2];								 */ /* FREQ_RT_2G_WORLD */

	u8	chnl_5g[MAX_CHANNEL_NUM];
	u8	len_5g;
	u8	chnl_5g_scan_tp[MAX_CHANNEL_NUM];
	/* u8	Freq2G[2];								 */ /* FREQ_RT_2G_WORLD */

	struct _RT_CHANNEL_PLAN_MAXPWR	chnl_max_pwr;


};


#endif /* __RT_CHANNELPLAN_H__ */
