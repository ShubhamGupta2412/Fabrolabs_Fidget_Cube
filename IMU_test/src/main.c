#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include "bmx160_mag.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define BMX160_I2C_ADDR 0x68
#define SLEEP_TIME_MS 1000

#define ACCEL_LSB_TO_G   (4.0f / 2048.0f)
#define G_TO_MS2         (9.80665f)
#define GYRO_LSB_TO_DPS  (500.0f / 32768.0f)

static inline float accel_ms2(int16_t raw) {
    return raw * ACCEL_LSB_TO_G * G_TO_MS2;
}
static inline float gyro_dps(int16_t raw) {
    return raw * GYRO_LSB_TO_DPS;
}
static inline float mag_ut(int16_t raw) {
    return raw * 0.3f;
}

int main(void)
{
    const struct device *i2c_dev;
    struct sensor_value accel[3];
    struct sensor_value gyro[3];
    struct bmx160_mag_data mag_data;
    int ret;

    LOG_INF("BMX160 IMU Demo Starting (custom driver)");

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
        if (ret < 0) {
            LOG_ERR("Accel read failed: %d", ret);
        }

        /* Read gyro */
        ret = bmx160_read_gyro(i2c_dev, BMX160_I2C_ADDR, gyro_raw);
        if (ret < 0) {
            LOG_ERR("Gyro read failed: %d", ret);
        }

        /* Read magnetometer */
        ret = bmx160_mag_read_data(i2c_dev, BMX160_I2C_ADDR, &mag_data);
        if (ret < 0) {
            LOG_ERR("Mag read failed: %d", ret);
        }

        printf("A: X%7.2f Y%7.2f Z%7.2f m/s² | "
               "G: X%7.2f Y%7.2f Z%7.2f dps | "
               "M: X%7.2f Y%7.2f Z%7.2f uT\n",
               accel_ms2(accel_raw[0]), accel_ms2(accel_raw[1]), accel_ms2(accel_raw[2]),
               gyro_dps(gyro_raw[0]), gyro_dps(gyro_raw[1]), gyro_dps(gyro_raw[2]),
               mag_ut(mag_data.x), mag_ut(mag_data.y), mag_ut(mag_data.z));

        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }

    return 0;
}
