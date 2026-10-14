// T2: IMU (LSM6DS3TR-C, internal Wire1). Watch with the Serial Monitor or Serial Plotter.
#include <Adafruit_TinyUSB.h>  // for Serial
#include <LSM6DS3.h>

LSM6DS3 imu(I2C_MODE, 0x6A);
const float SHAKE_G = 0.8;  // "shake" when |a| deviates from 1 g by this much

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  if (imu.begin() != 0) {
    Serial.println("NO IMU: begin() failed");
    while (true) {}
  }
  Serial.println("IMU OK");
}

void loop() {
  float ax = imu.readFloatAccelX(), ay = imu.readFloatAccelY(), az = imu.readFloatAccelZ();
  float a = sqrtf(ax * ax + ay * ay + az * az);
  Serial.printf("ax:%.2f ay:%.2f az:%.2f a:%.2f gx:%.0f gy:%.0f gz:%.0f shake:%d\n",
                ax, ay, az, a, imu.readFloatGyroX(), imu.readFloatGyroY(), imu.readFloatGyroZ(),
                fabsf(a - 1.0f) > SHAKE_G);
  delay(100);
}
