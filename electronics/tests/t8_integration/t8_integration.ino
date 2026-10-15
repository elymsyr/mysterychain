// T8: Integration test. Display + button + motor + IMU + battery together.
//   Short press: motor 150 ms.  Long press: 10-pulse motor stress.  Shake: face changes.
//   If the uptime on screen restarts, the board has reset (supply dip on a motor pulse while on battery).
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Adafruit_GC9A01A.h>
#include <LSM6DS3.h>

const uint8_t BTN = D0, TFT_CS = D1, TFT_DC = D2, TFT_RST = D3, MOTOR = D6, DISP_PWR = D7, CHG = 23;
const uint32_t SPI_HZ = 8000000;     // value that worked in T4
const float VBAT_CAL = 1.000;        // value found in T7
const float SHAKE_G = 0.8;
const uint32_t MOTOR_MASK_MS = 400;  // ignore the IMU this long after a pulse ends; tune from the serial output
const uint32_t DEBOUNCE_MS = 25, LONG_MS = 600;

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
LSM6DS3 imu(I2C_MODE, 0x6A);

bool happy = true, stable = HIGH, lastRaw = HIGH;
uint32_t tChange, tDown, motorEnd, lastShake, motorTail, presses, shakes;

void motor(uint32_t ms) {
  digitalWrite(MOTOR, HIGH);
  delay(ms);
  digitalWrite(MOTOR, LOW);
  motorEnd = millis();
}

void drawFace() {
  tft.fillRect(70, 118, 100, 62, GC9A01A_BLACK);
  tft.fillCircle(120, happy ? 140 : 185, 30, GC9A01A_YELLOW);  // mouth: crescent from two circles
  tft.fillCircle(120, happy ? 130 : 195, 30, GC9A01A_BLACK);
  tft.fillCircle(85, 100, 14, GC9A01A_WHITE);
  tft.fillCircle(155, 100, 14, GC9A01A_WHITE);
}

float readVbatMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(PIN_VBAT);
  return sum / 16.0f * 2400.0f / 4096.0f * 1510.0f / 510.0f * VBAT_CAL;
}

void drawStatus() {
  tft.setTextSize(2);
  tft.setCursor(60, 36);
  tft.printf("%6lu s", millis() / 1000);
  tft.setCursor(54, 190);
  tft.printf("%4.0fmV %s", readVbatMv(), digitalRead(CHG) == LOW ? "CHG" : "   ");
  tft.setTextSize(1);
  tft.setCursor(60, 214);
  tft.printf("btn %lu  shake %lu  ", presses, shakes);
}

void setup() {
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(CHG, INPUT_PULLUP);
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  analogSampleTime(40);
  Serial.begin(115200);

  pinMode(DISP_PWR, OUTPUT);
  digitalWrite(DISP_PWR, LOW);  // Q2 on (harmless before Q2 is fitted)
  delay(50);
  tft.begin(SPI_HZ);
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setCursor(48, 110);
  tft.printf("reset 0x%lX", readResetReason());
  bool imuOk = imu.begin() == 0;
  tft.setCursor(48, 130);
  tft.print(imuOk ? "IMU OK" : "NO IMU");
  delay(2000);
  tft.fillScreen(GC9A01A_BLACK);
  drawFace();
}

void loop() {
  // button
  bool raw = digitalRead(BTN);
  if (raw != lastRaw) { lastRaw = raw; tChange = millis(); }
  if (raw != stable && millis() - tChange > DEBOUNCE_MS) {
    stable = raw;
    if (stable == LOW) {
      tDown = millis();
    } else {
      presses++;
      if (millis() - tDown >= LONG_MS) {
        for (int i = 0; i < 10; i++) { motor(150); delay(250); }
      } else {
        motor(150);
      }
    }
  }

  // IMU: shake + the vibration tail the motor leaves on the IMU
  float ax = imu.readFloatAccelX(), ay = imu.readFloatAccelY(), az = imu.readFloatAccelZ();
  float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
  uint32_t since = millis() - motorEnd;
  if (dev > SHAKE_G) {
    if (since < 1000) motorTail = since;
    if (since > MOTOR_MASK_MS && millis() - lastShake > 500) {
      lastShake = millis();
      shakes++;
      happy = !happy;
      drawFace();
    }
  }
  if (motorTail && since >= 1000) {
    Serial.printf("IMU tail after motor: %lu ms  (MOTOR_MASK_MS = %lu)\n", motorTail, MOTOR_MASK_MS);
    motorTail = 0;
  }

  static uint32_t lastDraw;
  if (millis() - lastDraw >= 1000) {
    lastDraw = millis();
    drawStatus();
  }
  delay(20);
}
