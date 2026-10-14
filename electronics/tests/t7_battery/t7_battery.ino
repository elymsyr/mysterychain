// T7: Battery voltage (P0.31, 1M/510k divider) and charge status (P0.17, LOW = charging).
#include <Adafruit_TinyUSB.h>  // for Serial

const uint8_t CHG = 23;        // D23 = P0.17 (~CHG), variant.cpp
const float VBAT_CAL = 1.000;  // multimeter / reading; measure once, write it here

float readVbatMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(PIN_VBAT);
  return sum / 16.0f * 2400.0f / 4096.0f * 1510.0f / 510.0f * VBAT_CAL;
}

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);  // the core drives it HIGH at boot; LOW = measuring on, P0.31 safe
  pinMode(CHG, INPUT_PULLUP);
  analogReference(AR_INTERNAL_2_4);  // 4.2 V x 510/1510 = 1.42 V < 2.4 V
  analogReadResolution(12);
  analogSampleTime(40);  // 1 MOhm divider: a short sample time reads low
  Serial.begin(115200);
}

void loop() {
  Serial.printf("VBAT %.0f mV  %s\n", readVbatMv(), digitalRead(CHG) == LOW ? "CHARGING" : "not charging");
  delay(1000);
}
