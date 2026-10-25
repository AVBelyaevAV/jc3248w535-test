#pragma once
#include <Arduino.h>
#include "driver/spi_master.h"

#define AXS_SWRESET  0x01
#define AXS_SLPOUT   0x11
#define AXS_NORON    0x13
#define AXS_INVOFF   0x20
#define AXS_INVON    0x21
#define AXS_DISPOFF  0x28
#define AXS_DISPON   0x29
#define AXS_CASET    0x2A
#define AXS_RASET    0x2B
#define AXS_RAMWR    0x2C
#define AXS_COLMOD   0x3A
#define AXS_MADCTL   0x36
#define AXS_TEOFF    0x34
#define AXS_TEON     0x35

#define AXS_CMD_WR   0x02

class AXS15231B_QSPI {
public:
    AXS15231B_QSPI(uint8_t clk, uint8_t cs,
                   uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

    bool     begin(uint16_t w, uint16_t h);
    void     setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void     pushPixel(uint16_t color);
    void     pushPixels(const uint16_t* data, uint32_t count);
    void     fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void     fillScreen(uint16_t color);
    void     drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void     setRotation(uint8_t r);
    void     invertDisplay(bool inv);

    uint16_t width()  const { return _width;  }
    uint16_t height() const { return _height; }

private:
    spi_device_handle_t _spi;
    uint8_t  _clk, _cs, _d0, _d1, _d2, _d3;
    uint16_t _width, _height;
    uint8_t  _rotation;

    void     sendCmd(uint8_t cmd);
    void     sendCmdData(uint8_t cmd, const uint8_t* data, uint8_t len);
};