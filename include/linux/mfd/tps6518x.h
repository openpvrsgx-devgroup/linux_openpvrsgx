/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __LINUX_REGULATOR_TPS6518x_H_
#define __LINUX_REGULATOR_TPS6518x_H_


#define TPS65185_VDROP_PROC_IN_KERNEL		1


/*
 * currently supported rev IDs
 */
//#define TPS65180_PASS1 0x54
//#define TPS65181_PASS1 0x55
#define TPS65180_PASS1 0x50
#define TPS65181_PASS1 0x51
#define TPS65180_PASS2 0x60
#define TPS65181_PASS2 0x61
#define TPS65185_PASS0 0x45
#define TPS65186_PASS0 0x46
#define TPS65185_PASS1 0x55
#define TPS65186_PASS1 0x56
#define TPS65185_PASS2 0x65
#define TPS65186_PASS2 0x66

/*
 * PMIC Register Addresses
 */
enum {
    REG_TPS6518x_TMST_VAL = 0x0,
    REG_TPS65185_ENABLE,
    REG_TPS65185_VADJ,
    REG_TPS65185_VCOM1,
    REG_TPS65185_VCOM2,
    REG_TPS65185_INT_EN1,
    REG_TPS65185_INT_EN2,
    REG_TPS65185_INT1,
    REG_TPS65185_INT2,
    REG_TPS65185_UPSEQ0,
    REG_TPS65185_UPSEQ1,
    REG_TPS65185_DWNSEQ0,
    REG_TPS65185_DWNSEQ1,
    REG_TPS65185_TMST1,
    REG_TPS65185_TMST2,
    REG_TPS6518x_PG,
    REG_TPS6518x_REVID,
    TPS6518x_REG_NUM,
};

enum {
    REG_TPS65180_TMST_VAL = 0x0,
    REG_TPS65180_ENABLE,
    REG_TPS65180_VP_ADJUST,
    REG_TPS65180_VN_ADJUST,
    REG_TPS65180_VCOM_ADJUST,
    REG_TPS65180_INT_EN1,
    REG_TPS65180_INT_EN2,
    REG_TPS65180_INT1,
    REG_TPS65180_INT2,
    REG_TPS65180_PWRSEQ0,
    REG_TPS65180_PWRSEQ1,
    REG_TPS65180_PWRSEQ2,
    REG_TPS65180_TMST_CONFIG,
    REG_TPS65180_TMST_OS,
    REG_TPS65180_TMST_HYST,
    REG_TPS65180_PG_STATUS,
    REG_TPS65180_REVID,
    REG_TPS65180_FIX_READ_PTR,
    TPS65180_REG_NUM,
};

#define TPS6518X_MAX_REG 0x12

/*
 * Bitfield macros that use rely on bitfield width/shift information.
 */
#define BITFMASK(field) (((1U << (field ## _WID)) - 1) << (field ## _LSH))
#define BITFVAL(field, val) ((val) << (field ## _LSH))
#define BITFEXT(var, bit) ((var & BITFMASK(bit)) >> (bit ## _LSH))

/*
 * Shift and width values for each register bitfield
 */
