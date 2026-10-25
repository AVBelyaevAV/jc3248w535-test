#include "axs15231b_qspi.h"
#include <cstring>
#include "esp_heap_caps.h"

#ifndef SPI_DEVICE_QUAD_ENABLE
    #define SPI_DEVICE_QUAD_ENABLE (1 << 9)
#endif

AXS15231B_QSPI::AXS15231B_QSPI(uint8_t clk, uint8_t cs,
                               uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
    : _clk(clk), _cs(cs), _d0(d0), _d1(d1), _d2(d2), _d3(d3),
      _width(320), _height(480), _rotation(0) {
    _spi = nullptr;
}

bool AXS15231B_QSPI::begin(uint16_t w, uint16_t h) {
    _width = w; _height = h;

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num   = _d0;
    buscfg.miso_io_num   = _d1;
    buscfg.sclk_io_num   = _clk;
    buscfg.quadwp_io_num = _d2;
    buscfg.quadhd_io_num = _d3;
    buscfg.max_transfer_sz = 320 * 480 * 2;

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 40 * 1000 * 1000;
    devcfg.mode           = 3;  // Обязательно для AXS15231B
    devcfg.spics_io_num   = _cs;
    devcfg.queue_size     = 7;
    devcfg.flags          = SPI_DEVICE_QUAD_ENABLE | SPI_DEVICE_HALFDUPLEX;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("SPI bus init failed: %d\n", ret);
        return false;
    }
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &_spi);
    if (ret != ESP_OK) {
        Serial.printf("SPI add device failed: %d\n", ret);
        return false;
    }

    delay(50);

    // ===== Полная вендорская последовательность для JC3248W535C =====
    // Источник: рабочий конфиг из ESP32_Display_Panel Issue #164[reference:3]
    sendCmd(0x11); // SLPOUT
    delay(120);

    uint8_t bb1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5};
    sendCmdData(0xBB, bb1, 8);

    uint8_t a0[] = {0xC0, 0x10, 0x00, 0x02, 0x00, 0x00, 0x04, 0x3F, 0x20, 0x05, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendCmdData(0xA0, a0, 17);

    uint8_t a2[] = {0x30, 0x3C, 0x24, 0x14, 0xD0, 0x20, 0xFF, 0xE0, 0x40, 0x19, 0x80, 0x80, 0x80, 0x20, 0xf9, 0x10, 0x02, 0xff, 0xff, 0xF0, 0x90, 0x01, 0x32, 0xA0, 0x91, 0xE0, 0x20, 0x7F, 0xFF, 0x00, 0x5A};
    sendCmdData(0xA2, a2, 31);

    uint8_t d0[] = {0xE0, 0x40, 0x51, 0x24, 0x08, 0x05, 0x10, 0x01, 0x20, 0x15, 0x42, 0xC2, 0x22, 0x22, 0xAA, 0x03, 0x10, 0x12, 0x60, 0x14, 0x1E, 0x51, 0x15, 0x00, 0x8A, 0x20, 0x00, 0x03, 0x3A, 0x12};
    sendCmdData(0xD0, d0, 30);

    uint8_t a3[] = {0xA0, 0x06, 0xAA, 0x00, 0x08, 0x02, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x55, 0x55};
    sendCmdData(0xA3, a3, 22);

    uint8_t c3[] = {0x00, 0x00, 0x00, 0x50, 0x03, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01};
    sendCmdData(0xC3, c3, 11);

    uint8_t c5[] = {0x18, 0x00, 0x00, 0x03, 0xFE, 0x3A, 0x4A, 0x20, 0x30, 0x10, 0x88, 0xDE, 0x0D, 0x08, 0x0F, 0x0F, 0x01, 0x3A, 0x4A, 0x20, 0x10, 0x10, 0x00};
    sendCmdData(0xC5, c5, 23);

    uint8_t c1[] = {0x33, 0x04, 0x02, 0x02};
    sendCmdData(0xC1, c1, 4);

    uint8_t bb2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendCmdData(0xBB, bb2, 8);

    // ===== Стандартные DCS-команды =====
    uint8_t pf = 0x55;  // RGB565
    sendCmdData(AXS_COLMOD, &pf, 1);

    uint8_t mad = 0x00;
    sendCmdData(AXS_MADCTL, &mad, 1);

    sendCmd(0x13); // NORON (Normal Display Mode On)
    delay(10);
    sendCmd(AXS_DISPON);
    delay(20);

    fillScreen(0x0000);
    return true;
}

