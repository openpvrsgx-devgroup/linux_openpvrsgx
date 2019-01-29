/*
 * BNO055 - Bosch 9-axis orientation sensor
 *
 * Copyright (c) 2016, Intel Corporation.
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * TODO:
 *  - buffering
 *  - interrupt support
 *  - linear and gravitational acceleration (not supported in IIO)
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include <linux/iio/iio.h>

#define BNO055_DRIVER_NAME		"bno055"

#define BNO055_REG_CHIP_ID		0x00
#define BNO055_REG_PAGE_ID		0x07

#define BNO055_REG_ACC_DATA_X_LSB	0x08
#define BNO055_REG_MAG_DATA_X_LSB	0x0E
#define BNO055_REG_GYR_DATA_X_LSB	0x14
#define BNO055_REG_EUL_HEADING_LSB	0x1A
#define BNO055_REG_QUA_DATA_W_LSB	0x20
#define BNO055_REG_TEMP			0x34

#define BNO055_REG_SYS_ERR		0x3A
#define BNO055_REG_UNIT_SEL		0x3B

#define BNO055_REG_OPR_MODE		0x3D
#define BNO055_REG_AXIS_MAP_SIGN	0x42

#define BNO055_REG_ACC_OFFSET_X_LSB	0x55
#define BNO055_REG_MAG_OFFSET_X_LSB	0x5B
#define BNO055_REG_GYR_OFFSET_X_LSB	0x61
#define BNO055_REG_MAG_RADIUS_MSB	0x6A

/*
 * The difference in address between the register that contains the
 * value and the register that contains the offset.  This applies for
 * accel, gyro and magn channels.
 */
#define BNO055_REG_OFFSET_ADDR		0x4D

#define BNO055_OPR_MODE_MASK		GENMASK(3, 0)

/* Combination of BNO055 and individual chip IDs. */
#define BNO055_CHIP_ID			0x0F32FBA0

#define BNO055_ANDROID_ORIENTATION	BIT(7)
#define BNO055_TEMP_CELSIUS		0
#define BNO055_EUL_RADIANS		BIT(2)
#define BNO055_GYR_RADIANS		BIT(1)
#define BNO055_ACC_MPSS			0

/*
 * Operation modes.  It is important that these are listed in the order
 * they appear in the datasheet, as an index to this table is used to
 * write the actual bits in the operation config register.
 */
enum bno055_operation_mode {
	BNO055_CONFIG_MODE,

	/* Non-fusion modes. */
	BNO055_MODE_ACC_ONLY,
	BNO055_MODE_MAG_ONLY,
	BNO055_MODE_GYRO_ONLY,
	BNO055_MODE_ACC_MAG,
	BNO055_MODE_ACC_GYRO,
	BNO055_MODE_MAG_GYRO,
	BNO055_MODE_AMG,

	/* Fusion modes. */
	BNO055_MODE_IMU,
	BNO055_MODE_COMPASS,
	BNO055_MODE_M4G,
	BNO055_MODE_NDOF_FMC_OFF,
	BNO055_MODE_NDOF,

	BNO055_MODE_MAX,
};

/*
 * Number of channels for each operation mode.  See Table 3-3 in the
 * datasheet for a summary of each operation mode.  Each non-config mode
 * also supports a temperature channel.
 */
static const int bno055_num_channels[] = {
	0, 4, 4, 4, 7, 7, 7, 10,
	/*
	 * In fusion modes, data from the raw sensors is still
	 * available.  Additionally, the linear and gravitational
	 * components of acceleration are available in all fusion modes,
	 * but there are currently no IIO attributes for these.
	 *
	 * Orientation is exposed both as a quaternion multi-value
	 * (meaning a single channel) and as Euler angles (3 separate
	 * channels).
	 */
	11, 11, 11, 14, 14,
};

struct bno055_data {
	struct regmap *regmap;
	enum bno055_operation_mode op_mode;
	struct iio_mount_matrix orientation;
};

