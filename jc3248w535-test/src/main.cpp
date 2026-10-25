#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "drivers/touch_i2c.h"
#include "apps/wifi_app.h"
#include "apps/bt_app.h"
#include "apps/audio_app.h"
#include "apps/sd_app.h"
#include "esp_heap_caps.h"

// ===== Пины платы JC3248W535C =====
#define LCD_CS    45
#define LCD_CLK   47
#define LCD_DATA0 21
#define LCD_DATA1 48
#define LCD_DATA2 40
#define LCD_DATA3 39
#define LCD_BL    1

#define TP_SDA    4
#define TP_SCL    8

#define SD_CLK    12
#define SD_MOSI   11
#define SD_MISO   13
#define SD_CS     10

#define I2S_LRCLK 2
#define I2S_BCLK  42
#define I2S_DOUT  41

#define BAT_ADC   5

// ===== Физический размер дисплея (портрет) =====
#define PHYS_W 320
#define PHYS_H 480

// ===== Логический размер (альбом) =====
#define LAND_W 480
#define LAND_H 320

// ===== Arduino_GFX =====
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_CLK, LCD_DATA0, LCD_DATA1, LCD_DATA2, LCD_DATA3
);

Arduino_GFX *panel = new Arduino_AXS15231B(
    bus, GFX_NOT_DEFINED, 0, false, PHYS_W, PHYS_H
);

Arduino_Canvas *gfx = new Arduino_Canvas(
    PHYS_W, PHYS_H, panel, 0, 0, 0
);

static uint16_t* landBuffer = nullptr;
static TouchI2C touch(TP_SDA, TP_SCL);

// ===== 5x7 шрифт ASCII 32..127 =====
static const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
    {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
    {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00},
    {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18},
    {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00},
    {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C},
    {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C},
    {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00},
    {0x08,0x04,0x08,0x10,0x08}
};

// ===== Преобразование логических (альбом) координат в физические =====
static inline void landToPhys(int16_t lx, int16_t ly, int16_t &px, int16_t &py) {
    px = ly;
    py = PHYS_H - 1 - lx;
}

// ===== Операции с логическим буфером =====
static void landFill(uint16_t color) {
    for (uint32_t i = 0; i < (uint32_t)LAND_W * LAND_H; i++)
        landBuffer[i] = color;
}

static void landFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t row = 0; row < h; row++) {
        int16_t yy = y + row;
        if (yy < 0 || yy >= LAND_H) continue;
        for (int16_t col = 0; col < w; col++) {
            int16_t xx = x + col;
            if (xx < 0 || xx >= LAND_W) continue;
            landBuffer[yy * LAND_W + xx] = color;
        }
    }
}

static void landDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    while (true) {
        if (x0 >= 0 && x0 < LAND_W && y0 >= 0 && y0 < LAND_H)
            landBuffer[y0 * LAND_W + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ===== Рисование текста в логическом буфере =====
static void landDrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < 32 || c > 127) return;
    const uint8_t* glyph = font5x7[c - 32];
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                landFillRect(x + i * size, y + j * size, size, size, color);
            } else if (bg != color) {
                landFillRect(x + i * size, y + j * size, size, size, bg);
            }
        }
    }
}

static void landDrawString(int16_t x, int16_t y, const char* s, uint16_t color, uint16_t bg, uint8_t size) {
    while (*s) {
        landDrawChar(x, y, *s, color, bg, size);
        x += 6 * size;
        s++;
    }
}

