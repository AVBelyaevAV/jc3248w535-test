#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

namespace SDApp {
    void init(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t cs);
    void benchmark();
    bool isMounted();
    void listRoot();
}