/*
 * Note: The BNO055 has two pages of registers.  All the addresses below
 * are page 0 addresses.  If the driver ever uses page 1 registers, it
 * is expected to manually switch between pages via the PAGE ID register
 * and make sure that no other transactions happen.  It also cannot use
 * the regmap interface for accessing registers in page 1.
 */
static const struct regmap_range bno055_writable_ranges[] = {
	regmap_reg_range(BNO055_REG_ACC_OFFSET_X_LSB, BNO055_REG_MAG_RADIUS_MSB),
	regmap_reg_range(BNO055_REG_OPR_MODE, BNO055_REG_AXIS_MAP_SIGN),
	regmap_reg_range(BNO055_REG_UNIT_SEL, BNO055_REG_UNIT_SEL),
	/* Listed as read-only in the datasheet, but probably an error. */
	regmap_reg_range(BNO055_REG_PAGE_ID, BNO055_REG_PAGE_ID),
};

static const struct regmap_access_table bno055_writable_regs = {
	.yes_ranges = bno055_writable_ranges,
	.n_yes_ranges = ARRAY_SIZE(bno055_writable_ranges),
};

/* Only reserved registers are non-readable. */
static const struct regmap_range bno055_non_readable_reg_ranges[] = {
	regmap_reg_range(BNO055_REG_AXIS_MAP_SIGN + 1, BNO055_REG_ACC_OFFSET_X_LSB - 1),
};

static const struct regmap_access_table bno055_readable_regs = {
	.no_ranges = bno055_non_readable_reg_ranges,
	.n_no_ranges = ARRAY_SIZE(bno055_non_readable_reg_ranges),
};

static const struct regmap_range bno055_volatile_reg_ranges[] = {
	regmap_reg_range(BNO055_REG_ACC_DATA_X_LSB, BNO055_REG_SYS_ERR),
};

static const struct regmap_access_table bno055_volatile_regs = {
	.yes_ranges = bno055_volatile_reg_ranges,
	.n_yes_ranges = ARRAY_SIZE(bno055_volatile_reg_ranges),
};

static const struct regmap_config bno055_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = BNO055_REG_MAG_RADIUS_MSB + 1,
	.cache_type = REGCACHE_RBTREE,

	.wr_table = &bno055_writable_regs,
	.rd_table = &bno055_readable_regs,
	.volatile_table = &bno055_volatile_regs,
};

static int bno055_read_simple_chan(struct iio_dev *indio_dev,
				   struct iio_chan_spec const *chan,
				   int *val, int *val2, long mask)
{
	struct bno055_data *data = iio_priv(indio_dev);
	__le16 raw_val;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = regmap_bulk_read(data->regmap, chan->address,
				       &raw_val, 2);
		if (ret < 0)
			return ret;
		*val = (s16)le16_to_cpu(raw_val);
		*val2 = 0;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_OFFSET:
		ret = regmap_bulk_read(data->regmap,
				       chan->address + BNO055_REG_OFFSET_ADDR,
				       &raw_val, 2);
		if (ret < 0)
			return ret;
		*val = (s16)le16_to_cpu(raw_val);
		*val2 = 0;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		*val = 1;
		switch (chan->type) {
		case IIO_ACCEL:
			/* Table 3-17: 1 m/s^2 = 100 LSB */
			*val2 = 100;
			break;
		case IIO_MAGN:
			/*
			 * Table 3-19: 1 uT = 16 LSB.  But we need
			 * Gauss: 1G = 0.1 uT.
			 */
			*val2 = 160;
			break;
		case IIO_ANGL_VEL:
			/* Table 3-22: 1 Rps = 900 LSB */
			*val2 = 900;
			break;
		case IIO_ROT:
			/* Table 3-28: 1 degree = 16 LSB */
			*val2 = 16;
			break;
		default:
			return -EINVAL;
		}
		return IIO_VAL_FRACTIONAL;
	default:
		return -EINVAL;
	}
}

