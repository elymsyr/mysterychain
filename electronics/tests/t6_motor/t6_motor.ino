// T6: Motor driver (D6 -> 1k -> BC337). Send from the Serial Monitor:
//   1 / 2 / 3 = 100 / 200 / 300 ms pulse,  s = 20-pulse stress,  h = 3 s continuous (for measuring)
// If the "BOOT" line shows up again, the board has reset.
#include <Adafruit_TinyUSB.h>  // for Serial

const uint8_t MOTOR = D6;

void pulse(uint32_t ms) {
  digitalWrite(MOTOR, HIGH);
  delay(ms);
  digitalWrite(MOTOR, LOW);
}

void setup() {
  pinMode(MOTOR, OUTPUT);
  digitalWrite(MOTOR, LOW);
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.printf("BOOT  reset reason = 0x%lX\n", readResetReason());
}

void loop() {
  switch (Serial.read()) {
    case '1': pulse(100); break;
    case '2': pulse(200); break;
    case '3': pulse(300); break;
    case 's': for (int i = 0; i < 20; i++) { pulse(200); delay(300); } break;
    case 'h': pulse(3000); break;
  }
  static uint32_t last;
  if (millis() - last >= 2000) {
    last = millis();
    Serial.printf("alive %lu ms\n", last);
  }
}
