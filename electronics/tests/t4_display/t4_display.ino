// T4: GC9A01 display. Color order, edge circle, text and a 30 min endurance counter.
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>

const uint8_t TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, DISP_PWR = D7;
const uint32_t SPI_HZ = 8000000;  // 8 MHz on long bench wires; try 0 (library default) once wired permanently
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // Q2 on (harmless before Q2 is fitted)
  delay(50);
  tft.begin(SPI_HZ);
  const uint16_t colors[] = {GC9A01A_RED, GC9A01A_GREEN, GC9A01A_BLUE, GC9A01A_WHITE, GC9A01A_BLACK};
  const char *names[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};
  for (int i = 0; i < 5; i++) {
    uint32_t t = micros();
    tft.fillScreen(colors[i]);
    Serial.printf("%s  full screen: %lu us\n", names[i], micros() - t);
    delay(1000);
  }
  tft.drawCircle(120, 120, 119, GC9A01A_WHITE);  // edge circle: should look equally thick everywhere
  tft.fillCircle(85, 100, 15, GC9A01A_WHITE);    // eyes
  tft.fillCircle(155, 100, 15, GC9A01A_WHITE);
  tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
  tft.setTextSize(2);
}

void loop() {
  tft.setCursor(66, 160);
  tft.printf("%6lu s", millis() / 1000);
  delay(1000);
}
