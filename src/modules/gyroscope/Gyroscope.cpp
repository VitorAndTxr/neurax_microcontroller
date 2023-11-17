#include "Gyroscope.h"

const int Gyroscope::gyroscope_i2c_address = ADC_I2C_ADDR; // already set in lib
float Gyroscope::last_value = 0.0f;

bool Gyroscope::error = false;
Adafruit_MPU6050 Gyroscope::mpu;
int minAccelY, maxAccelY;
double currentPitch, previousPitch;
unsigned long previousTime;

#if GYROSCOPE_MODULE_ENABLE
void gyroscopeDisabledWarning() {
	ESP_LOGW(TAG_GYRO, "Gyroscope module is disabled.");
}
#endif


void Gyroscope::init()
{
#if GYROSCOPE_MODULE_ENABLE
	ESP_LOGI(TAG_GYRO, "Setting up...");

    if (!mpu.begin()) {
		ESP_LOGE(TAG_GYRO, "MPU6050 connection failed. Please check your connections.");
        while (1) {};
    }
	ESP_LOGI(TAG_GYRO, "Setup done.");
#else
	void gyroscopeDisabledWarning();
#endif
}

void Gyroscope::calibrateMPU6050() {
#if GYROSCOPE_MODULE_ENABLE

	ESP_LOGI(TAG_GYRO, "Starting calibration...");

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

	// TODO send calibration done message
	ESP_LOGI(TAG_GYRO, "Finished calibration!");
#else
	void gyroscopeDisabledWarning();
#endif
}

float Gyroscope::calculatePitch() {
#if GYROSCOPE_MODULE_ENABLE
	sensors_event_t accel, gyro, temp;
	mpu.getEvent(&accel, &gyro, &temp);
	//int pitchValue = map(accel.acceleration.y, minAccelY, maxAccelY, -90, 90);
	float pitch = RAD_TO_DEG * (atan2(accel.acceleration.x, accel.acceleration.z) + PI);
	return pitch;
#else
	void gyroscopeDisabledWarning();
	return 0.0f;
#endif
}

float Gyroscope::gyroscopeRoutine(){
#if GYROSCOPE_MODULE_ENABLE
	ESP_LOGI(TAG_GYRO, "Starting gyroscope routine...");

    int gyroscope_time = 10000; //10 segundos
    float initial_position, aux_value, movement_result; 
    

    //calibra
    Gyroscope::calibrateMPU6050();

    //le o primeiro valor
    initial_position = Gyroscope::calculatePitch();

    //le o segundo valor e salva a diferença 
    aux_value = Gyroscope::calculatePitch();
    if ((aux_value - initial_position) < 0){
        movement_result = 0;
    }
    else{
        movement_result = aux_value - initial_position;
    }

    // tempo de duracao da medida
	ESP_LOGI(TAG_GYRO, "Starting measurement...");
    unsigned long startTime = millis();
    while (millis() - startTime < gyroscope_time) {
        aux_value = Gyroscope::calculatePitch();
        
        if ((aux_value - initial_position) > movement_result){
            //Serial.println(aux_value);
            movement_result = aux_value - initial_position;
        }
        // Add some delay to avoid constant checking, adjust as needed
        //delay(100); 
    }
	ESP_LOGI(TAG_GYRO, "Finished measurement.");
	ESP_LOGI(TAG_GYRO, "Exiting gyroscope routine.");
	
	last_value = movement_result;
    return movement_result;

#else
	void gyroscopeDisabledWarning();
	return 0.0f;
#endif
}

void Gyroscope::testGyroscope(){
#if GYROSCOPE_MODULE_ENABLE
	ESP_LOGI(TAG_GYRO, "Testing...");

    //calibra
    Gyroscope::calibrateMPU6050();

    //le o primeiro valor
    float position;
    position = Gyroscope::calculatePitch();

	ESP_LOGD(TAG_GYRO, "Read: %lf", position);
	ESP_LOGI(TAG_GYRO, "Finished testing gyroscope.");
#endif
}

angle Gyroscope::getLastValue(){
    return Gyroscope::last_value;
}

void Gyroscope::sendLastValue(){
	ESP_LOGI(TAG_GYRO, "Building measurement message...");
    DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(3));

    (*message_document)[MESSAGE_KEYS::CODE] = GYROSCOPE_MESSAGE;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

    JsonObject body = 
      (*message_document).createNestedObject(MESSAGE_KEYS::BODY);

    body[MESSAGE_KEYS::ANGLE] = 
      last_value;
	
	ESP_LOGI(TAG_GYRO, "Sending measurement message...");
    MessageHandler::sendMessage(message_document);
	ESP_LOGI(TAG_GYRO, "Measurement sent.");
}