/* TMST_VALUE */
#define TMST_VALUE_LSH  0
#define TMST_VALUE_WID  8
/* ENABLE */
#define ACTIVE		BIT(7)
#define STANDBY     BIT(6)
#define V3P3_SW_EN  BIT(5)
#define VCOM_EN     BIT(4)
#define VDDH_EN BIT(3)
#define VPOS_EN BIT(2)
#define VEE_EN      BIT(1)
#define VNEG_EN     BIT(0)
/* VCOM_ADJUST */
#define VCOM_SET_LSH    0
#define VCOM_SET_WID    8
#define VCOM1_SET_LSH    0
#define VCOM1_SET_WID    8
#define VCOM2_SET_LSH    0
#define VCOM2_SET_WID    1
#define VCOM_ACQ_LSH	15
#define VCOM_ACQ_WID	1
#define VCOM_PROG_LSH	14
#define VCOM_PEOG_WID	1
#define VCOM_HiZ_LSH	13
#define VCOM_HiZ_WID	1
#define VCOM_AVG_LSH	11
#define VCOM_AVG_WID	2
/* INT_ENABLE1 */
#define TSD_EN_LSH        6
#define TSD_EN_WID        1
#define HOT_EN_LSH        5
#define HOT_EN_WID        1
#define TMST_HOT_EN_LSH   4
#define TMST_HOT_EN_WID   1
#define TMST_COOL_EN_LSH  3
#define TMST_COOL_EN_WID  1
#define UVLO_EN_LSH       2
#define UVLO_EN_WID       1
/* INT_ENABLE2 */
#define VB_UV_EN_LSH      7
#define VB_UV_EN_WID      1
#define VDDH_UV_EN_LSH    6
#define VDDH_UV_EN_WID    1
#define VN_UV_EN_LSH      5
#define VN_UV_EN_WID      1
#define VPOS_UV_EN_LSH    4
#define VPOS_UV_EN_WID    1
#define VEE_UV_EN_LSH     3
#define VEE_UV_EN_WID     1
#define VNEG_UV_EN_LSH    1
#define VNEG_UV_EN_WID    1
#define EOC_EN_LSH        0
#define EOC_EN_WID        1
/* INT_STATUS1 */
#define TSDN_LSH        6
#define TSDN_WID        1
#define HOT_LSH         5
#define HOT_WID         1
#define TMST_HOT_LSH    4
#define TMST_HOT_WID    1
#define TMST_COOL_LSH   3
#define TMST_COOL_WID   1
#define UVLO_LSH        2
#define UVLO_WID        1
/* INT_STATUS2 */
#define VB_UV_LSH       7
#define VB_UV_WID       1
#define VDDH_UV_LSH     6
#define VDDH_UV_WID     1
#define VN_UV_LSH       5
#define VN_UV_WID       1
#define VPOS_UV_LSH     4
#define VPOS_UV_WID     1
#define VEE_UV_LSH      3
#define VEE_UV_WID      1
#define VCOMF_LSH      2
#define VCOMF_WID      1
#define VNEG_UV_LSH     1
#define VNEG_UV_WID     1
#define EOC_LSH         0
#define EOC_WID         1
/* PWR_SEQ0 */
#define VDDH_SEQ_LSH    6
#define VDDH_SEQ_WID    2
#define VPOS_SEQ_LSH    4
#define VPOS_SEQ_WID    2
#define VEE_SEQ_LSH     2
#define VEE_SEQ_WID     2
#define VNEG_SEQ_LSH    0
#define VNEG_SEQ_WID    2
/* PWR_SEQ1 */
#define DLY1_LSH    4
#define DLY1_WID    4
#define DLY0_LSH    0
#define DLY0_WID    4
/* PWR_SEQ2 */
#define DLY3_LSH    4
#define DLY3_WID    4
#define DLY2_LSH    0
#define DLY2_WID    4
/* TMST_CONFIG */
#define READ_THERM_LSH      7
#define READ_THERM_WID      1
#define CONV_END_LSH        5
#define CONV_END_WID        1
#define FAULT_QUE_LSH       3
#define FAULT_QUE_WID       2
#define FAULT_QUE_CLR_LSH   2
#define FAULT_QUE_CLR_WID   1
/* TMST_OS */
#define TMST_HOT_SET_LSH    0
#define TMST_HOT_SET_WID    8
/* TMST_HYST */
#define TMST_COOL_SET_LSH   0
#define TMST_COOL_SET_WID   8
/* PG_STATUS */
#define VB_PG_LSH       7
#define VB_PG_WID       1
#define VDDH_PG_LSH     6
#define VDDH_PG_WID     1
#define VN_PG_LSH       5
#define VN_PG_WID       1
#define VPOS_PG_LSH     4
#define VPOS_PG_WID     1
#define VEE_PG_LSH      3
#define VEE_PG_WID      1
#define VNEG_PG_LSH     1
#define VNEG_PG_WID     1
/* REVID */
#define MJREV_LSH       6
#define MJREV_WID       2
#define MNREV_LSH       4
#define MNREV_WID       2
#define VERSION_LSH     0
#define VERSION_WID     4
/* FIX_READ_POINTER */
#define FIX_RD_PTR_LSH  0
#define FIX_RD_PTR_WID  1

/*
 * VCOM Definitions
 *
 * The register fields accept voltages in the range 0V to -2.75V, but the
 * VCOM parametric performance is only guaranteed from -0.3V to -2.5V.
 */
#define TPS65180_VCOM_MAX_uV   2750000
#define TPS65180_VCOM_MIN_uV          0
#define TPS65180_VCOM_MIN_SET         0
#define TPS65180_VCOM_MAX_SET       255
#define TPS65180_VCOM_BASE_uV     10740
#define TPS65180_VCOM_STEP_uV     10740
#define TPS65185_VCOM_MAX_uV   5110000
#define TPS65185_VCOM_MIN_uV          0
#define TPS65185_VCOM_MIN_SET         0
#define TPS65185_VCOM_MAX_SET       511
#define TPS65185_VCOM_BASE_uV     10000
#define TPS65185_VCOM_STEP_uV     10000



#define TPS6518x_VCOM_MIN_VAL         0
#define TPS6518x_VCOM_MAX_VAL       255

#define TPS6518X_WAKEUP_MS	3
#define TPS6518X_WAKEUP_WAIT_TICKS	1


