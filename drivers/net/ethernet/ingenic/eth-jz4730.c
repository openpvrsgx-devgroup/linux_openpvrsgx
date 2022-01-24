// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/drivers/net/ethernet/ingenic/jz4730.c
 *
 *  Jz4730/Jz5730 On-Chip ethernet driver.
 *
 *  Copyright (C) 2005 - 2007  Ingenic Semiconductor Inc.
 *  Copyright (C) 2021 H. Nikolaus Schaller <hns@goldelico.com> - adaptation to v5.12
 *
 */

#define DEBUG

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>

/*************************************************************************
 * ETH
 *************************************************************************/
#define ETH_BMR		(ETH_BASE + 0x1000)
#define ETH_TPDR	(ETH_BASE + 0x1004)
#define ETH_RPDR	(ETH_BASE + 0x1008)
#define ETH_RAR		(ETH_BASE + 0x100C)
#define ETH_TAR		(ETH_BASE + 0x1010)
#define ETH_SR		(ETH_BASE + 0x1014)
#define ETH_CR		(ETH_BASE + 0x1018)
#define ETH_IER		(ETH_BASE + 0x101C)
#define ETH_MFCR	(ETH_BASE + 0x1020)
#define ETH_CTAR	(ETH_BASE + 0x1050)
#define ETH_CRAR	(ETH_BASE + 0x1054)
#define ETH_MCR		(ETH_BASE + 0x0000)
#define ETH_MAHR	(ETH_BASE + 0x0004)
#define ETH_MALR	(ETH_BASE + 0x0008)
#define ETH_HTHR	(ETH_BASE + 0x000C)
#define ETH_HTLR	(ETH_BASE + 0x0010)
#define ETH_MIAR	(ETH_BASE + 0x0014)
#define ETH_MIDR	(ETH_BASE + 0x0018)
#define ETH_FCR		(ETH_BASE + 0x001C)
#define ETH_VTR1	(ETH_BASE + 0x0020)
#define ETH_VTR2	(ETH_BASE + 0x0024)
#define ETH_WKFR	(ETH_BASE + 0x0028)
#define ETH_PMTR	(ETH_BASE + 0x002C)

#if 0 // there is no REG32 macro
#define REG_ETH_BMR	REG32(ETH_BMR)
#define REG_ETH_TPDR	REG32(ETH_TPDR)
#define REG_ETH_RPDR	REG32(ETH_RPDR)
#define REG_ETH_RAR	REG32(ETH_RAR)
#define REG_ETH_TAR	REG32(ETH_TAR)
#define REG_ETH_SR	REG32(ETH_SR)
#define REG_ETH_CR	REG32(ETH_CR)
#define REG_ETH_IER	REG32(ETH_IER)
#define REG_ETH_MFCR	REG32(ETH_MFCR)
#define REG_ETH_CTAR	REG32(ETH_CTAR)
#define REG_ETH_CRAR	REG32(ETH_CRAR)
#define REG_ETH_MCR	REG32(ETH_MCR)
#define REG_ETH_MAHR	REG32(ETH_MAHR)
#define REG_ETH_MALR	REG32(ETH_MALR)
#define REG_ETH_HTHR	REG32(ETH_HTHR)
#define REG_ETH_HTLR	REG32(ETH_HTLR)
#define REG_ETH_MIAR	REG32(ETH_MIAR)
#define REG_ETH_MIDR	REG32(ETH_MIDR)
#define REG_ETH_FCR	REG32(ETH_FCR)
#define REG_ETH_VTR1	REG32(ETH_VTR1)
#define REG_ETH_VTR2	REG32(ETH_VTR2)
#define REG_ETH_WKFR	REG32(ETH_WKFR)
#define REG_ETH_PMTR	REG32(ETH_PMTR)
#endif

/* Bus Mode Register (ETH_BMR) */

#define ETH_BMR_DBO		BIT(20)
#define ETH_BMR_PBL_BIT		8
#define ETH_BMR_PBL_MASK	(0x3f << ETH_BMR_PBL_BIT)
  #define ETH_BMR_PBL_1		  (0x1 << ETH_BMR_PBL_BIT)
  #define ETH_BMR_PBL_4		  (0x4 << ETH_BMR_PBL_BIT)
#define ETH_BMR_BLE		BIT(7)
#define ETH_BMR_DSL_BIT		2
#define ETH_BMR_DSL_MASK	(0x1f << ETH_BMR_DSL_BIT)
  #define ETH_BMR_DSL_0		  (0x0 << ETH_BMR_DSL_BIT)
  #define ETH_BMR_DSL_1		  (0x1 << ETH_BMR_DSL_BIT)
  #define ETH_BMR_DSL_2		  (0x2 << ETH_BMR_DSL_BIT)
  #define ETH_BMR_DSL_4		  (0x4 << ETH_BMR_DSL_BIT)
  #define ETH_BMR_DSL_8		  (0x8 << ETH_BMR_DSL_BIT)
#define ETH_BMR_SWR		BIT(0)

/* DMA Status Register (ETH_SR) */

#define ETH_SR_EB_BIT		23
#define ETH_SR_EB_MASK		(0x7 << ETH_SR_EB_BIT)
  #define ETH_SR_EB_TX_ABORT	  (0x1 << ETH_SR_EB_BIT)
  #define ETH_SR_EB_RX_ABORT	  (0x2 << ETH_SR_EB_BIT)
#define ETH_SR_TS_BIT		20
#define ETH_SR_TS_MASK		(0x7 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_STOP	  (0x0 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_FTD		  (0x1 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_WEOT	  (0x2 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_QDAT	  (0x3 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_SUSPEND	  (0x6 << ETH_SR_TS_BIT)
  #define ETH_SR_TS_CTD		  (0x7 << ETH_SR_TS_BIT)
#define ETH_SR_RS_BIT		17
#define ETH_SR_RS_MASK		(0x7 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_STOP	  (0x0 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_FRD		  (0x1 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_CEOR	  (0x2 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_WRP		  (0x3 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_SUSPEND	  (0x4 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_CRD		  (0x5 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_FCF		  (0x6 << ETH_SR_RS_BIT)
  #define ETH_SR_RS_QRF		  (0x7 << ETH_SR_RS_BIT)
#define ETH_SR_NIS		BIT(16)
#define ETH_SR_AIS		BIT(15)
#define ETH_SR_ERI		BIT(14)
#define ETH_SR_FBE		BIT(13)
#define ETH_SR_ETI		BIT(10)
#define ETH_SR_RWT		BIT(9)
#define ETH_SR_RPS		BIT(8)
#define ETH_SR_RU		BIT(7)
#define ETH_SR_RI		BIT(6)
#define ETH_SR_UNF		BIT(5)
#define ETH_SR_TJT		BIT(3)
#define ETH_SR_TU		BIT(2)
#define ETH_SR_TPS		BIT(1)
#define ETH_SR_TI		BIT(0)

/* Control (Operation Mode) Register (ETH_CR) */

#define ETH_CR_TTM		BIT(22)
#define ETH_CR_SF		BIT(21)
#define ETH_CR_TR_BIT		14
#define ETH_CR_TR_MASK		(0x3 << ETH_CR_TR_BIT)
#define ETH_CR_ST		BIT(13)
#define ETH_CR_OSF		BIT(2)
#define ETH_CR_SR		BIT(1)

/* Interrupt Enable Register (ETH_IER) */

#define ETH_IER_NI		BIT(16)
#define ETH_IER_AI		BIT(15)
#define ETH_IER_ERE		BIT(14)
#define ETH_IER_FBE		BIT(13)
#define ETH_IER_ET		BIT(10)
#define ETH_IER_RWE		BIT(9)
#define ETH_IER_RS		BIT(8)
#define ETH_IER_RU		BIT(7)
#define ETH_IER_RI		BIT(6)
#define ETH_IER_UN		BIT(5)
#define ETH_IER_TJ		BIT(3)
#define ETH_IER_TU		BIT(2)
#define ETH_IER_TS		BIT(1)
#define ETH_IER_TI		BIT(0)

/* Missed Frame and Buffer Overflow Counter Register (ETH_MFCR) */

#define ETH_MFCR_OVERFLOW_BIT	17
#define ETH_MFCR_OVERFLOW_MASK	(0x7ff << ETH_MFCR_OVERFLOW_BIT)
#define ETH_MFCR_MFC_BIT	0
#define ETH_MFCR_MFC_MASK	(0xffff << ETH_MFCR_MFC_BIT)

/* MAC Control Register (ETH_MCR) */

#define ETH_MCR_RA		BIT(31)
#define ETH_MCR_HBD		BIT(28)
#define ETH_MCR_PS		BIT(27)
#define ETH_MCR_DRO		BIT(23)
#define ETH_MCR_OM_BIT		21
#define ETH_MCR_OM_MASK		(0x3 << ETH_MCR_OM_BIT)
  #define ETH_MCR_OM_NORMAL	  (0x0 << ETH_MCR_OM_BIT)
  #define ETH_MCR_OM_INTERNAL	  (0x1 << ETH_MCR_OM_BIT)
  #define ETH_MCR_OM_EXTERNAL	  (0x2 << ETH_MCR_OM_BIT)