void AXS15231B_QSPI::sendCmd(uint8_t cmd) {
    if (!_spi) return;
    spi_transaction_t t = {};
    uint8_t buf[3] = { AXS_CMD_WR, 0x00, cmd };
    t.length = 8 * 3;
    t.tx_buffer = buf;
    spi_device_polling_transmit(_spi, &t);
}

void AXS15231B_QSPI::sendCmdData(uint8_t cmd, const uint8_t* data, uint8_t len) {
    if (!_spi) return;

    spi_transaction_t t = {};
    uint8_t buf[64];
    if (len > 60) len = 60;

    buf[0] = AXS_CMD_WR;
    buf[1] = 0x00;
    buf[2] = cmd;
    if (len > 0) memcpy(&buf[3], data, len);

    t.length = 8 * (3 + len);
    t.tx_buffer = buf;
    spi_device_polling_transmit(_spi, &t);
}

void AXS15231B_QSPI::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    if (!_spi) return;
    uint8_t ca[4] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
                      (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    uint8_t ra[4] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
                      (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    sendCmdData(AXS_CASET, ca, 4);
    sendCmdData(AXS_RASET, ra, 4);
    sendCmd(AXS_RAMWR);
}

void AXS15231B_QSPI::pushPixel(uint16_t color) {
    pushPixels(&color, 1);
}

// КРИТИЧЕСКИ ВАЖНО: отправка данных в Big-Endian
void AXS15231B_QSPI::pushPixels(const uint16_t* data, uint32_t count) {
    if (!_spi || count == 0) return;

    // Выделяем DMA-совместимый буфер и копируем данные с заменой порядка байт
    uint8_t* tx_buf = (uint8_t*)heap_caps_malloc(count * 2, MALLOC_CAP_DMA);
    if (!tx_buf) return;

    for (uint32_t i = 0; i < count; i++) {
        tx_buf[i * 2]     = (data[i] >> 8) & 0xFF; // Старший байт
        tx_buf[i * 2 + 1] = data[i] & 0xFF;        // Младший байт
    }

    spi_transaction_t t = {};
    t.length    = count * 16;
    t.tx_buffer = tx_buf;
    spi_device_polling_transmit(_spi, &t);

    free(tx_buf);
}

void AXS15231B_QSPI::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (!_spi) return;
    setWindow(x, y, x + w - 1, y + h - 1);
    
    const uint32_t BS = 512;
    uint16_t* buf = (uint16_t*)heap_caps_malloc(BS * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) return;

    for (uint32_t i = 0; i < BS; i++) buf[i] = color;

    uint32_t n = (uint32_t)w * h;
    while (n > 0) {
        uint32_t chunk = (n > BS) ? BS : n;
        pushPixels(buf, chunk);
        n -= chunk;
    }
    free(buf);
}

void AXS15231B_QSPI::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

void AXS15231B_QSPI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!_spi) return;
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    while (true) {
        setWindow(x0, y0, x0, y0);
        pushPixels(&color, 1);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void AXS15231B_QSPI::setRotation(uint8_t r) {
    if (!_spi) return;
    _rotation = r & 3;
    uint8_t mad = 0;
    switch (_rotation) {
        case 0: mad = 0x00; break;
        case 1: mad = 0x60; break;
        case 2: mad = 0xC0; break;
        case 3: mad = 0xA0; break;
    }
    sendCmdData(AXS_MADCTL, &mad, 1);
    if (_rotation & 1) { 
        uint16_t t = _width; _width = _height; _height = t; 
    }
}

void AXS15231B_QSPI::invertDisplay(bool inv) {
    if (!_spi) return;
    sendCmd(inv ? AXS_INVON : AXS_INVOFF);
}