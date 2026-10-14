// T9: Current measurement (USB NOT connected, multimeter in the battery line). Each press moves to the next state:
//   1 white screen -> 2 black screen -> 3 display asleep -> 4 + BLE advertising -> 5 System OFF
// The button wakes it from System OFF; wake-up is a reset, so the sequence starts again at 1.
#include <Adafruit_GC9A01A.h>
#include <bluefruit.h>

const uint8_t BTN = D0, TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, DISP_PWR = D7;
const uint32_t SPI_HZ = 8000000;
const bool HAS_DISP_SWITCH = false;  // true if the AO3401A display switch (Q2) is fitted
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

void waitPress() {
  while (digitalRead(BTN) == HIGH) delay(10);
  delay(30);
  while (digitalRead(BTN) == LOW) delay(10);
  delay(30);
}

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  if (HAS_DISP_SWITCH) {
    pinMode(DISP_PWR, OUTPUT);
    digitalWrite(DISP_PWR, LOW);  // LOW = display powered
    delay(50);
  }
  tft.begin(SPI_HZ);
  tft.fillScreen(GC9A01A_WHITE);     // 1
  waitPress();
  tft.fillScreen(GC9A01A_BLACK);     // 2
  waitPress();
  tft.sendCommand(GC9A01A_DISPOFF);  // 3: backlight stays on (module has no BL pin)
  tft.sendCommand(GC9A01A_SLPIN);
  waitPress();
  Bluefruit.begin();                 // 4
  Bluefruit.setName("MysteryChain");
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start(0);
  waitPress();
  if (HAS_DISP_SWITCH) {             // don't feed the display through its signal pins: all LOW, then cut power
    SPI.end();
    const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
    for (uint8_t p : pins) {
      pinMode(p, OUTPUT);
      digitalWrite(p, LOW);
    }
    digitalWrite(DISP_PWR, HIGH);
  }
  systemOff(BTN, LOW);               // 5
}

void loop() {}
