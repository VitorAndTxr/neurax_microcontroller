#include "Gyroscope.h"

const int Gyroscope::gyroscope_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Gyroscope::error = false;
Adafruit_MPU6050 Gyroscope::mpu;
int minAccelY, maxAccelY;
double currentPitch, previousPitch;
unsigned long previousTime;
float Gyroscope::last_value = 0.0;

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

float Gyroscope::calculatePitch() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  //int pitchValue = map(accel.acceleration.y, minAccelY, maxAccelY, -90, 90);
  float pitch = RAD_TO_DEG * (atan2(accel.acceleration.x, accel.acceleration.z) + PI);
  return pitch;
}

float Gyroscope::gyroscopeRoutine(){
    
    int gyroscope_time = 3000; //3 segundos
    float initial_position, aux_value, movement_result; 
    unsigned long startTime = millis();

    //calibra
    Gyroscope::calibrateMPU6050();

    //le o primeiro valor
    initial_position = Gyroscope::calculatePitch();

    //le o segundo valor e salva a diferença 
    aux_value = Gyroscope::calculatePitch();
    if ((initial_position - aux_value) < 0){
        movement_result = 0;
    }
    else{
        movement_result = initial_position - aux_value;
    }

    // tempo de duracao da medida
    while (millis() - startTime < gyroscope_time) {
        aux_value = Gyroscope::calculatePitch();
        if ((initial_position - aux_value) > movement_result){
            movement_result = initial_position - aux_value;
        }
        // Add some delay to avoid constant checking, adjust as needed
        //delay(100); 
    }
    last_value = movement_result;
    return movement_result;

}

void Gyroscope::testGyroscope(){
    printDebug("testing gyroscope");
    //calibra
    Gyroscope::calibrateMPU6050();

    //le o primeiro valor
    float position;
    position = Gyroscope::calculatePitch();
    printDebug("testing gyroscope ending");


}

angle Gyroscope::getLastValue(){
    return last_value;
}

void Gyroscope::sendLastValue(){
    DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(3));

    (*message_document)[MESSAGE_KEYS::CODE] = GYROSCOPE_MESSAGE;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

    JsonObject body = 
      (*message_document).createNestedObject(MESSAGE_KEYS::BODY);

    body[MESSAGE_KEYS::ANGLE] = 
      last_value;

    MessageHandler::addMessageToQueue(message_document);
}