static int bno055_read_temp_chan(struct iio_dev *indio_dev, int *val)
{
	struct bno055_data *data = iio_priv(indio_dev);
	unsigned int raw_val;
	int ret;

	ret = regmap_read(data->regmap, BNO055_REG_TEMP, &raw_val);
	if (ret < 0)
		return ret;

	/*
	 * Tables 3-36 and 3-37: one byte of data, signed, 1 LSB = 1C.
	 * ABI wants milliC.
	 */
	*val = raw_val * 1000;

	return IIO_VAL_INT;
}

static int bno055_read_quaternion(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int size, int *vals, int *val_len,
				  long mask)
{
	struct bno055_data *data = iio_priv(indio_dev);
	__le16 raw_vals[4];
	int i, ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (size < 4)
			return -EINVAL;
		ret = regmap_bulk_read(data->regmap,
				       BNO055_REG_QUA_DATA_W_LSB,
				       raw_vals, sizeof(raw_vals));
		if (ret < 0)
			return ret;
		for (i = 0; i < 4; i++)
			vals[i] = (s16)le16_to_cpu(raw_vals[i]);
		*val_len = 4;
		return IIO_VAL_INT_MULTIPLE;
	case IIO_CHAN_INFO_SCALE:
		/* Table 3-31: 1 quaternion = 2^14 LSB */
		if (size < 2)
			return -EINVAL;
		vals[0] = 1;
		vals[1] = 1 << 14;
		return IIO_VAL_FRACTIONAL;
	default:
		return -EINVAL;
	}
}

static int bno055_read_raw_multi(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int size, int *vals, int *val_len,
				 long mask)
{
	switch (chan->type) {
	case IIO_ACCEL:
	case IIO_MAGN:
	case IIO_ANGL_VEL:
		if (size < 2)
			return -EINVAL;
		*val_len = 2;
		return bno055_read_simple_chan(indio_dev, chan,
					       &vals[0], &vals[1],
					       mask);

	case IIO_TEMP:
		*val_len = 1;
		return bno055_read_temp_chan(indio_dev, &vals[0]);

	case IIO_ROT:
		/*
		 * Rotation is exposed as either a quaternion or three
		 * Euler angles.
		 */
		if (chan->channel2 == IIO_MOD_QUATERNION)
			return bno055_read_quaternion(indio_dev, chan,
						      size, vals,
						      val_len, mask);
		if (size < 2)
			return -EINVAL;
		*val_len = 2;
		return bno055_read_simple_chan(indio_dev, chan,
					       &vals[0], &vals[1],
					       mask);
	default:
		return -EINVAL;
	}
}

