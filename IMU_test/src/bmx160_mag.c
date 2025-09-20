#include "bmx160_mag.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bmx160_mag, LOG_LEVEL_DBG);

static int bmx160_write_reg(const struct device *i2c_dev, uint16_t addr, 
                           uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return i2c_write(i2c_dev, data, sizeof(data), addr);
}

static int bmx160_read_reg(const struct device *i2c_dev, uint16_t addr, 
                          uint8_t reg, uint8_t *val)
{
    return i2c_write_read(i2c_dev, addr, &reg, 1, val, 1);
}

static int bmx160_mag_write_reg(const struct device *i2c_dev, uint16_t addr,
                               uint8_t mag_reg, uint8_t val)
{
    int ret;
    
    /* Set manual mode */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_MAG_IF_1, MAG_IF_MANUAL_MODE);
    if (ret < 0) return ret;
    
    /* Set magnetometer register address */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_MAG_IF_2, mag_reg);
    if (ret < 0) return ret;
    
    /* Write data */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_MAG_IF_3, val);
    if (ret < 0) return ret;
    
    k_msleep(2); /* Wait for write completion */
    
    return 0;
}

int bmx160_soft_reset(const struct device *i2c_dev, uint16_t addr)
{
    int ret = bmx160_write_reg(i2c_dev, addr, BMX160_CMD, 0xB6);
    if (ret == 0) {
        k_msleep(50); /* Wait for reset completion */
    }
    return ret;
}

int bmx160_set_accel_range(const struct device *i2c_dev, uint16_t addr, uint8_t range)
{
    return bmx160_write_reg(i2c_dev, addr, 0x41, range);
}

int bmx160_set_gyro_range(const struct device *i2c_dev, uint16_t addr, uint8_t range)
{
    return bmx160_write_reg(i2c_dev, addr, 0x43, range);
}

int bmx160_mag_init(const struct device *i2c_dev, uint16_t addr)
{
    int ret;
    uint8_t chip_id;
    
    /* Read chip ID to verify communication */
    ret = bmx160_read_reg(i2c_dev, addr, 0x00, &chip_id);
    if (ret < 0) {
        LOG_ERR("Failed to read chip ID: %d", ret);
        return ret;
    }
    
    LOG_INF("BMX160 Chip ID: 0x%02X", chip_id);
    
    /* Configure accelerometer and gyroscope power modes */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_CMD, 0x11); /* Accel normal mode */
    if (ret < 0) return ret;
    k_msleep(5);
    
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_CMD, 0x15); /* Gyro normal mode */
    if (ret < 0) return ret;
    k_msleep(100);
    
    /* Configure magnetometer interface */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_CMD, 0x19); /* Mag normal mode */
    if (ret < 0) return ret;
    k_msleep(5);
    
    /* Initialize BMM150 magnetometer */
    ret = bmx160_mag_write_reg(i2c_dev, addr, BMM150_POWER_CTRL, BMM150_POWER_ON);
    if (ret < 0) return ret;
    
    ret = bmx160_mag_write_reg(i2c_dev, addr, BMM150_OP_MODE, BMM150_NORMAL_MODE);
    if (ret < 0) return ret;
    
    ret = bmx160_mag_write_reg(i2c_dev, addr, BMM150_REP_XY, BMM150_REP_XY_REGULAR);
    if (ret < 0) return ret;
    
    ret = bmx160_mag_write_reg(i2c_dev, addr, BMM150_REP_Z, BMM150_REP_Z_REGULAR);
    if (ret < 0) return ret;
    
    /* Configure MAG_CONF for regular preset */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_MAG_CONF, 0x00);
    if (ret < 0) return ret;
    
    /* Set MAG_IF to auto mode */
    ret = bmx160_write_reg(i2c_dev, addr, BMX160_MAG_IF_1, MAG_IF_READ_BURST);
    if (ret < 0) return ret;
    
    LOG_INF("BMX160 magnetometer initialized successfully");
    return 0;
}

int bmx160_mag_read_data(const struct device *i2c_dev, uint16_t addr,
                        struct bmx160_mag_data *data)
{
    uint8_t raw_data[8];
    int ret;
    
    /* Read magnetometer data registers */
    ret = i2c_write_read(i2c_dev, addr, 
                        (uint8_t[]){BMX160_MAG_DATA_X_LSB}, 1,
                        raw_data, sizeof(raw_data));
    if (ret < 0) {
        return ret;
    }
    
    /* Convert raw data to signed values */
data->x     = (int16_t)((raw_data[1] << 8) | raw_data[0]) >> 3;
data->y     = (int16_t)((raw_data[3] << 8) | raw_data[2]) >> 3;
data->z     = (int16_t)((raw_data[5] << 8) | raw_data[4]) >> 1;
data->rhall = (uint16_t)((raw_data[7] << 8) | raw_data[6]) >> 2;

    
    return 0;
}
