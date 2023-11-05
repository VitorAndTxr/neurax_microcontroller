#include "Gyroscope.h"

const int Gyroscope::gyroscope_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Gyroscope::error = false;
Adafruit_MPU6050 Gyroscope::mpu;
int minAccelY, maxAccelY;
double currentPitch, previousPitch;
unsigned long previousTime;

void Gyroscope::init()
{

    if (!mpu.begin()) {
        printDebug("MPU6050 connection failed. Please check your connections.");
        Gyroscope::error = true;
        while (1);
    }

}

void Gyroscope::calibrateMPU6050() {

  for (int i = 0; i < 1000; i++) {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    if (i == 0) {
      minAccelY = maxAccelY = accel.acceleration.y;
    } else {
      if (accel.acceleration.y < minAccelY) minAccelY = accel.acceleration.y;
      if (accel.acceleration.y > maxAccelY) maxAccelY = accel.acceleration.y;
    }
    delay(1);
  }
}

double Gyroscope::calculatePitch() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  //int pitchValue = map(accel.acceleration.y, minAccelY, maxAccelY, -90, 90);
  double pitch = RAD_TO_DEG * (atan2(accel.acceleration.x, accel.acceleration.z) + PI);
  return pitch;
}

