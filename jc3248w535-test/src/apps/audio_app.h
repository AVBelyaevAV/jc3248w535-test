#pragma once
#include <Arduino.h>

namespace AudioApp {
    void init(uint8_t bclk, uint8_t lrclk, uint8_t dout);
    void tick();
}