// ===== Отправка логического буфера на дисплей =====
static void flushLand() {
    if (!landBuffer) return;
    for (int16_t ly = 0; ly < LAND_H; ly++) {
        for (int16_t lx = 0; lx < LAND_W; lx++) {
            int16_t px, py;
            landToPhys(lx, ly, px, py);
            gfx->drawPixel(px, py, landBuffer[ly * LAND_W + lx]);
        }
    }
    gfx->flush();
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("\n=== JC3248W535C Landscape (software rotation) ===");
    Serial.printf("PSRAM: %s\n", psramFound() ? "YES" : "NO");

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    analogReadResolution(12);
    pinMode(BAT_ADC, INPUT);

    landBuffer = (uint16_t*)heap_caps_malloc(
        (size_t)LAND_W * LAND_H * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM
    );
    if (!landBuffer) {
        Serial.println("FATAL: Cannot allocate landscape buffer!");
        while (1) delay(1000);
    }

    Serial.println("[1/5] Init LCD...");
    if (!gfx->begin()) {
        Serial.println("LCD init FAILED");
        while (1) delay(1000);
    }
    Serial.println("LCD OK");

    Serial.println("[2/5] Init Touch...");
    if (!touch.begin()) Serial.println("Touch init FAILED");
    else                    Serial.println("Touch OK");

    Serial.println("[3/5] Init SD...");
    SDApp::init(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

    Serial.println("[4/5] Init I2S Audio...");
    AudioApp::init(I2S_BCLK, I2S_LRCLK, I2S_DOUT);

    Serial.println("[5/5] Init WiFi+BT...");
    WiFiApp::begin();
    BTApp::begin();

    landFill(RGB565_RED);
    flushLand();
    Serial.println("=== Setup complete ===\n");
}

static const uint16_t palette[] = {
    RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW,
    RGB565_MAGENTA, RGB565_CYAN, RGB565_WHITE, RGB565_BLACK
};

// ===== Сцена: SD =====
static void drawSdContent() {
    landFill(RGB565_BLACK);
    landDrawString(10, 10, "=== SD ROOT ===", RGB565_WHITE, RGB565_BLACK, 2);

    if (!SDApp::isMounted()) {
        landDrawString(10, 50, "SD NOT MOUNTED", RGB565_RED, RGB565_BLACK, 2);
        return;
    }

    File root = SD.open("/");
    if (!root) {
        landDrawString(10, 50, "Failed to open root", RGB565_RED, RGB565_BLACK, 2);
        return;
    }

    int y = 50;
    int count = 0;
    File entry = root.openNextFile();
    while (entry) {
        if (y > LAND_H - 20) {
            landDrawString(10, y, "...", RGB565_WHITE, RGB565_BLACK, 2);
            entry.close();
            break;
        }

        char line[64];
        if (entry.isDirectory()) {
            snprintf(line, sizeof(line), "[D] %s", entry.name());
        } else {
            snprintf(line, sizeof(line), "[F] %s", entry.name());
        }
        landDrawString(10, y, line, RGB565_WHITE, RGB565_BLACK, 2);
        y += 22;
        count++;

        entry.close();
        entry = root.openNextFile();
    }
    root.close();

    if (count == 0) {
        landDrawString(10, y, "(empty)", RGB565_YELLOW, RGB565_BLACK, 2);
    }
}

static void drawDemoPhase(uint8_t phase) {
    switch (phase % 9) {
    case 0: {
        for (int16_t y = 0; y < LAND_H; y++) {
            uint8_t r = (y * 31) / LAND_H;
            uint8_t g = (y * 63) / LAND_H;
            uint8_t b = (y * 31) / LAND_H;
            uint16_t c = (r << 11) | (g << 5) | b;
            for (int16_t x = 0; x < LAND_W; x++)
                landBuffer[y * LAND_W + x] = c;
        }
        break;
    }
    case 1: {
        uint16_t bw = LAND_W / 8;
        for (int i = 0; i < 8; i++)
            landFillRect(i * bw, 0, bw + 1, LAND_H, palette[i]);
        break;
    }
    case 2: {
        landFill(RGB565_BLACK);
        for (int x = 0; x < LAND_W; x += 20)
            landDrawLine(x, 0, x, LAND_H - 1, RGB565_GREEN);
        for (int y = 0; y < LAND_H; y += 20)
            landDrawLine(0, y, LAND_W - 1, y, RGB565_GREEN);
        break;
    }
    case 3: {
        landFill(RGB565_BLACK);
        int cx = LAND_W / 2, cy = LAND_H / 2;
        int maxR = (LAND_H / 2) - 10;
        for (int r = 10; r < maxR; r += 15) {
            for (int dy = -r; dy <= r; dy++) {
                int dx = (int)sqrt((double)(r * r - dy * dy));
                landFillRect(cx - dx, cy + dy, 2 * dx + 1, 1, palette[(r / 15) % 8]);
            }
        }
        break;
    }
    case 4:
        gfx->invertDisplay(true); delay(500);
        gfx->invertDisplay(false);
        break;
    case 5:
        landFill(palette[2]);
        landFillRect(0, 0, LAND_W, LAND_H / 2, palette[1]);
        break;
    case 6: {
        uint32_t t0 = micros();
        landFill(RGB565_RED);   flushLand();
        landFill(RGB565_GREEN); flushLand();
        landFill(RGB565_BLUE);  flushLand();
        landFill(RGB565_BLACK); flushLand();
        uint32_t dt = micros() - t0;
        Serial.printf("4 landscape flushes: %u us (%.1f fps)\n",
                      dt, 4.0f * 1e6f / dt);
        break;
    }
    case 7: {
        landFill(RGB565_BLACK);
        landDrawString(40, 80, "AXS15231B", RGB565_WHITE, RGB565_BLACK, 5);
        landDrawString(40, 150, "LANDSCAPE", RGB565_CYAN,  RGB565_BLACK, 5);
        landDrawString(40, 220, "480x320",   RGB565_YELLOW, RGB565_BLACK, 4);
        break;
    }
    case 8:
        drawSdContent();
        break;
    }
}

uint32_t lastTick = 0;
uint8_t   demoPhase = 0;

void loop() {
    uint32_t now = millis();

    TouchPoint tp;
    if (touch.read(&tp)) {
        int16_t lx = PHYS_H - 1 - tp.y;
        int16_t ly = tp.x;
        if (lx >= 0 && lx < LAND_W && ly >= 0 && ly < LAND_H) {
            landFillRect(lx - 2, ly - 2, 5, 5, RGB565_WHITE);
            flushLand();
        }
        Serial.printf("Touch raw=(%d,%d) -> land=(%d,%d)\n", tp.x, tp.y, lx, ly);
    }

    if (now - lastTick > 3000) {
        lastTick = now;
        drawDemoPhase(demoPhase);
        flushLand();
        Serial.printf("[DEMO] phase %d\n", demoPhase % 9);
        demoPhase++;
    }

    AudioApp::tick();
    WiFiApp::loop();
    BTApp::loop();

    if ((now % 2000) < 50) {
        int raw = analogRead(BAT_ADC);
        float v = raw * 3.3f / 4095.0f * 2.0f;
        Serial.printf("BAT: %d -> %.2f V\n", raw, v);
    }

    delay(5);
}