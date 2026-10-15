// T9: Current measurement (USB NOT connected, multimeter in the battery + line). Each press moves to the next state:
//   1 white screen -> 2 black screen -> 3 display asleep -> 4 + BLE advertising -> 5 off (same as T10 powerOff)
// The button wakes it from state 5; wake-up is a reset, so the sequence starts again at 1.
#include <Adafruit_GC9A01A.h>
#include <bluefruit.h>

const uint8_t BTN = D0, TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, DISP_PWR = D7;
const uint32_t SPI_HZ = 8000000;
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

void waitPress() {
  while (digitalRead(BTN) == HIGH) delay(10);
  delay(30);
  while (digitalRead(BTN) == LOW) delay(10);  // released before System OFF, or it wakes at once
  delay(30);
}

void flashSleep() {  // on-board QSPI flash into deep power-down (0xB9), see T10
  const uint8_t pins[] = {PIN_QSPI_CS, PIN_QSPI_SCK, PIN_QSPI_IO0, PIN_QSPI_IO2, PIN_QSPI_IO3};
  for (uint8_t p : pins) pinMode(p, OUTPUT);
  digitalWrite(PIN_QSPI_SCK, LOW);
  digitalWrite(PIN_QSPI_CS, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_QSPI_IO0, (0xB9 >> i) & 1);
    digitalWrite(PIN_QSPI_SCK, HIGH);
    digitalWrite(PIN_QSPI_SCK, LOW);
  }
  digitalWrite(PIN_QSPI_CS, HIGH);
}

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // LOW = display powered through Q2
  delay(50);
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
  SPI.end();                         // 5: display pins LOW first so they don't feed it, then cut its power
  const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
  for (uint8_t p : pins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  digitalWrite(DISP_PWR, HIGH);
  const uint8_t loads[] = {PIN_LSM6DS3TR_C_POWER, PIN_PDM_PWR};  // IMU and mic supplies
  for (uint8_t p : loads) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  flashSleep();
  systemOff(BTN, LOW);
}

void loop() {}