#define ETH_MCR_F		BIT(20)
#define ETH_MCR_PM		BIT(19)
#define ETH_MCR_PR		BIT(18)
#define ETH_MCR_IF		BIT(17)
#define ETH_MCR_PB		BIT(16)
#define ETH_MCR_HO		BIT(15)
#define ETH_MCR_HP		BIT(13)
#define ETH_MCR_LCC		BIT(12)
#define ETH_MCR_DBF		BIT(11)
#define ETH_MCR_DTRY		BIT(10)
#define ETH_MCR_ASTP		BIT(8)
#define ETH_MCR_BOLMT_BIT	6
#define ETH_MCR_BOLMT_MASK	(0x3 << ETH_MCR_BOLMT_BIT)
  #define ETH_MCR_BOLMT_10	  (0 << ETH_MCR_BOLMT_BIT)
  #define ETH_MCR_BOLMT_8	  BIT(ETH_MCR_BOLMT_BIT)
  #define ETH_MCR_BOLMT_4	  (2 << ETH_MCR_BOLMT_BIT)
  #define ETH_MCR_BOLMT_1	  (3 << ETH_MCR_BOLMT_BIT)
#define ETH_MCR_DC		BIT(5)
#define ETH_MCR_TE		BIT(3)
#define ETH_MCR_RE		BIT(2)

/* MII Address Register (ETH_MIAR) */

#define ETH_MIAR_PHY_ADDR_BIT	11
#define ETH_MIAR_PHY_ADDR_MASK	(0x1f << ETH_MIAR_PHY_ADDR_BIT)
#define ETH_MIAR_MII_REG_BIT	6
#define ETH_MIAR_MII_REG_MASK	(0x1f << ETH_MIAR_MII_REG_BIT)
#define ETH_MIAR_MII_WRITE	BIT(1)
#define ETH_MIAR_MII_BUSY	BIT(0)

/* Flow Control Register (ETH_FCR) */

#define	ETH_FCR_PAUSE_TIME_BIT	16
#define	ETH_FCR_PAUSE_TIME_MASK	(0xffff << ETH_FCR_PAUSE_TIME_BIT)
#define	ETH_FCR_PCF		BIT(2)
#define	ETH_FCR_FCE		BIT(1)
#define	ETH_FCR_BUSY		BIT(0)

/* PMT Control and Status Register (ETH_PMTR) */

#define ETH_PMTR_GU		BIT(9)
#define ETH_PMTR_RF		BIT(6)
#define ETH_PMTR_MF		BIT(5)
#define ETH_PMTR_RWK		BIT(2)
#define ETH_PMTR_MPK		BIT(1)

/* Receive Descriptor 0 (ETH_RD0) Bits */

#define ETH_RD0_OWN		BIT(31)
#define ETH_RD0_FF		BIT(30)
#define ETH_RD0_FL_BIT		16
#define ETH_RD0_FL_MASK		(0x3fff << ETH_RD0_FL_BIT)
#define ETH_RD0_ES		BIT(15)
#define ETH_RD0_DE		BIT(14)
#define ETH_RD0_LE		BIT(12)
#define ETH_RD0_RF		BIT(11)
#define ETH_RD0_MF		BIT(10)
#define ETH_RD0_FD		BIT(9)
#define ETH_RD0_LD		BIT(8)
#define ETH_RD0_TL		BIT(7)
#define ETH_RD0_CS		BIT(6)
#define ETH_RD0_FT		BIT(5)
#define ETH_RD0_WT		BIT(4)
#define ETH_RD0_ME		BIT(3)
#define ETH_RD0_DB		BIT(2)
#define ETH_RD0_CE		BIT(1)

/* Receive Descriptor 1 (ETH_RD1) Bits */

#define ETH_RD1_RER		BIT(25)
#define ETH_RD1_RCH		BIT(24)
#define ETH_RD1_RBS2_BIT	11
#define ETH_RD1_RBS2_MASK	(0x7ff << ETH_RD1_RBS2_BIT)
#define ETH_RD1_RBS1_BIT	0
#define ETH_RD1_RBS1_MASK	(0x7ff << ETH_RD1_RBS1_BIT)

/* Transmit Descriptor 0 (ETH_TD0) Bits */

#define ETH_TD0_OWN		BIT(31)
#define ETH_TD0_FA		BIT(15)
#define ETH_TD0_LOC		BIT(11)
#define ETH_TD0_NC		BIT(10)
#define ETH_TD0_LC		BIT(9)
#define ETH_TD0_EC		BIT(8)
#define ETH_TD0_HBF		BIT(7)
#define ETH_TD0_CC_BIT		3
#define ETH_TD0_CC_MASK		(0xf << ETH_TD0_CC_BIT)
#define ETH_TD0_ED		BIT(2)
#define ETH_TD0_UF		BIT(1)
#define ETH_TD0_DF		BIT(0)

/* Transmit Descriptor 1 (ETH_TD1) Bits */

#define ETH_TD1_IC		BIT(31)
#define ETH_TD1_LS		BIT(30)
#define ETH_TD1_FS		BIT(29)
#define ETH_TD1_AC		BIT(26)
#define ETH_TD1_TER		BIT(25)
#define ETH_TD1_TCH		BIT(24)
#define ETH_TD1_DPD		BIT(23)
#define ETH_TD1_TBS2_BIT	11
#define ETH_TD1_TBS2_MASK	(0x7ff << ETH_TD1_TBS2_BIT)
#define ETH_TD1_TBS1_BIT	0
#define ETH_TD1_TBS1_MASK	(0x7ff << ETH_TD1_TBS1_BIT)

/*
 *  linux/drivers/net/jz_eth.h
 *
 *  Jz4730/Jz5730 On-Chip ethernet driver.
 *
 *  Copyright (C) 2005 - 2007  Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* DMA control and status registers */
#define DMA_BMR                (ETH_BASE + 0x1000) // Bus mode
#define DMA_TPD                (ETH_BASE + 0x1004) // Transmit poll demand register
#define DMA_RPD                (ETH_BASE + 0x1008) // Receive poll demand register
#define DMA_RRBA               (ETH_BASE + 0x100C) // Receive descriptor base address
#define DMA_TRBA               (ETH_BASE + 0x1010) // Transmit descriptor base address
#define DMA_STS                (ETH_BASE + 0x1014) // Status register
#define DMA_OMR                (ETH_BASE + 0x1018) // Command register
#define DMA_IMR                (ETH_BASE + 0x101C)
#define DMA_MFC                (ETH_BASE + 0x1020)

/* DMA CSR8-CSR19 reserved */
#define DMA_CTA                (ETH_BASE + 0x1050)
#define DMA_CRA                (ETH_BASE + 0x1054)

/* Mac control and status registers */
#define MAC_MCR                (ETH_BASE + 0x0000)
#define MAC_MAH                (ETH_BASE + 0x0004)
#define MAC_MAL                (ETH_BASE + 0x0008)
#define MAC_HTH                (ETH_BASE + 0x000C)
#define MAC_HTL                (ETH_BASE + 0x0010)
#define MAC_MIIA               (ETH_BASE + 0x0014)
#define MAC_MIID               (ETH_BASE + 0x0018)
#define MAC_FCR                (ETH_BASE + 0x001C)
#define MAC_VTR1               (ETH_BASE + 0x0020)
#define MAC_VTR2               (ETH_BASE + 0x0024)

/*
 * Bus Mode Register (DMA_BMR)
 */
#define BMR_PBL    0x00003f00       /* Programmable Burst Length */
#define BMR_DSL    0x0000007c       /* Descriptor Skip Length */
#define BMR_BAR    0x00000002       /* Bus ARbitration */
#define BMR_SWR    0x00000001       /* Software Reset */

#define PBL_0      0x00000000       /*  DMA burst length = amount in RX FIFO */
#define PBL_1      0x00000100       /*  1 longword  DMA burst length */
#define PBL_2      0x00000200       /*  2 longwords DMA burst length */
#define PBL_4      0x00000400       /*  4 longwords DMA burst length */
#define PBL_8      0x00000800       /*  8 longwords DMA burst length */
#define PBL_16     0x00001000       /* 16 longwords DMA burst length */
#define PBL_32     0x00002000       /* 32 longwords DMA burst length */

#define DSL_0      0x00000000       /*  0 longword  / descriptor */
#define DSL_1      0x00000004       /*  1 longword  / descriptor */
#define DSL_2      0x00000008       /*  2 longwords / descriptor */
#define DSL_4      0x00000010       /*  4 longwords / descriptor */
#define DSL_8      0x00000020       /*  8 longwords / descriptor */
#define DSL_16     0x00000040       /* 16 longwords / descriptor */
#define DSL_32     0x00000080       /* 32 longwords / descriptor */

/*
 * Status Register (DMA_STS)
 */
#define STS_BE     0x03800000       /* Bus Error Bits */
#define STS_TS     0x00700000       /* Transmit Process State */
#define STS_RS     0x000e0000       /* Receive Process State */

#define TS_STOP    0x00000000       /* Stopped */
#define TS_FTD     0x00100000       /* Running Fetch Transmit Descriptor */
#define TS_WEOT    0x00200000       /* Running Wait for End Of Transmission */
#define TS_QDAT    0x00300000       /* Running Queue skb data into TX FIFO */
#define TS_RES     0x00400000       /* Reserved */
#define TS_SPKT    0x00500000       /* Reserved */
#define TS_SUSP    0x00600000       /* Suspended */
#define TS_CLTD    0x00700000       /* Running Close Transmit Descriptor */