static int bno055_init_chip(struct iio_dev *indio_dev)
{
	struct bno055_data *data = iio_priv(indio_dev);
	struct device *dev = regmap_get_device(data->regmap);
	u8 chip_id_bytes[6];
	u32 chip_id;
	u16 sw_rev;
	int ret;

	ret = device_property_read_u32(dev, "bosch,operation-mode",
				       &data->op_mode);
	if (ret < 0) {
		dev_info(dev, "failed to read operation mode, falling back to accel+gyro\n");
		data->op_mode = BNO055_MODE_ACC_GYRO;
	}

	if (data->op_mode >= BNO055_MODE_MAX) {
		dev_err(dev, "bad operation mode %d\n", data->op_mode);
		return -EINVAL;
	}

	ret = regmap_write(data->regmap, BNO055_REG_PAGE_ID, 0);
	if (ret < 0) {
		dev_err(dev, "failed to switch to register page 0\n");
		return ret;
	}

	/*
	 * Configure units to what we care about.  Also configure
	 * Android orientation mode.  See datasheet Section 4.3.60.
	 */
	ret = regmap_write(data->regmap, BNO055_REG_UNIT_SEL,
			   BNO055_ANDROID_ORIENTATION | BNO055_TEMP_CELSIUS |
			   BNO055_EUL_RADIANS | BNO055_GYR_RADIANS |
			   BNO055_ACC_MPSS);
	if (ret < 0) {
		dev_err(dev, "failed to set measurement units\n");
		return ret;
	}

	ret = regmap_bulk_read(data->regmap, BNO055_REG_CHIP_ID,
			       chip_id_bytes, sizeof(chip_id_bytes));
	if (ret < 0) {
		dev_err(dev, "failed to read chip id\n");
		return ret;
	}

	chip_id = le32_to_cpu(*(u32 *)chip_id_bytes);
	sw_rev = le16_to_cpu(*(u16 *)&chip_id_bytes[4]);

	if (chip_id != BNO055_CHIP_ID) {
		dev_err(dev, "bad chip id; got %08x expected %08x\n",
			chip_id, BNO055_CHIP_ID);
		return -EINVAL;
	}

	dev_info(dev, "software revision id %04x\n", sw_rev);

	ret = regmap_update_bits(data->regmap, BNO055_REG_OPR_MODE,
				 BNO055_OPR_MODE_MASK, data->op_mode);
	if (ret < 0) {
		dev_err(dev, "failed to switch operating mode\n");
		return ret;
	}

	/*
	 * Table 3-6 says transition from CONFIGMODE to any other mode
	 * takes 7ms.
	 */
	udelay(10);

	return 0;
}

static bool bno055_fusion_mode(struct bno055_data *data)
{
	return data->op_mode >= BNO055_MODE_IMU;
}

static void bno055_init_simple_channels(struct iio_chan_spec *p,
					enum iio_chan_type type,
					u8 address,
					bool has_offset)
{
	int i;
	int mask = BIT(IIO_CHAN_INFO_RAW);

	/*
	 * Section 3.6.5 of the datasheet explains that in fusion modes,
	 * readout from the output registers is already compensated.  In
	 * non-fusion modes, the output offset is exposed separately.
	 */
	if (has_offset)
		mask |= BIT(IIO_CHAN_INFO_OFFSET);

	for (i = 0; i < 3; i++) {
		p[i] = (struct iio_chan_spec) {
			.type = type,
			.info_mask_separate = mask,
			.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
			.modified = 1,
			.channel2 = IIO_MOD_X + i,
			/* Each value is stored in two registers. */
			.address = address + 2 * i,
		};
	}
}

static const struct iio_mount_matrix *
bno055_get_mount_matrix(const struct iio_dev *indio_dev,
			      const struct iio_chan_spec *chan)
{
	return &((struct bno055_data *)iio_priv(indio_dev))->orientation;
}

static const struct iio_chan_spec_ext_info bno055_ext_info[] = {
	IIO_MOUNT_MATRIX(IIO_SHARED_BY_DIR, bno055_get_mount_matrix),
	{ },
};

