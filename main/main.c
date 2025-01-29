/*
 * SPDX-FileCopyrightText: 2025 Li Wei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <driver/rmt_tx.h>
#include <rmt_pwm_encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#define RMT_PWM_RESOLUTION_HZ 1000000 // 1MHz resolution
#define RMT_PWM_GPIO_NUM 19

static const char* TAG = "rmt_pwm_example";

void app_main(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t pwm_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_PWM_GPIO_NUM,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_PWM_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the maximum number of transactions that can pend in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &pwm_chan));

    ESP_LOGI(TAG, "Install RMT PWM encoder");
    rmt_encoder_handle_t pwm_encoder = NULL;
    rmt_pwm_encoder_config_t encoder_config = {
        .resolution = RMT_PWM_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_pwm_encoder(&encoder_config, &pwm_encoder));


    uint32_t duty = 10;
    rmt_transmit_config_t tx_config = {
        .loop_count = -1,
    };

    while (true) {
        ESP_ERROR_CHECK(rmt_enable(pwm_chan));
        ESP_ERROR_CHECK(rmt_transmit(pwm_chan, pwm_encoder, &duty, sizeof(duty), &tx_config));
        ESP_LOGI(TAG, "Change duty to: %lu", duty);

        duty += 10;
        if(duty > 100) {
            duty = 0;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(rmt_disable(pwm_chan));

    }
}
