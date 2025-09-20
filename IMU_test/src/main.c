#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include "bmx160_mag.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define BMX160_I2C_ADDR 0x68
#define SLEEP_TIME_MS   1000

/* Function to convert sensor value to double (mimicking Arduino library) */
static double to_double(struct sensor_value *val)
{
    return (double)val->val1 + (double)val->val2 / 1000000;
}

/* Equivalent to Arduino library's getAllData function */
static int get_all_data(const struct device *sensor_dev, 
                       const struct device *i2c_dev,
                       struct sensor_value *accel,
                       struct sensor_value *gyro,
                       struct bmx160_mag_data *mag)
{
    int ret;
    
    /* Fetch accelerometer and gyroscope data using Zephyr sensor API */
    ret = sensor_sample_fetch(sensor_dev);
    if (ret < 0) {
        LOG_ERR("Failed to fetch sensor data: %d", ret);
        return ret;
    }
    
    /* Get accelerometer data */
    ret = sensor_channel_get(sensor_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret < 0) {
        LOG_ERR("Failed to get accelerometer data: %d", ret);
        return ret;
    }
    
    /* Get gyroscope data */
    ret = sensor_channel_get(sensor_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (ret < 0) {
        LOG_ERR("Failed to get gyroscope data: %d", ret);
        return ret;
    }
    
    /* Read magnetometer data directly */
    ret = bmx160_mag_read_data(i2c_dev, BMX160_I2C_ADDR, mag);
    if (ret < 0) {
        LOG_ERR("Failed to read magnetometer data: %d", ret);
        return ret;
    }
    
    return 0;
}

/* Equivalent to Arduino library's setAccelRange function */
static int set_accel_range(const struct device *i2c_dev, uint8_t range)
{
    return bmx160_set_accel_range(i2c_dev, BMX160_I2C_ADDR, range);
}

/* Equivalent to Arduino library's setGyroRange function */
static int set_gyro_range(const struct device *i2c_dev, uint8_t range)
{
    return bmx160_set_gyro_range(i2c_dev, BMX160_I2C_ADDR, range);
}

/* Equivalent to Arduino library's softReset function */
static int soft_reset(const struct device *i2c_dev)
{
    return bmx160_soft_reset(i2c_dev, BMX160_I2C_ADDR);
}

/* Convert magnetometer raw values to microTesla (approximate) */
static double mag_raw_to_ut(int16_t raw_val)
{
    return (double)raw_val * 0.3; /* Approximate conversion factor */
}


int main(void)
{
    const struct device *i2c_dev;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    struct bmx160_mag_data mag_data;
    int ret;

    LOG_INF("BMX160 IMU Demo Starting (custom driver)");

    /* Get I2C device */
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -1;
    }

    LOG_INF("I2C device ready, initializing BMX160...");

    /* Initialize BMX160 */
    ret = bmx160_mag_init(i2c_dev, BMX160_I2C_ADDR);
    if (ret < 0) {
        LOG_ERR("BMX160 initialization failed: %d", ret);
        return ret;
    }

    /* Configure ranges */
    ret = bmx160_set_accel_range(i2c_dev, BMX160_I2C_ADDR, BMX160_ACCEL_RANGE_4G);
    if (ret < 0) {
        LOG_WRN("Failed to set accelerometer range: %d", ret);
    }
    ret = bmx160_set_gyro_range(i2c_dev, BMX160_I2C_ADDR, BMX160_GYRO_RANGE_500DPS);
    if (ret < 0) {
        LOG_WRN("Failed to set gyroscope range: %d", ret);
    }

    LOG_INF("BMX160 initialization complete");

    while (1) {
    int16_t accel_raw[3];
    int16_t gyro_raw[3];

    /* Read accel */
    ret = bmx160_read_accel(i2c_dev, BMX160_I2C_ADDR, accel_raw);
    if (ret == 0) {
const double lsb_to_g    = 4.0 / 2048.0;
const double g_to_ms2 = 9.80665;

printf("A X: %7.2f  Y: %7.2f  Z: %7.2f m/s²\n",
       accel_raw[0] * lsb_to_g * g_to_ms2,
       accel_raw[1] * lsb_to_g * g_to_ms2,
       accel_raw[2] * lsb_to_g * g_to_ms2);

        
    } else {
        LOG_ERR("Accel read failed: %d", ret);
    }

    /* Read gyro */
    ret = bmx160_read_gyro(i2c_dev, BMX160_I2C_ADDR, gyro_raw);
    if (ret == 0) {
        printf("G X: %7.2f  Y: %7.2f  Z: %7.2f dps\n",
               gyro_raw[0] * (500.0/32768),  // 500 dps range: LSB = 1/64 dps
               gyro_raw[1] * (500.0/32768),
               gyro_raw[2] * (500.0/32768));
    } else {
        LOG_ERR("Gyro read failed: %d", ret);
    }

    /* Read magnetometer */
    ret = bmx160_mag_read_data(i2c_dev, BMX160_I2C_ADDR, &mag_data);
    if (ret == 0) {
        printf("M X: %6.2f  Y: %6.2f  Z: %6.2f uT\n",
               mag_raw_to_ut(mag_data.x),
               mag_raw_to_ut(mag_data.y),
               mag_raw_to_ut(mag_data.z));
    } else {
        LOG_ERR("Mag read failed: %d", ret);
    }

    k_msleep(SLEEP_TIME_MS);
}

    return 0;
}
