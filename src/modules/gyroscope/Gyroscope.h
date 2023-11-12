#ifndef GYROSCOPE_MODULE
#define GYROSCOPE_MODULE

typedef float angle;
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include "../debug/debug.h"
#include "../message_handler/MessageHandler.h"

class Gyroscope
{
private:
    static float last_value;
public:

    Gyroscope() = delete;
    ~Gyroscope() = delete;
    static void init();
    static angle getLastValue();
    static void sendLastValue();

    static const int gyroscope_i2c_address; 
    //static Adafruit_ADS1115 ads;
    static Adafruit_MPU6050 mpu;
    static bool error;
public:
    static void calibrateMPU6050();
    static float calculatePitch();
    //TODO mutex needed?
    static float gyroscopeRoutine(); 
    static void testGyroscope();
};

#endif