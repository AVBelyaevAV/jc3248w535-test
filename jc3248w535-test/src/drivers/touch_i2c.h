#pragma once
#include <Arduino.h>
#include <Wire.h>

struct TouchPoint {
    uint16_t x, y;
    bool valid;
};

class TouchI2C {
public:
    TouchI2C(uint8_t sda, uint8_t scl);
    bool begin();
    bool read(TouchPoint* tp);

private:
    TwoWire _wire;
    uint8_t _addr;
    bool _initAXS();
};