#define RS_STOP    0x00000000       /* Stopped */
#define RS_FRD     0x00020000       /* Running Fetch Receive Descriptor */
#define RS_CEOR    0x00040000       /* Running Check for End of Receive Packet */
#define RS_WFRP    0x00060000       /* Running Wait for Receive Packet */
#define RS_SUSP    0x00080000       /* Suspended */
#define RS_CLRD    0x000a0000       /* Running Close Receive Descriptor */
#define RS_FLUSH   0x000c0000       /* Running Flush RX FIFO */
#define RS_QRFS    0x000e0000       /* Running Queue RX FIFO into RX Skb */

/*
 * Operation Mode Register (DMA_OMR)
 */
#define OMR_TTM    0x00400000       /* Transmit Threshold Mode */
#define OMR_SF     0x00200000       /* Store and Forward */
#define OMR_TR     0x0000c000       /* Threshold Control Bits */
#define OMR_ST     0x00002000       /* Start/Stop Transmission Command */
#define OMR_OSF    0x00000004       /* Operate on Second Frame */
#define OMR_SR     0x00000002       /* Start/Stop Receive */

#define TR_18      0x00000000       /* Threshold set to 18 (32) bytes */
#define TR_24      0x00004000       /* Threshold set to 24 (64) bytes */
#define TR_32      0x00008000       /* Threshold set to 32 (128) bytes */
#define TR_40      0x0000c000       /* Threshold set to 40 (256) bytes */

/*
 * Missed Frames Counters (DMA_MFC)
 */
//#define MFC_CNT1   0xffff0000       /* Missed Frames Counter Bits by application */
#define MFC_CNT1   0x0ffe0000       /* Missed Frames Counter Bits by application */
#define MFC_CNT2   0x0000ffff       /* Missed Frames Counter Bits by controller */

/*
 * Mac control  Register (MAC_MCR)
 */
#define MCR_RA     0x80000000       /* Receive All */
#define MCR_HBD    0x10000000       /* HeartBeat Disable */
#define MCR_PS     0x08000000       /* Port Select */
#define MCR_OWD    0x00800000       /* Receive own Disable */
#define MCR_OM     0x00600000       /* Operating(loopback) Mode */
#define MCR_FDX    0x00100000       /* Full Duplex Mode */
#define MCR_PM     0x00080000       /* Pass All Multicast */
#define MCR_PR     0x00040000       /* Promiscuous Mode */
#define MCR_IF     0x00020000       /* Inverse Filtering */
#define MCR_PB     0x00010000       /* Pass Bad Frames */
#define MCR_HO     0x00008000       /* Hash Only Filtering Mode */
#define MCR_HP     0x00002000       /* Hash/Perfect Receive Filtering Mode */
#define MCR_FC     0x00001000       /* Late Collision control */
#define MCR_BFD    0x00000800       /* Broadcast frame Disable */
#define MCR_RED    0x00000400       /* Retry Disable */
#define MCR_APS    0x00000100       /* Automatic pad stripping */
#define MCR_BL     0x000000c0       /* Back off Limit */
#define MCR_DC     0x00000020       /* Deferral check */
#define MCR_TE     0x00000008       /* Transmitter enable */
#define MCR_RE     0x00000004       /* Receiver enable */

#define MCR_MII_10  (OMR_TTM | MCR_PS)
#define MCR_MII_100 (MCR_HBD | MCR_PS)

/* Flow control Register (MAC_FCR) */
#define FCR_PT     0xffff0000       /* Pause time */
#define FCR_PCF    0x00000004       /* Pass control frames */
#define FCR_FCE    0x00000002       /* Flow control enable */
#define FCR_FCB    0x00000001       /* Flow control busy */

/* Constants for the interrupt mask and
 * interrupt status registers. (DMA_SIS and DMA_IMR)
 */
#define DMA_INT_NI             0x00010000       // Normal interrupt summary
#define DMA_INT_AI             0x00008000       // Abnormal interrupt summary
#define DMA_INT_ER             0x00004000       // Early receive interrupt
#define DMA_INT_FB             0x00002000       // Fatal bus error
#define DMA_INT_ET             0x00000400       // Early transmit interrupt
#define DMA_INT_RW             0x00000200       // Receive watchdog timeout
#define DMA_INT_RS             0x00000100       // Receive stop
#define DMA_INT_RU             0x00000080       // Receive buffer unavailble
#define DMA_INT_RI             0x00000040       // Receive interrupt
#define DMA_INT_UN             0x00000020       // Underflow
#define DMA_INT_TJ             0x00000008       // Transmit jabber timeout
#define DMA_INT_TU             0x00000004       // Transmit buffer unavailble
#define DMA_INT_TS             0x00000002       // Transmit stop
#define DMA_INT_TI             0x00000001       // Transmit interrupt

/*
 * Receive Descriptor Bit Summary
 */
#define R_OWN      0x80000000       /* Own Bit */
#define RD_FF      0x40000000       /* Filtering Fail */
#define RD_FL      0x3fff0000       /* Frame Length */
#define RD_ES      0x00008000       /* Error Summary */
#define RD_DE      0x00004000       /* Descriptor Error */
#define RD_LE      0x00001000       /* Length Error */
#define RD_RF      0x00000800       /* Runt Frame */
#define RD_MF      0x00000400       /* Multicast Frame */
#define RD_FS      0x00000200       /* First Descriptor */
#define RD_LS      0x00000100       /* Last Descriptor */
#define RD_TL      0x00000080       /* Frame Too Long */
#define RD_CS      0x00000040       /* Collision Seen */
#define RD_FT      0x00000020       /* Frame Type */
#define RD_RJ      0x00000010       /* Receive Watchdog timeout*/
#define RD_RE      0x00000008       /* Report on MII Error */
#define RD_DB      0x00000004       /* Dribbling Bit */
#define RD_CE      0x00000002       /* CRC Error */

#define RD_RER     0x02000000       /* Receive End Of Ring */
#define RD_RCH     0x01000000       /* Second Address Chained */
#define RD_RBS2    0x003ff800       /* Buffer 2 Size */
#define RD_RBS1    0x000007ff       /* Buffer 1 Size */

/*
 * Transmit Descriptor Bit Summary
 */
#define T_OWN      0x80000000       /* Own Bit */
#define TD_ES      0x00008000       /* Frame Aborted (error summary)*/
#define TD_LO      0x00000800       /* Loss Of Carrier */
#define TD_NC      0x00000400       /* No Carrier */
#define TD_LC      0x00000200       /* Late Collision */
#define TD_EC      0x00000100       /* Excessive Collisions */
#define TD_HF      0x00000080       /* Heartbeat Fail */
#define TD_CC      0x0000003c       /* Collision Counter */
#define TD_UF      0x00000002       /* Underflow Error */
#define TD_DE      0x00000001       /* Deferred */

#define TD_IC      0x80000000       /* Interrupt On Completion */
#define TD_LS      0x40000000       /* Last Segment */
#define TD_FS      0x20000000       /* First Segment */
#define TD_FT1     0x10000000       /* Filtering Type */
#define TD_SET     0x08000000       /* Setup Packet */
#define TD_AC      0x04000000       /* Add CRC Disable */
#define TD_TER     0x02000000       /* Transmit End Of Ring */
#define TD_TCH     0x01000000       /* Second Address Chained */
#define TD_DPD     0x00800000       /* Disabled Padding */
#define TD_FT0     0x00400000       /* Filtering Type */
#define TD_TBS2    0x003ff800       /* Buffer 2 Size */
#define TD_TBS1    0x000007ff       /* Buffer 1 Size */

#define PERFECT_F  0x00000000
#define HASH_F     TD_FT0
#define INVERSE_F  TD_FT1
#define HASH_O_F   (TD_FT1 | TD_F0)

/*
 * Constant setting
 */

#define IMR_DEFAULT    (DMA_INT_TI | DMA_INT_RI |	\
			 DMA_INT_TS | DMA_INT_RS |	\
			 DMA_INT_TU | DMA_INT_RU |	\
			 DMA_INT_FB)

#define IMR_ENABLE     (DMA_INT_NI | DMA_INT_AI)

#define CRC_POLYNOMIAL_BE 0x04c11db7UL  /* Ethernet CRC, big endian */
#define CRC_POLYNOMIAL_LE 0xedb88320UL  /* Ethernet CRC, little endian */

#define HASH_TABLE_LEN   512       /* Bits */
//#define HASH_BITS        0x01ff    /* 9 LS bits */

#define SETUP_FRAME_LEN  192       /* Bytes */
#define IMPERF_PA_OFFSET 156       /* Bytes */

/*
 * Address Filtering Modes
 */
#define PERFECT              0     /* 16 perfect physical addresses */
#define HASH_PERF            1     /* 1 perfect, 512 multicast addresses */
#define PERFECT_REJ          2     /* Reject 16 perfect physical addresses */
#define ALL_HASH             3     /* Hashes all physical & multicast addrs */

#define ALL                  0     /* Clear out all the setup frame */
#define PHYS_ADDR_ONLY       1     /* Update the physical address only */

