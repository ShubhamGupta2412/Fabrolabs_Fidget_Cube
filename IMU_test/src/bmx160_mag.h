#ifndef BMX160_MAG_H_
#define BMX160_MAG_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* BMX160 Magnetometer Registers */
#define BMX160_MAG_DATA_X_LSB       0x04
#define BMX160_MAG_DATA_X_MSB       0x05
#define BMX160_MAG_DATA_Y_LSB       0x06
#define BMX160_MAG_DATA_Y_MSB       0x07
#define BMX160_MAG_DATA_Z_LSB       0x08
#define BMX160_MAG_DATA_Z_MSB       0x09
#define BMX160_MAG_RHALL_LSB        0x0A
#define BMX160_MAG_RHALL_MSB        0x0B

#define BMX160_MAG_CONF             0x44
#define BMX160_MAG_IF_0             0x4C
#define BMX160_MAG_IF_1             0x4D
#define BMX160_MAG_IF_2             0x4E
#define BMX160_MAG_IF_3             0x4F

#define BMX160_CMD                  0x7E

/* BMM150 Registers (accessed via MAG_IF) */
#define BMM150_CHIP_ID              0x40
#define BMM150_POWER_CTRL           0x4B
#define BMM150_OP_MODE              0x4C
#define BMM150_REP_XY               0x51
#define BMM150_REP_Z                0x52

/* BMM150 Values */
#define BMM150_POWER_ON             0x01
#define BMM150_NORMAL_MODE          0x00
#define BMM150_REP_XY_REGULAR       0x04
#define BMM150_REP_Z_REGULAR        0x0E

/* MAG_IF Control Bits */
#define MAG_IF_MANUAL_MODE          0x80
#define MAG_IF_READ_BURST           0x03

struct bmx160_mag_data {
    int16_t x;
    int16_t y;
    int16_t z;
    uint16_t rhall;
};

int bmx160_mag_init(const struct device *i2c_dev, uint16_t addr);
int bmx160_mag_read_data(const struct device *i2c_dev, uint16_t addr, 
                         struct bmx160_mag_data *data);
int bmx160_soft_reset(const struct device *i2c_dev, uint16_t addr);
int bmx160_set_accel_range(const struct device *i2c_dev, uint16_t addr, uint8_t range);
int bmx160_set_gyro_range(const struct device *i2c_dev, uint16_t addr, uint8_t range);

/* Range constants matching Arduino library */
#define BMX160_ACCEL_RANGE_2G       0x03
#define BMX160_ACCEL_RANGE_4G       0x05
#define BMX160_ACCEL_RANGE_8G       0x08
#define BMX160_ACCEL_RANGE_16G      0x0C

#define BMX160_GYRO_RANGE_2000DPS   0x00
#define BMX160_GYRO_RANGE_1000DPS   0x01
#define BMX160_GYRO_RANGE_500DPS    0x02
#define BMX160_GYRO_RANGE_250DPS    0x03
#define BMX160_GYRO_RANGE_125DPS    0x04


int bmx160_read_accel(const struct device *i2c_dev, uint16_t addr, int16_t *accel_xyz);
int bmx160_read_gyro(const struct device *i2c_dev, uint16_t addr, int16_t *gyro_xyz);

#endif /* BMX160_MAG_H_ */
