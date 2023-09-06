#include <Arduino.h>
#include "esp_system.h"

#define LED 16

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const int ledPin = 16;
const int freq = 2;
const int ledChannel = 0;
const int resolution = 8;
hw_timer_t *My_timer = NULL;


void initPwm(){
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(ledPin, ledChannel);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Iniciando");
  pinMode(LED, OUTPUT);
  initPwm();
  ledcWrite(ledChannel, 128);

}
void loop() {
  Serial.println("rodando");
  Serial.println(ledcRead(ledChannel));
  delayMicroseconds(100000);
}



// put function definitions he