#include "touch_i2c.h"

static const uint8_t AXS_READ_TOUCHPAD[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8};

TouchI2C::TouchI2C(uint8_t sda, uint8_t scl) : _wire(0), _addr(0x3B) {
    _wire.begin(sda, scl, 400000UL);
}

bool TouchI2C::begin() {
    _wire.beginTransmission(_addr);
    if (_wire.endTransmission() != 0) {
        Serial.println("Touch not found at 0x3B");
        return false;
    }
    return true;
}

bool TouchI2C::read(TouchPoint* tp) {
    tp->valid = false;

    _wire.beginTransmission(_addr);
    _wire.write(AXS_READ_TOUCHPAD, sizeof(AXS_READ_TOUCHPAD));
    if (_wire.endTransmission(false) != 0) return false;

    if (_wire.requestFrom(_addr, (uint8_t)8) != 8) return false;

    uint8_t data[8];
    for (int i = 0; i < 8; i++) {
        if (_wire.available()) data[i] = _wire.read();
    }

    if (data[0] != 0) return false;

    uint16_t x = ((data[2] & 0x0F) << 8) | data[3];
    uint16_t y = ((data[4] & 0x0F) << 8) | data[5];

    // Фильтр ложных (0,0)
    if (x == 0 && y == 0) return false;

    tp->x = x;
    tp->y = y;
    tp->valid = true;
    return true;
}