/* MII register */
#define MII_BMCR       0x00          /* MII Basic Mode Control Register */
#define MII_BMSR       0x01          /* MII Basic Mode Status Register */
#define MII_ID1        0x02          /* PHY Identifier Register 1 */
#define MII_ID2        0x03          /* PHY Identifier Register 2 */
#define MII_ANAR       0x04          /* Auto Negotiation Advertisement Register */
#define MII_ANLPAR     0x05          /* Auto Negotiation Link Partner Ability */
#define MII_ANER       0x06          /* Auto Negotiation Expansion */
#define MII_DSCR       0x10          /* Davicom Specified Configuration Register */
#define MII_DSCSR      0x11          /* Davicom Specified Configuration/Status Register */
#define MII_10BTCSR    0x12          /* 10base-T Specified Configuration/Status Register */

#define MII_PREAMBLE 0xffffffff    /* MII Management Preamble */
#define MII_TEST     0xaaaaaaaa    /* MII Test Signal */
#define MII_STRD     0x06          /* Start of Frame+Op Code: use low nibble */
#define MII_STWR     0x0a          /* Start of Frame+Op Code: use low nibble */

/*
 * MII Management Control Register
 */
#define MII_CR_RST  0x8000         /* RESET the PHY chip */
#define MII_CR_LPBK 0x4000         /* Loopback enable */
#define MII_CR_SPD  0x2000         /* 0: 10Mb/s; 1: 100Mb/s */
#define MII_CR_ASSE 0x1000         /* Auto Speed Select Enable */
#define MII_CR_PD   0x0800         /* Power Down */
#define MII_CR_ISOL 0x0400         /* Isolate Mode */
#define MII_CR_RAN  0x0200         /* Restart Auto Negotiation */
#define MII_CR_FDM  0x0100         /* Full Duplex Mode */
#define MII_CR_CTE  0x0080         /* Collision Test Enable */

/*
 * MII Management Status Register
 */
#define MII_SR_T4C  0x8000         /* 100BASE-T4 capable */
#define MII_SR_TXFD 0x4000         /* 100BASE-TX Full Duplex capable */
#define MII_SR_TXHD 0x2000         /* 100BASE-TX Half Duplex capable */
#define MII_SR_TFD  0x1000         /* 10BASE-T Full Duplex capable */
#define MII_SR_THD  0x0800         /* 10BASE-T Half Duplex capable */
#define MII_SR_ASSC 0x0020         /* Auto Speed Selection Complete*/
#define MII_SR_RFD  0x0010         /* Remote Fault Detected */
#define MII_SR_ANC  0x0008         /* Auto Negotiation capable */
#define MII_SR_LKS  0x0004         /* Link Status */
#define MII_SR_JABD 0x0002         /* Jabber Detect */
#define MII_SR_XC   0x0001         /* Extended Capabilities */

/*
 * MII Management Auto Negotiation Advertisement Register
 */
#define MII_ANA_TAF  0x03e0        /* Technology Ability Field */
#define MII_ANA_T4AM 0x0200        /* T4 Technology Ability Mask */
#define MII_ANA_TXAM 0x0180        /* TX Technology Ability Mask */
#define MII_ANA_FDAM 0x0140        /* Full Duplex Technology Ability Mask */
#define MII_ANA_HDAM 0x02a0        /* Half Duplex Technology Ability Mask */
#define MII_ANA_100M 0x0380        /* 100Mb Technology Ability Mask */
#define MII_ANA_10M  0x0060        /* 10Mb Technology Ability Mask */
#define MII_ANA_CSMA 0x0001        /* CSMA-CD Capable */

/*
 * MII Management Auto Negotiation Remote End Register
 */
#define MII_ANLPA_NP   0x8000      /* Next Page (Enable) */
#define MII_ANLPA_ACK  0x4000      /* Remote Acknowledge */
#define MII_ANLPA_RF   0x2000      /* Remote Fault */
#define MII_ANLPA_TAF  0x03e0      /* Technology Ability Field */
#define MII_ANLPA_T4AM 0x0200      /* T4 Technology Ability Mask */
#define MII_ANLPA_TXAM 0x0180      /* TX Technology Ability Mask */
#define MII_ANLPA_FDAM 0x0140      /* Full Duplex Technology Ability Mask */
#define MII_ANLPA_HDAM 0x02a0      /* Half Duplex Technology Ability Mask */
#define MII_ANLPA_100M 0x0380      /* 100Mb Technology Ability Mask */
#define MII_ANLPA_10M  0x0060      /* 10Mb Technology Ability Mask */
#define MII_ANLPA_CSMA 0x0001      /* CSMA-CD Capable */

/*
 * MII Management DAVICOM Specified Configuration And Status Register
 */
#define MII_DSCSR_100FDX       0x8000  /* 100M Full Duplex Operation Mode */
#define MII_DSCSR_100HDX       0x4000  /* 100M Half Duplex Operation Mode */
#define MII_DSCSR_10FDX        0x2000  /* 10M  Full Duplex Operation Mode */
#define MII_DSCSR_10HDX        0x1000  /* 10M  Half Duplex Operation Mode */
#define MII_DSCSR_ANMB         0x000f  /* Auto-Negotiation Monitor Bits   */

/*
 * Used by IOCTL
 */
#define READ_COMMAND		(SIOCDEVPRIVATE + 4)
#define WRITE_COMMAND		(SIOCDEVPRIVATE + 5)
#define GETDRIVERINFO		(SIOCDEVPRIVATE + 6)

/*
 * Device data and structure
 */

#define ETH_TX_TIMEOUT		(6 * HZ)

#define RX_BUF_SIZE		1536

#define NUM_RX_DESCS		32
#define NUM_TX_DESCS		16

static const char *media_types[] = {
	"10BaseT-HD ", "10BaseT-FD ", "100baseTx-HD ",
	"100baseTx-FD", "100baseT4", 0
};

struct jz_desc {
	u32 status;
	u32 desc1;
	u32 buf1_addr;
	u32 next_addr;
};

struct jz_eth_private {
	struct device		*dev;
	struct platform_device	*pdev;
	struct net_device	*ndev;

	void *base;
#define ETH_BASE (np->base)

	struct jz_desc tx_ring[NUM_TX_DESCS];	/* transmit descriptors */
	struct jz_desc rx_ring[NUM_RX_DESCS];	/* receive descriptors */
	dma_addr_t dma_tx_ring;		/* bus address of tx ring */
	dma_addr_t dma_rx_ring;		/* bus address of rx ring */
	dma_addr_t dma_rx_buf;			/* DMA address of rx buffer  */
	unsigned int vaddr_rx_buf;		/* virtual address of rx buffer  */

	unsigned int rx_head;			/* first rx descriptor */
	unsigned int tx_head;			/* first tx descriptor */
	unsigned int tx_tail;		/* last unacked transmit packet */
	unsigned int tx_full;			/* transmit buffers are full */
	struct sk_buff *tx_skb[NUM_TX_DESCS];	/* skbuffs for packets to transmit */

	spinlock_t lock;

	int media;				/* Media (eg TP), mode (eg 100B)*/
	int full_duplex;			/* Current duplex setting. */
	int link_state;
	char phys[32];				/* List of attached PHY devices */
	char valid_phy;				/* Current linked phy-id with MAC */
	int mii_phy_cnt;
	int phy_type;				/* 1-RTL8309,0-DVCOM */
	struct ethtool_cmd ecmds[32];
	u16 advertising;			/* NWay media advertisement */

	struct delayed_work check_work;

	struct pm_dev *pmdev;
};

#define P2ADDR(a)	(((unsigned long)(a) & 0x1fffffff) | 0xa0000000)
#define P1ADDR(a)	(((unsigned long)(a) & 0x1fffffff) | 0x80000000)

#ifdef DEBUG
static void mii_db_out(struct net_device *dev);
#endif

#define DRV_NAME	"jz_eth"
#define DRV_VERSION	"1.2"
#define DRV_AUTHOR	"Peter Wei <jlwei@ingenic.cn>"
#define DRV_DESC	"JzSOC On-chip Ethernet driver"

/*
 * Local variables
 */
static char *hwaddr;
static int debug = -1;
static struct mii_if_info mii_info;

module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "debug flag");
module_param(hwaddr, charp, 0);
MODULE_PARM_DESC(hwaddr, "hardware MAC address");

/*
 * Local routines
 */
static irqreturn_t jz_eth_interrupt(int irq, void *dev_id);
static struct net_device_stats *jz_eth_get_stats(struct net_device *dev);
static void link_check_worker(struct work_struct *work);

/*
 * Get MAC address
 */

static inline unsigned char str2hexnum(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0; /* foo */
}

static bool str2eaddr(unsigned char *ea, const char *str)
{
	int i;

	for (i = 0; i < 6; i++) {
		unsigned char num;

		if ((*str == '.') || (*str == ':'))
			str++;
		if (!str[0] || !str[1])
			return false;	/* early end */
		num = str2hexnum(*str++) << 4;
		num |= (str2hexnum(*str++));
		ea[i] = num;
	}
	return true;
}

static char *ethaddr = "";
module_param(ethaddr, charp, 0);
MODULE_PARM_DESC(ethaddr, "ethernet address (hex)");