struct regulator_init_data;

typedef void *(TPS65185_INTEVT_CB)(int iEVENT);

struct tps6518x {
	/* chip revision */
	int revID;

	struct device *dev;
	struct tps6518x_platform_data *pdata;

	/* Platform connection */
	struct regmap *regmap;
	/* struct i2c_client *i2c_client; */

	/* Timings */
	unsigned int pwr_seq0;
	unsigned int pwr_seq1;
	unsigned int pwr_seq2;
	unsigned int upseq0;
	unsigned int upseq1;
	unsigned int dwnseq0;
	unsigned int dwnseq1;

	/* GPIOs */
	struct gpio_desc * gpio_pmic_pwrgood;
	struct gpio_desc * gpio_pmic_vcom_ctrl;
	struct gpio_desc * gpio_pmic_wakeup;
	struct gpio_desc * gpio_pmic_intr;
	struct gpio_desc * gpio_pmic_powerup;
	struct gpio_desc * gpio_pmic_v3p3;
	struct gpio_desc * gpio_pmic_pwrall;

	/* TPS6518x part variables */
	int pass_num;
	int vcom_uV;

	/* One-time VCOM setup marker */
	bool vcom_setup;

	
	bool timing_need_restore;// flag to restore timing . 

	/* powerup/powerdown wait time */
	int max_wait;

	/* Dynamically determined polarity for PWRGOOD */
	int pwrgood_polarity;

	//int gpio_pmic_powerup_stat;
	//int gpio_pmic_wakeup_stat;
	//int gpio_pmic_pwrall_stat;

	//int iNeedReloadVCOM; // VCOM must be loaded again .
	//unsigned regVCOM1,regVCOM2;
	int iRegEnable; //current Enable register . 

	int int_state; // interrupt state .
	TPS65185_INTEVT_CB *pfnINTCB;
	struct work_struct int_work;
	struct workqueue_struct *int_workqueue;

	unsigned long dwSafeTickToCommunication;
};

enum {
    /* In alphabetical order */
    TPS6518x_DISPLAY, /* virtual master enable */
    TPS6518x_VCOM,
    TPS6518x_V3P3,
    TPS6518x_TMST,
    TPS6518x_NUM_REGULATORS,
};

/*
 * Declarations
 */
struct regulator_init_data;
struct tps6518x_regulator_data;

struct tps6518x_platform_data {
	unsigned int pwr_seq0;
	unsigned int pwr_seq1;
	unsigned int pwr_seq2;
	unsigned int upseq0;
	unsigned int upseq1;
	unsigned int dwnseq0;
	unsigned int dwnseq1;
	struct gpio_desc * gpio_pmic_pwrgood;
	struct gpio_desc * gpio_pmic_vcom_ctrl;
	struct gpio_desc * gpio_pmic_wakeup;
	struct gpio_desc * gpio_pmic_intr;
	struct gpio_desc * gpio_pmic_powerup;
	struct gpio_desc * gpio_pmic_pwrall;
	int pass_num;
	int vcom_uV;

	/* PMIC */
	struct tps6518x_regulator_data *regulators;
	int num_regulators;
};

struct tps6518x_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

/*
 * to_reg_val(): Creates a register value with new data
 *
 * Creates a new register value for a particular field.  The data
 * outside of the new field is not modified.
 *
 * @cur_reg: current value in register
 * @reg_mask: mask of field bits to be modified
 * @fld_val: new value for register field.
 */
static inline unsigned int to_reg_val(unsigned int cur_reg, unsigned int fld_mask,
							   unsigned int fld_val)
{
	return (cur_reg & (~fld_mask)) | fld_val;
}

int tps6518x_reg_write(struct tps6518x *tps6518x,int reg_num, const unsigned int reg_val);
int tps6518x_reg_read(struct tps6518x *tps6518x,int reg_num, unsigned int *reg_val);
int tps6518x_get_temperature(struct tps6518x *tps6518x,int *O_piTemperature);
int tps6518x_get_vcom(struct tps6518x *tps6518x,int *O_piVCOMmV);
int tps6518x_set_vcom(struct tps6518x *tps6518x,int iVCOMmV,int iIsWriteToFlash);
//int tps6518x_restore_vcom(struct tps6518x *tps6518x);

int tps6518x_int_state_clear(struct tps6518x *tps6518x);
int tps6518x_int_state_get(struct tps6518x *tps6518x);

int tps6518x_int_callback_setup(struct tps6518x *tps6518x,TPS65185_INTEVT_CB fnCB);

int tps6518x_chip_power(struct tps6518x *tps6518x,int iIsON,int iIsWakeup);

int tps6518x_setup_timings(struct tps6518x *tps6518x);
#endif