static int bno055_init_channels(struct iio_dev *indio_dev)
{
	struct bno055_data *data = iio_priv(indio_dev);
	struct iio_chan_spec *channels, *p;
	bool has_offset = !bno055_fusion_mode(data);

	channels = kmalloc(sizeof(*channels) *
			   bno055_num_channels[data->op_mode], GFP_KERNEL);
	if (!channels)
		return -ENOMEM;

	p = channels;

	/* Refer to Table 3-3 of the datasheet for operation modes. */

	if (data->op_mode == BNO055_MODE_ACC_ONLY ||
	    data->op_mode == BNO055_MODE_ACC_MAG ||
	    data->op_mode == BNO055_MODE_ACC_GYRO ||
	    data->op_mode == BNO055_MODE_AMG ||
	    /* All fusion modes use the accelerometer. */
	    data->op_mode >= BNO055_MODE_IMU) {
		bno055_init_simple_channels(p, IIO_ACCEL,
					    BNO055_REG_ACC_DATA_X_LSB,
					    has_offset);
		p += 3;
	}

	if (data->op_mode == BNO055_MODE_MAG_ONLY ||
	    data->op_mode == BNO055_MODE_ACC_MAG ||
	    data->op_mode == BNO055_MODE_MAG_GYRO ||
	    data->op_mode == BNO055_MODE_AMG ||
	    data->op_mode >= BNO055_MODE_COMPASS) {
		bno055_init_simple_channels(p, IIO_MAGN,
					    BNO055_REG_MAG_DATA_X_LSB,
					    has_offset);
		p += 3;
	}

	if (data->op_mode == BNO055_MODE_GYRO_ONLY ||
	    data->op_mode == BNO055_MODE_ACC_GYRO ||
	    data->op_mode == BNO055_MODE_MAG_GYRO ||
	    data->op_mode == BNO055_MODE_AMG ||
	    data->op_mode == BNO055_MODE_IMU ||
	    data->op_mode == BNO055_MODE_NDOF_FMC_OFF ||
	    data->op_mode == BNO055_MODE_NDOF) {
		bno055_init_simple_channels(p, IIO_ANGL_VEL,
					    BNO055_REG_GYR_DATA_X_LSB,
					    has_offset);
		p += 3;
	}

	if (bno055_fusion_mode(data)) {
		/* Euler angles. */
		bno055_init_simple_channels(p, IIO_ROT,
					    BNO055_REG_EUL_HEADING_LSB,
					    false);
		p += 3;

		/* Add quaternion orientation channel. */
		*p = (struct iio_chan_spec) {
			.type = IIO_ROT,
			.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				BIT(IIO_CHAN_INFO_SCALE),
			.modified = 1,
			.channel2 = IIO_MOD_QUATERNION,
		};
		p++;
	}

	/* Finally, all modes have a temperature channel. */
	*p = (struct iio_chan_spec) {
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
	};

	p->ext_info = bno055_ext_info;

	indio_dev->channels = channels;
	indio_dev->num_channels = bno055_num_channels[data->op_mode];

	return 0;
}

static const struct iio_info bno055_info = {
	.read_raw_multi = &bno055_read_raw_multi,
};

static int bno055_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct iio_dev *indio_dev;
	struct bno055_data *data;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);

	ret = of_iio_read_mount_matrix(&client->dev, "mount-matrix",
				&data->orientation);
	if (ret)
		return ret;

	data->regmap = devm_regmap_init_i2c(client, &bno055_regmap_config);
	if (IS_ERR(data->regmap))
		return PTR_ERR(data->regmap);

	indio_dev->dev.parent = &client->dev;
	indio_dev->name = BNO055_DRIVER_NAME;

	ret = bno055_init_chip(indio_dev);
	if (ret)
		return ret;

	ret = bno055_init_channels(indio_dev);
	if (ret)
		return ret;

	indio_dev->info = &bno055_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	i2c_set_clientdata(client, indio_dev);

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "could not register IIO device\n");
		return ret;
	}

	return 0;
}

static const struct i2c_device_id bno055_id[] = {
	{"bno055", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, bno055_id);

static const struct of_device_id of_bno055_match[] = {
	{ .compatible = "bosch,bno055", .data = 0, },
	{},
};

MODULE_DEVICE_TABLE(of, of_bno055_match);

static struct i2c_driver bno055_driver = {
	.driver = {
		.name	= BNO055_DRIVER_NAME,
		.of_match_table = of_match_ptr(of_bno055_match),
	},
	.probe		= bno055_probe,
	.id_table	= bno055_id,
};
module_i2c_driver(bno055_driver);

MODULE_AUTHOR("Vlad Dogaru <vlad.dogaru@...>");
MODULE_DESCRIPTION("Driver for Bosch BNO055 9-axis orientation sensor");
MODULE_LICENSE("GPL v2");