static int get_mac_address(struct net_device *dev)
{
	int i;
	unsigned char flag0 = 0;
	unsigned char flag1 = 0xff;
	struct jz_eth_private *np = netdev_priv(dev);
	char a[6];

	if (strlen(ethaddr) > 0 && str2eaddr(a, ethaddr)) {
		/* check whether valid MAC address */
		for (i = 0; i < 6; i++) {
			flag0 |= a[i];
			flag1 &= a[i];
		}
	}

	if ((a[0] & 0xc0) || flag0 == 0 || flag1 == 0xff) {
		char addr[] = { 0x00, 0xef, 0xa3, 0xc1, 0x00, 0x03 };
		dev_warn(np->dev, "There is no MAC valid address, use default ..\n");
		dev_addr_mod(dev, 0, addr, sizeof(addr));
	} else
		dev_addr_mod(dev, 0, a, sizeof(a));

	return 0;
}

/*---------------------------------------------------------------------*/

static u32 jz_eth_curr_mode(struct net_device *dev);

/*
 * Ethernet START/STOP routines
 */

static void start_eth(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	s32 val;

	val = readl(DMA_OMR);
	val |= OMR_ST | OMR_SR;
	writel(val, DMA_OMR);
}

static void stop_eth(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	s32 val;

	val = readl(DMA_OMR);
	val &= ~(OMR_ST | OMR_SR);
	writel(val, DMA_OMR);
}

/*
 * Link check routines
 */
static void start_check(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);

	if (!schedule_delayed_work(&np->check_work, 0))
		dev_err(np->dev, "unable to start kernel thread\n");
}

static int close_check(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);

	if (!cancel_delayed_work(&np->check_work))
		dev_err(np->dev, "unable to cancel thread\n");

	return 0;
}

static void link_check_worker(struct work_struct *work)
{
	struct jz_eth_private *np = container_of(work, struct jz_eth_private, check_work.work);
	struct net_device *dev = np->ndev;
	unsigned char current_link;

	current_link = mii_link_ok(&mii_info);
	if (np->link_state != current_link) {
		if (current_link) {
			dev_info(np->dev, "Ethernet Link OK!\n");
			jz_eth_curr_mode(dev);
			netif_carrier_on(dev);
		} else {
			dev_err(np->dev, "Ethernet Link offline!\n");
			netif_carrier_off(dev);
		}
	}
	np->link_state = current_link;

	jz_eth_get_stats(dev);

	schedule_delayed_work(&np->check_work, msecs_to_jiffies(200));
}

#ifdef DEBUG
/*
 * Display ethernet packet header
 * This routine is used for test function
 */
static void eth_dbg_rx(struct sk_buff *skb, int len)
{
	int i, j;

	printk("R: %02x:%02x:%02x:%02x:%02x:%02x <- %02x:%02x:%02x:%02x:%02x:%02x len/SAP:%02x%02x [%d]\n",
	       (u8)skb->data[0],
	       (u8)skb->data[1],
	       (u8)skb->data[2],
	       (u8)skb->data[3],
	       (u8)skb->data[4],
	       (u8)skb->data[5],
	       (u8)skb->data[6],
	       (u8)skb->data[7],
	       (u8)skb->data[8],
	       (u8)skb->data[9],
	       (u8)skb->data[10],
	       (u8)skb->data[11],
	       (u8)skb->data[12],
	       (u8)skb->data[13],
	       len);
	for (j = 0; len > 0; j += 16, len -= 16) {
		printk("    %03x: ", j);
		for (i = 0; i < 16 && i < len; i++)
			printk("%02x ", (u8)skb->data[i + j]);
		printk("\n");
	}
	return;
}
#endif

/*
 * Reset ethernet device
 */
static inline void jz_eth_reset(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	u32 i;

// use iopoll.h for this

	i = readl(DMA_BMR);
	writel(i | BMR_SWR, DMA_BMR);
	for (i = 0; i < 1000; i++) {
		if (!(readl(DMA_BMR) & BMR_SWR))
			break;
		mdelay(1);
	}
}

/*
 * MII operation routines
 */
static inline void mii_wait(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int i;

// use iopoll.h for this

	for (i = 0; i < 10000; i++) {
		if (!(readl(MAC_MIIA) & 0x1))
			break;
		mdelay(1);
	}
	if (i >= 10000)
		dev_err(np->dev, "MII wait timeout : %d.\n", i);
}

static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	struct jz_eth_private *np = netdev_priv(dev);
	u32 mii_cmd = (phy_id << 11) | (location << 6) | 1;
	int retval = 0;

	writel(mii_cmd, MAC_MIIA);
	mii_wait(dev);
	retval = readl(MAC_MIID) & 0x0000ffff;

	return retval;
}

static void mdio_write(struct net_device *dev, int phy_id, int location, int data)
{
	struct jz_eth_private *np = netdev_priv(dev);
	u32 mii_cmd = (phy_id << 11) | (location << 6) | 0x2 | 1;

	writel(mii_cmd, MAC_MIIA);
	writel(data & 0x0000ffff, MAC_MIID);
	mii_wait(dev);
}

/*
 * Search MII phy
 */
static int jz_search_mii_phy(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int phy, phy_idx = 0;

	np->valid_phy = 0xff;
	for (phy = 0; phy < 32; phy++) {
		int mii_status = mdio_read(dev, phy, 1);

		if (mii_status != 0xffff && mii_status != 0x0000) {
			np->phys[phy_idx] = phy;
			np->ecmds[phy_idx].speed = SPEED_100;
			np->ecmds[phy_idx].duplex = DUPLEX_FULL;
			np->ecmds[phy_idx].port = PORT_MII;
			np->ecmds[phy_idx].transceiver = XCVR_INTERNAL;
			np->ecmds[phy_idx].phy_address = np->phys[phy_idx];
			np->ecmds[phy_idx].autoneg = AUTONEG_ENABLE;
			np->ecmds[phy_idx].advertising = (ADVERTISED_10baseT_Half |
							ADVERTISED_10baseT_Full |
							ADVERTISED_100baseT_Half |
							ADVERTISED_100baseT_Full);
			dev_dbg(np->dev, "found PHY idx %d at %d\n", phy_idx, phy);
			phy_idx++;
		}
	}
	if (phy_idx == 1) {
		np->valid_phy = np->phys[0];
		np->phy_type = 0;
	}
	if (phy_idx != 0) {
		phy = np->valid_phy;
		np->advertising = mdio_read(dev, phy, 4);
	}
	return phy_idx;
}

/*
 * CRC calc for Destination Address for gets hashtable index
 */

#define POLYNOMIAL 0x04c11db7UL
static u16 jz_hashtable_index(u8 *addr)
{
	u32 crc = 0xffffffff, msb;
	int  i, j;
	u32  byte;

	for (i = 0; i < 6; i++) {
		byte = *addr++;
		for (j = 0; j < 8; j++) {
			msb = crc >> 31;
			crc <<= 1;
			if (msb ^ (byte & 1))
				crc ^= POLYNOMIAL;
			byte >>= 1;
		}
	}
	return ((int)(crc >> 26));
}

/*
 * Multicast filter and config multicast hash table
 */
#define MULTICAST_FILTER_LIMIT 64

static void jz_set_multicast_list(struct net_device *dev)
{
	int hash_index;
	u32 mcr, hash_h, hash_l, hash_bit;
	struct jz_eth_private *np = netdev_priv(dev);
#ifdef FIXME_DEBUG
	int j;
#endif

// spinlock like in drivers/net/ethernet/nxp/lpc_eth.c ?
	mcr = readl(MAC_MCR);
	mcr &= ~(MCR_PR | MCR_PM | MCR_HP);

	if (dev->flags & IFF_PROMISC) {
		/* Accept any kinds of packets */
		mcr |= MCR_PR;
		hash_h = 0xffffffff;
		hash_l = 0xffffffff;
		dev_dbg(np->dev, "enter promisc mode!\n");
	} else if ((dev->flags & IFF_ALLMULTI) || netdev_mc_count(dev) > MULTICAST_FILTER_LIMIT) {
		/* Accept all multicast packets */
		mcr |= MCR_PM;
		hash_h = 0xffffffff;
		hash_l = 0xffffffff;
		dev_dbg(np->dev, "enter allmulticast mode! %d entries\n", netdev_mc_count(dev));
	} else if (dev->flags & IFF_MULTICAST) {
		/* Update multicast hash table */
		struct netdev_hw_addr *ha;

		hash_h = readl(MAC_HTH);
		hash_l = readl(MAC_HTL);
		netdev_for_each_mc_addr(ha, dev) {
			hash_index = jz_hashtable_index(ha->addr);
			hash_bit = BIT(hash_index & 0x1f);
			if (hash_index > 0x1f)
				hash_h |= hash_bit;
			else
				hash_l |= hash_bit;
			dev_dbg(np->dev, "----------------------------\n");
#ifdef FIXME_DEBUG
// these fields do not exists in netdev_hw_addr
			for (j = 0; j < ha->addrlen; j++)
				dev_dbg(np->dev, "%2.2x:", ha->addr[j]);
			printk("\n");
			dev_dbg(np->dev, "dmi.addrlen => %d\n", mclist->dmi_addrlen);
			dev_dbg(np->dev, "dmi.users   => %d\n", mclist->dmi_users);
			dev_dbg(np->dev, "dmi.gusers  => %d\n", mclist->dmi_users);
#endif
		}
		mcr |= MCR_HP;
		dev_dbg(np->dev, "This is multicast hash table high bits [%4.4x]\n", readl(MAC_HTH));
		dev_dbg(np->dev, "This is multicast hash table low  bits [%4.4x]\n", readl(MAC_HTL));
		dev_dbg(np->dev, "enter multicast mode!\n");
	}
	writel(hash_h, MAC_HTH);
	writel(hash_l, MAC_HTL);
	writel(mcr, MAC_MCR);
// spinunlock?
}

