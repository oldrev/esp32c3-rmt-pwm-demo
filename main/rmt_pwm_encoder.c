/*
 * SPDX-FileCopyrightText: 2025 Li Wei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "rmt_pwm_encoder.h"

static const char* TAG = "score_encoder";

#define RMT_PWM_FREQ_HZ 10000 // 10 kHz PWM

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t* copy_encoder;
    uint32_t resolution;
} rmt_pwm_encoder_t;

static size_t rmt_encode_pwm(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* primary_data,
                             size_t data_size, rmt_encode_state_t* ret_state)
{
    rmt_pwm_encoder_t* score_encoder = __containerof(encoder, rmt_pwm_encoder_t, base);
    rmt_encoder_handle_t copy_encoder = score_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    uint32_t* duty = (uint32_t*)primary_data;
    uint32_t rmt_raw_symbol_duration = score_encoder->resolution / (RMT_PWM_FREQ_HZ) / 100;
    rmt_symbol_word_t pwm_rmt_symbol = {
        .level0 = 0,
        .duration0 = rmt_raw_symbol_duration * (100 - (*duty)),
        .level1 = 1,
        .duration1 = rmt_raw_symbol_duration * (*duty),
    };
    size_t encoded_symbols
        = copy_encoder->encode(copy_encoder, channel, &pwm_rmt_symbol, sizeof(pwm_rmt_symbol), &session_state);
    *ret_state = session_state;
    return encoded_symbols;
}

static esp_err_t rmt_del_pwm_encoder(rmt_encoder_t* encoder)
{
    rmt_pwm_encoder_t* score_encoder = __containerof(encoder, rmt_pwm_encoder_t, base);
    rmt_del_encoder(score_encoder->copy_encoder);
    free(score_encoder);
    return ESP_OK;
}

static esp_err_t rmt_pwm_encoder_reset(rmt_encoder_t* encoder)
{
    rmt_pwm_encoder_t* score_encoder = __containerof(encoder, rmt_pwm_encoder_t, base);
    rmt_encoder_reset(score_encoder->copy_encoder);
    return ESP_OK;
}

esp_err_t rmt_new_pwm_encoder(const rmt_pwm_encoder_config_t* config, rmt_encoder_handle_t* ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_pwm_encoder_t* score_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    score_encoder = rmt_alloc_encoder_mem(sizeof(rmt_pwm_encoder_t));
    ESP_GOTO_ON_FALSE(score_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for musical duty encoder");
    score_encoder->base.encode = rmt_encode_pwm;
    score_encoder->base.del = rmt_del_pwm_encoder;
    score_encoder->base.reset = rmt_pwm_encoder_reset;
    score_encoder->resolution = config->resolution;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &score_encoder->copy_encoder), err, TAG,
                      "create copy encoder failed");
    *ret_encoder = &score_encoder->base;
    return ESP_OK;
err:
    if (score_encoder) {
        if (score_encoder->copy_encoder) {
            rmt_del_encoder(score_encoder->copy_encoder);
        }
        free(score_encoder);
    }
    return ret;
}
