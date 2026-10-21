// T10: Power button. The button turns the board on and off in firmware (battery switch SW2 stays on).
//   Off: hold 3 s, the screen says "release", let go -> display power cut (Q2), System OFF.
//   On:  hold ~1 s. A short tap wakes the chip, it sees the button already released and goes straight back off.
//   Plugging in USB wakes it too (VBUS) and it stays on. Without USB, below VBAT_OFF_MV it turns itself off.
// Reference for the main firmware: powerOff() and the wake check at the top of setup().
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>

const uint8_t BTN = D0, TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, DISP_PWR = D7, CHG = 23;
const uint32_t SPI_HZ = 8000000;                  // value that worked in T4
const uint32_t ON_HOLD_MS = 1000, OFF_HOLD_MS = 3000;
const float VBAT_CAL = 1.020;                     // value found in T7
const float VBAT_OFF_MV = 3400;                   // turn off below this without USB (the PCM cuts much lower)

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
bool dispOn;

float readVbatMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(PIN_VBAT);
  return sum / 16.0f * 2400.0f / 4096.0f * 1510.0f / 510.0f * VBAT_CAL;
}

// no SoftDevice in this sketch; with BLE running use sd_power_usbregstatus_get()
bool usbIn() { return NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk; }

// On-board QSPI flash: ~25 uA in standby, ~0 in deep power-down (command 0xB9; 0xAB wakes it before any use).
void flashSleep() {
  const uint8_t pins[] = {PIN_QSPI_CS, PIN_QSPI_SCK, PIN_QSPI_IO0, PIN_QSPI_IO2, PIN_QSPI_IO3};
  for (uint8_t p : pins) pinMode(p, OUTPUT);  // the core preloads every output HIGH: CS, WP#, HOLD# idle
  digitalWrite(PIN_QSPI_SCK, LOW);
  digitalWrite(PIN_QSPI_CS, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_QSPI_IO0, (0xB9 >> i) & 1);
    digitalWrite(PIN_QSPI_SCK, HIGH);
    digitalWrite(PIN_QSPI_SCK, LOW);
  }
  digitalWrite(PIN_QSPI_CS, HIGH);  // pin states are kept in System OFF
}

void powerOff() {
  digitalWrite(MOTOR, LOW);
  if (dispOn) {  // drive the display pins LOW first, or the display keeps running through them
    SPI.end();
    const uint8_t pins[] = {TFT_CS, TFT_DC, TFT_RST, PIN_SPI_SCK, PIN_SPI_MOSI};
    for (uint8_t p : pins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  }
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, HIGH);  // Q2 off: display and backlight dark
  const uint8_t loads[] = {PIN_LSM6DS3TR_C_POWER, PIN_PDM_PWR};  // IMU and mic supplies (GPIO-powered)
  for (uint8_t p : loads) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  flashSleep();
  // ponytail: a button stuck LOW keeps it here (~1 mA, display already off); a timeout would need a non-button wake
  while (digitalRead(BTN) == LOW) delay(10);  // System OFF with the button still held = instant wake (nRF52840 PS 5.3.3)
  delay(50);                                   // release bounce
  systemOff(BTN, LOW);                         // wake = reset: setup() starts over
}

void say(const char *s, uint16_t color) {
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(color, GC9A01A_BLACK);
  tft.setTextSize(3);
  tft.setCursor(120 - strlen(s) * 9, 108);
  tft.print(s);
}

void setup() {
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);  // the core preloads outputs HIGH: without this the motor runs (and stays on in System OFF)
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(CHG, INPUT_PULLUP);
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  analogSampleTime(40);

  uint32_t why = readResetReason();
  if (why & POWER_RESETREAS_OFF_Msk) {  // woken by the button: stay on only if it is held
    while (digitalRead(BTN) == LOW && millis() < ON_HOLD_MS) delay(10);
    if (millis() < ON_HOLD_MS) powerOff();
  }

  Serial.begin(115200);
  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // Q2 on (harmless without Q2: D7 is then unconnected)
  delay(50);                    // module LDO up before the reset pulse
  tft.begin(SPI_HZ);
  dispOn = true;
  say(why & POWER_RESETREAS_OFF_Msk ? "button" : why & POWER_RESETREAS_VBUS_Msk ? "USB" : why ? "reset" : "power on", GC9A01A_GREEN);
  tft.setTextSize(1);
  tft.setCursor(84, 140);
  tft.printf("RESETREAS 0x%lX", why);
  Serial.printf("RESETREAS 0x%lX\n", why);
  delay(1500);
  tft.fillScreen(GC9A01A_BLACK);
  while (digitalRead(BTN) == LOW) delay(10);  // the power-on hold must not count toward power-off
}

void loop() {
  static uint32_t tDown, lastDraw;
  uint32_t now = millis();
  if (digitalRead(BTN) == HIGH) tDown = 0;
  else if (!tDown) tDown = now;  // ponytail: debounce-free, a bounce only restarts the hold

  float mv = readVbatMv();
  bool usb = usbIn();
  if (tDown && now - tDown >= OFF_HOLD_MS) {
    say("release", GC9A01A_YELLOW);
    powerOff();
  }
  if (!usb && mv < VBAT_OFF_MV) {
    say("LOW BAT", GC9A01A_RED);
    Serial.printf("VBAT %.0f mV < %.0f: off\n", mv, VBAT_OFF_MV);
    delay(2000);
    powerOff();
  }

  if (now - lastDraw >= 250) {
    lastDraw = now;
    tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
    tft.setTextSize(2);
    tft.setCursor(60, 70);
    tft.printf("%6lu s", now / 1000);
    tft.setCursor(48, 110);
    tft.printf("%4.0fmV %s", mv, digitalRead(CHG) == LOW ? "CHG" : "   ");
    tft.setCursor(48, 150);
    tft.print(usb ? "USB     " : "battery ");
    tft.setCursor(48, 180);
    tft.printf("hold %4.1f s", tDown ? (now - tDown) / 1000.0f : 0.0f);
  }
  delay(10);
}