static inline int jz_phy_reset(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	unsigned int mii_reg0;
	unsigned int count;

	mii_reg0 = mdio_read(dev, np->valid_phy, MII_BMCR);
	mii_reg0 |= MII_CR_RST;
	mdio_write(dev, np->valid_phy, MII_BMCR, mii_reg0);  /* reset phy */

// can we use iopoll.h?
	for (count = 0; count < 1000; count++) {
		mdelay(1);
		mii_reg0 = mdio_read(dev, np->valid_phy, MII_BMCR);
		if (!(mii_reg0 & MII_CR_RST))
			break;  /* reset completed */
	}
	return (count >= 100);	/* phy error */
}

/*
 * Show all mii registers  -  this routine is used for test
 */
#ifdef DEBUG
static void mii_db_out(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	unsigned int mii_test;

	mii_test = mdio_read(dev, np->valid_phy, MII_BMCR);
	dev_dbg(np->dev, "BMCR ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, MII_BMSR);
	dev_dbg(np->dev, "BMSR ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, MII_ANAR);
	dev_dbg(np->dev, "ANAR ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, MII_ANLPAR);
	dev_dbg(np->dev, "ANLPAR ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, 16);
	dev_dbg(np->dev, "REG16 ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, 17);
	dev_dbg(np->dev, "REG17 ====> 0x%4.4x\n", mii_test);

	mii_test = mdio_read(dev, np->valid_phy, 2);
	dev_dbg(np->dev, "ID2 ====> 0x%4.4x\n", mii_test);
	mii_test = mdio_read(dev, np->valid_phy, 3);
	dev_dbg(np->dev, "ID3 ====> 0x%4.4x\n", mii_test);
}
#endif

/*
 * Start Auto-Negotiation function for PHY
 */
static int jz_autonet_complete(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int count;
	u32 mii_reg1, timeout = 3000;

// can we use iopoll.h???
	for (count = 0; count < timeout; count++) {
		mdelay(1);
		mii_reg1 = mdio_read(dev, np->valid_phy, MII_BMSR);
		if (mii_reg1 & 0x0020)
			break;
	}
	//mii_db_out(dev);  /* for debug to display all register of MII */
	return (count >= timeout);	/* auto negotiation error */
}

/*
 * Get current mode of eth phy
 */
static u32 jz_eth_curr_mode(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	unsigned int mii_reg17;
	u32 flag = 0;

	mii_reg17 = mdio_read(dev, np->valid_phy, MII_DSCSR);
	np->media = mii_reg17 >> 12;

	switch (np->media) {
	case 8:
		dev_info(np->dev, "Current Operation Mode is [100M Full Duplex]\n");
		flag = 0;
		np->full_duplex = 1;
		break;
	case 4:
		dev_info(np->dev, "Current Operation Mode is [100M Half Duplex]\n");
		flag = 0;
		np->full_duplex = 0;
		break;
	case 2:
		dev_info(np->dev, "Current Operation Mode is [10M Full Duplex]\n");
		flag = OMR_TTM;
		np->full_duplex = 1;
		break;
	case 1:
		dev_info(np->dev, "Current Operation Mode is [10M Half Duplex]\n");
		flag = OMR_TTM;
		np->full_duplex = 0;
		break;
	}
	return flag;
}

/*
 * Ethernet device hardware init
 * This routine initializes the ethernet device hardware and PHY
 */
static int jz_init_hw(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	struct ethtool_cmd ecmd;
	u32 mcr, omr;
	u32 sts, flag = 0;
	int i;

	jz_eth_reset(dev);
	stop_eth(dev);

/* XXX testwise enabled */
#if 0
	/* mii operation */
	if (jz_phy_reset(dev)) {
		dev_err(np->dev, "PHY device do not reset!\n");
		return -EPERM;          // return operation not permitted
	}
#endif
	/* Set MAC address */
	writel(le32_to_cpu(*(unsigned long *)&dev->dev_addr[0]), MAC_MAL);
	writel(le32_to_cpu(*(unsigned long *)&dev->dev_addr[4]), MAC_MAH);
	dev_info(np->dev, "JZ On-Chip ethernet (MAC %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x, IRQ %d)\n",
		 dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
			dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5],
			dev->irq);

	np->mii_phy_cnt = jz_search_mii_phy(dev);
	dev_info(np->dev, "Found %d PHY on JZ MAC\n", np->mii_phy_cnt);
#ifdef DEBUG
	mii_db_out(dev);
#endif

	mii_info.phy_id = np->valid_phy;
	mii_info.dev = dev;
	mii_info.mdio_read = &mdio_read;
	mii_info.mdio_write = &mdio_write;

	ecmd.speed = SPEED_100;
	ecmd.duplex = DUPLEX_FULL;
	ecmd.port = PORT_MII;
	ecmd.transceiver = XCVR_INTERNAL;
	ecmd.phy_address = np->valid_phy;
	ecmd.autoneg = AUTONEG_ENABLE;

	//mii_ethtool_sset(&mii_info, &ecmd);
	if (jz_autonet_complete(dev))
		dev_err(np->dev, "Ethernet Module AutoNegotiation failed\n");
	mii_ethtool_gset(&mii_info, &ecmd);

	for (i = 0; i < 5; i++)
		if (ecmd.advertising & (1 << i))
			dev_info(np->dev, "Provide Mode: (%d)%s\n", i + 1, media_types[i]);

	flag = jz_eth_curr_mode(dev);

	/* Config OMR register */
	omr = readl(DMA_OMR) & ~OMR_TTM;
	omr |= flag;
	//omr |= OMR_OSF;
	omr |= OMR_SF;
	writel(omr, DMA_OMR);

	readl(DMA_MFC); /* through read operation to clear the register for 0x0000000 */
	/* Set the programmable burst length (value 1 or 4 is validate)*/
#if 0 /* __BIG_ENDIAN__ */
	writel(PBL_4 | DSL_0 | 0x100080, DMA_BMR);  /* DSL_0: see DESC_SKIP_LEN and DESC_ALIGN */
#else /* __LITTLE_ENDIAN__ */
	writel(PBL_4 | DSL_0, DMA_BMR);  /* DSL_0: see DESC_SKIP_LEN and DESC_ALIGN */
#endif
	/* Config MCR register*/
	mcr = (readl(MAC_MCR) & ~(MCR_PS | MCR_HBD | MCR_FDX));
	if (np->full_duplex)
		mcr |= MCR_FDX;
	mcr |= MCR_BFD | MCR_TE | MCR_RE | MCR_OWD | MCR_HBD;
	writel(mcr, MAC_MCR);
//	mcr &= (readl(MAC_MCR) & ~(MCR_PM | MCR_PR | MCR_IF | MCR_HO | MCR_HP));
//	mcr &= 0xffdf;
//	mcr |= 0x0020;
//	writel(mcr, MAC_MCR);

	/* Set base address of TX and RX descriptors */
	writel(np->dma_rx_ring, DMA_RRBA);
	writel(np->dma_tx_ring, DMA_TRBA);

	start_eth(dev);

	/* set interrupt mask */
	writel(IMR_DEFAULT | IMR_ENABLE, DMA_IMR);

	/* Reset any pending (stale) interrupts */
	sts = readl(DMA_STS);
	writel(sts, DMA_STS);

	return 0;
}

static int jz_eth_open(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int retval, i;

	retval = devm_request_irq(&dev->dev, dev->irq, jz_eth_interrupt, IRQF_SHARED, dev->name, dev);
	if (retval) {
		dev_err(&dev->dev, "unable to get IRQ %d -> %d.\n", dev->irq, retval);
		return -EINVAL;
	}

	for (i = 0; i < NUM_RX_DESCS; i++) {
		np->rx_ring[i].status = cpu_to_le32(R_OWN);
		np->rx_ring[i].desc1 = cpu_to_le32(RX_BUF_SIZE | RD_RCH);
		np->rx_ring[i].buf1_addr = cpu_to_le32(np->dma_rx_buf + i * RX_BUF_SIZE);
		np->rx_ring[i].next_addr = cpu_to_le32(np->dma_rx_ring + (i + 1) * sizeof(struct jz_desc));
	}
	np->rx_ring[NUM_RX_DESCS - 1].next_addr = cpu_to_le32(np->dma_rx_ring);

	for (i = 0; i < NUM_TX_DESCS; i++) {
		np->tx_ring[i].status = cpu_to_le32(0);
		np->tx_ring[i].desc1  = cpu_to_le32(TD_TCH);
		np->tx_ring[i].buf1_addr = 0;
		np->tx_ring[i].next_addr = cpu_to_le32(np->dma_tx_ring + (i + 1) * sizeof(struct jz_desc));
	}
	np->tx_ring[NUM_TX_DESCS - 1].next_addr = cpu_to_le32(np->dma_tx_ring);

	np->rx_head = 0;
	np->tx_head = 0;
	np->tx_tail = 0;

	jz_init_hw(dev);

// FIXME	dev->trans_start = jiffies;
	netif_start_queue(dev);
	start_check(dev);

	return 0;
}

static int jz_eth_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	close_check(dev);
	stop_eth(dev);
	disable_irq(dev->irq);
	free_irq(dev->irq, dev);

	return 0;
}

/*
 * Get the current statistics.
 * This may be called with the device open or closed.
 */
static struct net_device_stats *jz_eth_get_stats(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int tmp;

	tmp = readl(DMA_MFC); /* After read clear to zero */
	np->ndev->stats.rx_missed_errors += (tmp & MFC_CNT2) + ((tmp & MFC_CNT1) >> 16);

	return &np->ndev->stats;
}

/*
 * ethtool routines
 */
static int jz_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
	struct jz_eth_private *np = netdev_priv(dev);
	u32 ethcmd;

	/* dev_ioctl() in ../../net/core/dev.c has already checked
	   capable(CAP_NET_ADMIN), so don't bother with that here.  */

	if (get_user(ethcmd, (u32 *)useraddr))
		return -EFAULT;

	switch (ethcmd) {
	case ETHTOOL_GDRVINFO: {
		struct ethtool_drvinfo info = { ETHTOOL_GDRVINFO };

		strcpy(info.driver, DRV_NAME);
		strcpy(info.version, DRV_VERSION);
		strcpy(info.bus_info, "OCS");
		if (copy_to_user(useraddr, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	/* get settings */
	case ETHTOOL_GSET: {
		struct ethtool_cmd ecmd = { ETHTOOL_GSET };

		spin_lock_irq(&np->lock);
		mii_ethtool_gset(&mii_info, &ecmd);
		spin_unlock_irq(&np->lock);
		if (copy_to_user(useraddr, &ecmd, sizeof(ecmd)))
			return -EFAULT;
		return 0;
	}
	/* set settings */
	case ETHTOOL_SSET: {
		int r;
		struct ethtool_cmd ecmd;

		if (copy_from_user(&ecmd, useraddr, sizeof(ecmd)))
			return -EFAULT;
		spin_lock_irq(&np->lock);
		r = mii_ethtool_sset(&mii_info, &ecmd);
		spin_unlock_irq(&np->lock);
		return r;
	}
	/* restart autonegotiation */
	case ETHTOOL_NWAY_RST: {
		return mii_nway_restart(&mii_info);
	}
	/* get link status */
	case ETHTOOL_GLINK: {
		struct ethtool_value edata = {ETHTOOL_GLINK};

		edata.data = mii_link_ok(&mii_info);
		if (copy_to_user(useraddr, &edata, sizeof(edata)))
			return -EFAULT;
		return 0;
	}

	/* get message-level */
	case ETHTOOL_GMSGLVL: {
		struct ethtool_value edata = {ETHTOOL_GMSGLVL};

		edata.data = debug;
		if (copy_to_user(useraddr, &edata, sizeof(edata)))
			return -EFAULT;
		return 0;
	}
	/* set message-level */
	case ETHTOOL_SMSGLVL: {
		struct ethtool_value edata;

		if (copy_from_user(&edata, useraddr, sizeof(edata)))
			return -EFAULT;
		debug = edata.data;
		return 0;
	}

	default:
		break;
	}

	return -EOPNOTSUPP;
}

/*
 * Config device
 */
static int jz_eth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct jz_eth_private *np = netdev_priv(dev);
	struct mii_ioctl_data *data, rdata;

	switch (cmd) {
	case SIOCETHTOOL:
		return jz_ethtool_ioctl(dev, (void *)rq->ifr_data);
	case SIOCGMIIPHY:
	case SIOCDEVPRIVATE:
		data = (struct mii_ioctl_data *)&rq->ifr_data;
		data->phy_id = np->valid_phy;
	case SIOCGMIIREG:
	case SIOCDEVPRIVATE + 1:
		data = (struct mii_ioctl_data *)&rq->ifr_data;
		data->val_out = mdio_read(dev, np->valid_phy, data->reg_num & 0x1f);
		return 0;
	case SIOCSMIIREG:
	case SIOCDEVPRIVATE + 2:
		data = (struct mii_ioctl_data *)&rq->ifr_data;
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		mdio_write(dev, np->valid_phy, data->reg_num & 0x1f, data->val_in);
		return 0;
	case READ_COMMAND:
		data = (struct mii_ioctl_data *)rq->ifr_data;
		if (copy_from_user(&rdata, data, sizeof(rdata)))
			return -EFAULT;
		rdata.val_out = mdio_read(dev, rdata.phy_id, rdata.reg_num & 0x1f);
		if (copy_to_user(data, &rdata, sizeof(rdata)))
			return -EFAULT;
		return 0;
	case WRITE_COMMAND:
		if (np->phy_type == 1) {
			data = (struct mii_ioctl_data *)rq->ifr_data;
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;
			if (copy_from_user(&rdata, data, sizeof(rdata)))
				return -EFAULT;
			mdio_write(dev, rdata.phy_id, rdata.reg_num & 0x1f, rdata.val_in);
		}
		return 0;
	case GETDRIVERINFO:
		if (np->phy_type == 1) {
			data = (struct mii_ioctl_data *)rq->ifr_data;
			if (copy_from_user(&rdata, data, sizeof(rdata)))
				return -EFAULT;
			rdata.val_in = 0x1;
			rdata.val_out = 0x00d0;
			if (copy_to_user(data, &rdata, sizeof(rdata)))
				return -EFAULT;
		}
		return 0;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/*
 * Received one packet
 */
static void eth_rxready(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	struct sk_buff *skb;
	unsigned char *pkt_ptr;
	u32 pkt_len;
	u32 status;

	status = le32_to_cpu(np->rx_ring[np->rx_head].status);
	while (!(status & R_OWN)) {			/* owner bit = 0 */
		if (status & RD_ES) {			/* error summary */
			np->ndev->stats.rx_errors++;		/* Update the error stats. */
			if (status & (RD_RF | RD_TL))
				np->ndev->stats.rx_frame_errors++;
			if (status & RD_CE)
				np->ndev->stats.rx_crc_errors++;
			if (status & RD_TL)
				np->ndev->stats.rx_length_errors++;
		} else {
			pkt_ptr = bus_to_virt(le32_to_cpu(np->rx_ring[np->rx_head].buf1_addr));
			pkt_len = ((status & RD_FL) >> 16) - 4;

			skb = dev_alloc_skb(pkt_len + 2);
			if (!skb) {
				dev_warn(np->dev, "Memory squeeze, dropping.\n");
				np->ndev->stats.rx_dropped++;
				break;
			}
			skb->dev = dev;
			skb_reserve(skb, 2); /* 16 byte align */

			//pkt_ptr = P1ADDR(pkt_ptr);
			//dma_cache_inv(pkt_ptr, pkt_len);
			memcpy(skb->data, pkt_ptr, pkt_len);
			skb_put(skb, pkt_len);

#ifdef DEBUG
#if 0
			eth_dbg_rx(skb, pkt_len);
#endif
#endif
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);	/* pass the packet to upper layers */
// FIXME:			dev->last_rx = jiffies;
			np->ndev->stats.rx_packets++;
			np->ndev->stats.rx_bytes += pkt_len;
		}
		np->rx_ring[np->rx_head].status = cpu_to_le32(R_OWN);

		np->rx_head++;
		if (np->rx_head >= NUM_RX_DESCS)
			np->rx_head = 0;
		status = le32_to_cpu(np->rx_ring[np->rx_head].status);
	}
}

/*
 * Tx timeout routine
 */
static void jz_eth_tx_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct jz_eth_private *np = netdev_priv(dev);

// spinlock?

	jz_init_hw(dev);
	np->ndev->stats.tx_errors++;
	netif_wake_queue(dev);
}

/*
 * One packet was transmitted
 */
static void eth_txdone(struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int tx_tail = np->tx_tail;

	while (tx_tail != np->tx_head) {
		int entry = tx_tail % NUM_TX_DESCS;
		s32 status = le32_to_cpu(np->tx_ring[entry].status);

		if (status < 0)
			break;
		if (status & TD_ES) {       /* Error summary */
			np->ndev->stats.tx_errors++;
			if (status & TD_NC)
				np->ndev->stats.tx_carrier_errors++;
			if (status & TD_LC)
				np->ndev->stats.tx_window_errors++;
			if (status & TD_UF)
				np->ndev->stats.tx_fifo_errors++;
			if (status & TD_DE)
				np->ndev->stats.tx_aborted_errors++;
			if (np->tx_head != np->tx_tail)
				writel(1, DMA_TPD);  /* Restart a stalled TX */
		} else {
			np->ndev->stats.tx_packets++;
		}

		/* Update the collision counter */
		np->ndev->stats.collisions += ((status & TD_EC) ? 16 : ((status & TD_CC) >> 3));

		/* Free the original skb */
		if (np->tx_skb[entry]) {
			dev_kfree_skb_irq(np->tx_skb[entry]);
			np->tx_skb[entry] = 0;
		}
		tx_tail++;
	}
	if (np->tx_full && (tx_tail + NUM_TX_DESCS > np->tx_head + 1)) {
		/* The ring is no longer full */
		np->tx_full = 0;
		netif_start_queue(dev);
	}
	np->tx_tail = tx_tail;
}

/*
 * Update the tx descriptor
 */
static void load_tx_packet(struct net_device *dev, unsigned char *buf, u32 flags, struct sk_buff *skb)
{
	struct jz_eth_private *np = netdev_priv(dev);
	int entry = np->tx_head % NUM_TX_DESCS;

	np->tx_ring[entry].buf1_addr = cpu_to_le32(virt_to_phys(buf));
	np->tx_ring[entry].desc1 &= cpu_to_le32((TD_TER | TD_TCH));
	np->tx_ring[entry].desc1 |= cpu_to_le32(flags);
	np->tx_ring[entry].status = cpu_to_le32(T_OWN);
	np->tx_skb[entry] = skb;
}

/*
 * Transmit one packet
 */
static int jz_eth_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct jz_eth_private *np = netdev_priv(dev);
	u32 length;

	if (np->tx_full)
		return 0;

	udelay(500);	/* FIXME: can we remove this delay ? */
	length = (skb->len < ETH_ZLEN) ? ETH_ZLEN : skb->len;
// FIXME:	dma_sync_single_for_device(np->dev, (dma_addr_t) skb->data, length, DMA_BIDIRECTIONAL);	// 1. kernel panics, 2. DMA_TO_DEVICE?
	load_tx_packet(dev, skb->data, TD_IC | TD_LS | TD_FS | length, skb);
	spin_lock_irq(&np->lock);
	np->tx_head++;
	np->ndev->stats.tx_bytes += length;
	writel(1, DMA_TPD);		/* Start the TX */

	netif_trans_update(dev);
	if (np->tx_tail + NUM_TX_DESCS > np->tx_head + 1) {
		np->tx_full = 0;
	} else {
		np->tx_full = 1;
		netif_stop_queue(dev);
	}
	spin_unlock_irq(&np->lock);

	return NETDEV_TX_OK;
}

/*
 * Interrupt service routine
 */
static irqreturn_t jz_eth_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct jz_eth_private *np = netdev_priv(dev);
	u32 sts;
	int i;

	spin_lock(&np->lock);

	writel((readl(DMA_IMR) & ~IMR_ENABLE), DMA_IMR); /* Disable interrupt */

	for (i = 0; i < 100; i++) {
		sts = readl(DMA_STS);
		writel(sts, DMA_STS);	/* clear status */

		if (!(sts & IMR_DEFAULT))
			break;

		if (sts & (DMA_INT_RI | DMA_INT_RU)) /* Rx IRQ */
			eth_rxready(dev);
		if (sts & (DMA_INT_TI | DMA_INT_TU)) /* Tx IRQ */
			eth_txdone(dev);

		/* check error conditions */
		if (sts & DMA_INT_FB) {      /* fatal bus error */
			stop_eth(dev);
			dev_err(np->dev, "Fatal bus error occurred, sts=%#8x, device stopped.\n", sts);
			break;
		}

		if (sts & DMA_INT_UN) {     /* Transmit underrun */
			u32 omr;

			omr = readl(DMA_OMR);
			if (!(omr & OMR_SF)) {
				omr &= ~(OMR_ST | OMR_SR);
				writel(omr, DMA_OMR);
				while (readl(DMA_STS) & STS_TS)
					;  /* wait for stop */
				if ((omr & OMR_TR) < OMR_TR) {  /* ? */
					omr += TR_24;
				} else {
					omr |= OMR_SF;
				}
				writel(omr | OMR_ST | OMR_SR, DMA_OMR);
			}
		}
	}

	writel(readl(DMA_IMR) | IMR_ENABLE, DMA_IMR); /* enable interrupt */

	spin_unlock(&np->lock);

	return IRQ_HANDLED;
}

#if 0 //def CONFIG_PM
/*
 * Suspend the ETH interface.
 */
static int jz_eth_suspend(struct net_device *dev, int state)
{
	struct jz_eth_private *jep = netdev_priv(dev);
	unsigned long flags, tmp;

	dev_info(np->dev, "ETH suspend.\n");

	if (!netif_running(dev))
		return 0;

	netif_device_detach(dev);

	spin_lock_irqsave(&jep->lock, flags);

	/* Disable interrupts, stop Tx and Rx. */
// FIXME:
	REG32(DMA_IMR) = 0;
	stop_eth(dev);

	/* Update the error counts. */
// FIXME:
	tmp = REG32(DMA_MFC);
	jep->stats.rx_missed_errors += (tmp & 0x1ffff);
	jep->stats.rx_fifo_errors += ((tmp >> 17) & 0x7ff);

	spin_unlock_irqrestore(&jep->lock, flags);

	return 0;
}

/*
 * Resume the ETH interface.
 */
static int jz_eth_resume(struct net_device *dev)
{
	dev_info(np->dev, "ETH resume.\n");

	if (!netif_running(dev))
		return 0;

	jz_init_hw(dev);

	netif_device_attach(dev);
	jz_eth_tx_timeout(dev);
	netif_wake_queue(dev);

	return 0;
}

static int jz_eth_pm_callback(struct pm_dev *dev, pm_request_t rqst, void *data)
{
	int ret;

	if (!dev->data)
		return -EINVAL;

	switch (rqst) {
	case PM_SUSPEND:
		ret = jz_eth_suspend((struct net_device *)dev->data,
				     (int)data);
		break;

	case PM_RESUME:
		ret = jz_eth_resume((struct net_device *)dev->data);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

#endif /* CONFIG_PM */

static const struct net_device_ops jz4730_eth_ops = {
	.ndo_open		= jz_eth_open,
	.ndo_stop		= jz_eth_close,
	.ndo_start_xmit		= jz_eth_send_packet,
	.ndo_tx_timeout		= jz_eth_tx_timeout,
	.ndo_set_rx_mode	= jz_set_multicast_list,
	.ndo_do_ioctl		= jz_eth_ioctl,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= NULL,
};

static int jz4730_eth_probe(struct platform_device *pdev)
{
	struct device_node *of = pdev->dev.of_node;
	struct net_device *dev;
	struct jz_eth_private *np;
	int err;

	dev = alloc_etherdev(sizeof(struct jz_eth_private));
	if (!dev) {
		dev_err(&pdev->dev, "alloc_etherdev failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	np = netdev_priv(dev);
	memset(np, 0, sizeof(struct jz_eth_private));

	np->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(np->base)) {
		free_netdev(dev);
		return PTR_ERR(np->base);
	}

	np->dev = &pdev->dev;
	np->ndev = dev;
	np->pdev = pdev;

/* should this become like	map = dma_map_page(priv->dev, page, 0, RX_BUF_SIZE, DMA_FROM_DEVICE); or similar? */

	np->vaddr_rx_buf = (u32)dma_alloc_noncoherent(&pdev->dev, NUM_RX_DESCS * RX_BUF_SIZE,
						      &np->dma_rx_buf, DMA_BIDIRECTIONAL, GFP_KERNEL);

	if (!np->vaddr_rx_buf) {
		dev_err(&pdev->dev, "Cannot alloc dma buffers\n");
		unregister_netdev(dev);
		free_netdev(dev);
		return -ENOMEM;
	}

	np->dma_rx_ring = virt_to_bus(np->rx_ring);
	np->dma_tx_ring = virt_to_bus(np->tx_ring);
	np->full_duplex = 1;
	np->link_state = 1;
	INIT_DELAYED_WORK(&np->check_work, link_check_worker);

	spin_lock_init(&np->lock);

	ether_setup(dev);

	dev->irq = irq_of_parse_and_map(of, 0);
	if (dev->irq <= 0) {
		dev_err(&pdev->dev, "unable to parse IRQ, error %d\n", err);
		free_netdev(dev);
		return dev->irq;
	}

	dev->netdev_ops = &jz4730_eth_ops;
// dev->ethtool_ops = ?
	dev->watchdog_timeo = ETH_TX_TIMEOUT;

	/* configure MAC address */
	get_mac_address(dev);

	err = register_netdev(dev);
	if (err) {
		dev_err(&pdev->dev, "Cannot register net device, error %d\n", err);
		free_netdev(dev);
		return -ENOMEM;
	}

//#ifdef 0 //CONFIG_PM
//	np->pmdev = pm_register(PM_SYS_DEV, PM_SYS_UNKNOWN, jz_eth_pm_callback);
//	if (np->pmdev)
//		np->pmdev->data = dev;
//#endif

	return 0;
}

static int jz4730_eth_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct jz_eth_private *np = netdev_priv(dev);

	unregister_netdev(dev);
	dma_free_noncoherent(NULL, NUM_RX_DESCS * RX_BUF_SIZE,
			     (void *)np->vaddr_rx_buf, np->dma_rx_buf,
			     DMA_BIDIRECTIONAL);
	free_netdev(dev);
	return 0;
}

static const struct of_device_id jz4730_eth_match[] = {
	{ .compatible = "ingenic,jz4730-ethernet" },
	{}
};

MODULE_DEVICE_TABLE(of, jz4730_eth_match);

static struct platform_driver jz4730_eth_driver = {
	.probe		= jz4730_eth_probe,
	.remove		= jz4730_eth_remove,
	.driver = {
		.name	= "jz4730-eth",
		.of_match_table = jz4730_eth_match,
	}
};

MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");

module_platform_driver(jz4730_eth_driver);
