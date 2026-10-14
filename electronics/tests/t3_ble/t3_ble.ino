// T3: BLE advertising. Find "MysteryChain" with nRF Connect on the phone and read the RSSI.
#include <bluefruit.h>

void setup() {
  Bluefruit.begin();
  Bluefruit.setTxPower(0);  // dBm; test with the value the final firmware will use
  Bluefruit.setName("MysteryChain");
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);  // 0 = forever
}

void loop() {}
