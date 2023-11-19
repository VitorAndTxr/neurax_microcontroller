#include <Arduino.h>
#include <gpioConfig.h>


void setup(){
  Serial.begin(115200);
  initGpio();
}

void voltageUp(int passos){
  digitalWrite(UP_DOWN_POTENCIOMETER, HIGH);
  digitalWrite(CS_POTENCIOMETER, LOW);
  for(int i = 0; i< passos; i++){
    digitalWrite(INCREMENT_POTENCIOMETER, HIGH);
    delayMicroseconds(10000);
    digitalWrite(INCREMENT_POTENCIOMETER, LOW);
  }
  Serial.println("Voltage Up");

}
void voltageDown(int passos){
  digitalWrite(UP_DOWN_POTENCIOMETER, LOW);
  digitalWrite(CS_POTENCIOMETER, LOW);
  for(int i = 0; i< passos; i++){
    digitalWrite(INCREMENT_POTENCIOMETER, HIGH);
    delayMicroseconds(10);
    digitalWrite(INCREMENT_POTENCIOMETER, LOW);
  }
  Serial.println("Voltage Down");
}

void negativeHbridge(){
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
    digitalWrite(H_BRIDGE_INPUT_1, HIGH);
}

void positiveHbridge(){
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, HIGH);
}
void hBridgeReset(){
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
}

void estimulate(){
  for(int i= 0; i<120; i++){

    positiveHbridge();
    delayMicroseconds(300);
    negativeHbridge();
    delayMicroseconds(300);
    hBridgeReset();
    delayMicroseconds(16666);
  }
}


 
void loop(){
  if(Serial.available()){
    char c = Serial.read();

    if(c=='u')
    {
      voltageUp(1);  
    }
    
    if(c=='d'){
      voltageDown(1);
    }
    if(c=='p'){
      positiveHbridge();
    }

    if(c=='n'){
      negativeHbridge();
    }
    if(c=='e'){
      estimulate();
    }
  }

}



