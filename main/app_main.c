/*
 * MIT License
 *
 * Copyright (c) 2017-2018 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "app_main"

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ       400000                // Hz

typedef struct
{
    i2c_port_t port;
    i2c_config_t config;
} i2c_master_info_t;

static void _delay(void)
{
    vTaskDelay(2000 / portTICK_RATE_MS);
}

i2c_master_info_t * i2c_master_init(i2c_port_t i2c_port, gpio_num_t sda_io_num, gpio_num_t scl_io_num, uint32_t clk_speed)
{
    i2c_master_info_t * info = malloc(sizeof(*info));
    if (info)
    {
        memset(info, 0, sizeof(*info));
        info->port = i2c_port;
        info->config.mode = I2C_MODE_MASTER;
        info->config.sda_io_num = sda_io_num;
        info->config.sda_pullup_en = GPIO_PULLUP_ENABLE;  // use internal pullups
        info->config.scl_io_num = scl_io_num;
        info->config.scl_pullup_en = GPIO_PULLUP_ENABLE;  // use internal pullups
        info->config.master.clk_speed = clk_speed;         // Hz

        ESP_LOGW(TAG, "about to run i2c_param_config");
        _delay();

        ESP_ERROR_CHECK(i2c_param_config(i2c_port, &info->config));

        ESP_LOGW(TAG, "about to run i2c_driver_install");
        _delay();

        ESP_ERROR_CHECK(i2c_driver_install(i2c_port, info->config.mode,
                                           I2C_MASTER_RX_BUF_LEN,
                                           I2C_MASTER_TX_BUF_LEN, 0));
    }
    return info;
}

int i2c_master_scan(const i2c_master_info_t * info)
{
    int num_detected = 0;
    if (info)
    {
        for (int address = 1; address < 0x7f; ++address)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, address << 1, true /*ACK_CHECK*/);
            i2c_master_stop(cmd);
            esp_err_t err = i2c_master_cmd_begin(info->port, cmd, (1000 / portTICK_RATE_MS));
            if (err == 0)
            {
                ++num_detected;
                ESP_LOGI(TAG, "detected I2C address on master %d at address 0x%02x", info->port, address);
            }
            i2c_cmd_link_delete(cmd);
        }
    }
    else
    {
        ESP_LOGE(TAG, "info is NULL");
    }
    return num_detected;
}

void app_main()
{
    i2c_master_info_t * i2c_master_info = i2c_master_init(I2C_MASTER_NUM, CONFIG_I2C_MASTER_SDA_GPIO, CONFIG_I2C_MASTER_SCL_GPIO, I2C_MASTER_FREQ_HZ);

    ESP_LOGW(TAG, "about to run i2c_master_scan");
    _delay();
    while (1)
    {
        i2c_master_scan(i2c_master_info);
        i2c_master_scan(i2c_master_info);
        i2c_master_scan(i2c_master_info);
        i2c_master_scan(i2c_master_info);
        vTaskDelay(1);
    }
}
