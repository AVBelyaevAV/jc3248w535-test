#include "audio_app.h"
#include <driver/i2s.h>

#define SAMPLE_RATE   44100
#define BLOCK_SAMPLES 256

static int16_t  _buffer[BLOCK_SAMPLES];
static uint32_t _phaseAcc = 0;
static bool     _inited = false;

void AudioApp::init(uint8_t bclk, uint8_t lrclk, uint8_t dout) {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BLOCK_SAMPLES,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    i2s_pin_config_t pins = {
        .mck_io_num   = I2S_PIN_NO_CHANGE,
        .bck_io_num   = bclk,
        .ws_io_num    = lrclk,
        .data_out_num = dout,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    esp_err_t r = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    if (r != ESP_OK) { Serial.printf("I2S install err=%d\n", r); return; }
    r = i2s_set_pin(I2S_NUM_0, &pins);
    if (r != ESP_OK) { Serial.printf("I2S pin err=%d\n", r); return; }
    _inited = true;
    Serial.println("I2S Audio OK (440 Hz sine)");
}

void AudioApp::tick() {
    if (!_inited) return;
    const double freq = 440.0;
    const double phaseStep = (double)BLOCK_SAMPLES * freq / SAMPLE_RATE;
    for (int i = 0; i < BLOCK_SAMPLES; i++) {
        double phase = (_phaseAcc + i * phaseStep) * 2.0 * M_PI;
        _buffer[i] = (int16_t)(sin(phase) * 16000);
    }
    _phaseAcc = (_phaseAcc + BLOCK_SAMPLES * phaseStep);
    size_t written = 0;
    i2s_write(I2S_NUM_0, _buffer, sizeof(_buffer), &written, portMAX_DELAY);
}