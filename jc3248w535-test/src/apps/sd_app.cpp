#include "sd_app.h"

static uint8_t  _cs;
static uint32_t _benchIdx = 0;
static bool     _mounted = false;

// Используем стандартный SPI-объект (VSPI/SPI3 по умолчанию свободен,
// так как дисплей занял SPI2_HOST через Arduino_ESP32QSPI)
static SPIClass SD_SPI(HSPI);

void SDApp::init(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t cs) {
    _cs = cs;
    SD_SPI.begin(clk, miso, mosi, cs);

    if (!SD.begin(cs, SD_SPI, 25000000)) {
        Serial.println("SD FAIL (insert card?)");
        _mounted = false;
        return;
    }
    _mounted = true;
    Serial.printf("SD OK: %llu MB\n", SD.cardSize() / (1024 * 1024));
}

bool SDApp::isMounted() {
    return _mounted;
}

void SDApp::listRoot() {
    if (!_mounted) {
        Serial.println("SD not mounted");
        return;
    }

    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open root");
        return;
    }

    Serial.println("--- SD root ---");
    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            Serial.printf("[DIR]  %s\n", entry.name());
        } else {
            Serial.printf("[FILE] %s (%u bytes)\n", entry.name(), entry.size());
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
    Serial.println("---------------");
}

void SDApp::benchmark() {
    if (!_mounted) return;
    if (_benchIdx++ % 5 != 0) return;

    const uint32_t N = 4096;
    uint8_t buf[N];
    memset(buf, 0xAA, N);

    uint32_t t0 = micros();
    File f = SD.open("/bench.bin", FILE_WRITE);
    if (!f) return;
    for (int i = 0; i < 64; i++) f.write(buf, N);
    f.close();
    uint32_t dt = micros() - t0;
    float mbps = (64.0f * N) / (dt * 1e-6f) / 1e6f;
    Serial.printf("SD write: 64*4KB in %u us = %.2f MB/s\n", dt, mbps);
    SD.remove("/bench.bin");
}