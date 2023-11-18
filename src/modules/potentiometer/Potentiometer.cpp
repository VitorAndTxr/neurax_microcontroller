#include "Potentiometer.h"
#include "modules/adc/Adc.h"

void Potentiometer::init() {
    pinMode(POTENTIOMETER_PIN_INCREMENT, OUTPUT);
    pinMode(POTENTIOMETER_PIN_UP_DOWN, OUTPUT);
    pinMode(POTENTIOMETER_PIN_CS, OUTPUT);
    //digitalWrite(POTENTIOMETER_PIN_CS, LOW);

}

float Potentiometer::getCorrectedVoltage()
{
    //read the voltage of the potentiometer using the adc 
	// TODO define pin
    float voltage_adc = Adc::getValue(1);
    
    //corrige o valor devido a divisão de tensão
	// TODO
    float current_voltage = 4 * voltage_adc;
	ESP_LOGD(TAG_POT, "Corrected voltage: %lf", current_voltage);

    return current_voltage;
}

float Potentiometer::voltageSet(float target_voltage)
{
	ESP_LOGI(TAG_POT, "Setting voltage...");
    
    volatile bool last_mov_up = false;
    volatile float erro, last_erro;
    last_erro = 20;
    
    volatile bool found = false;
    volatile float current_voltage;
   // Serial.println("last erro");
    //Serial.println(last_erro);
    while (!found) {
        current_voltage = Potentiometer::getCorrectedVoltage();

        // Simula ajustes na tensão (aumentar ou diminuir)
       // Serial.println("----------------------------- current and target");
       // Serial.println(current_voltage);
        //Serial.println(target_voltage);
        erro = fabs(target_voltage - current_voltage);
        if (current_voltage < target_voltage) {
            Potentiometer::increase(1);
            last_mov_up = true;
            //Serial.println("increased");
        } else {
            Potentiometer::decrease(1);
            last_mov_up = false;
           // Serial.println("decreased");
        }

        current_voltage = Potentiometer::getCorrectedVoltage();
        //Serial.println("new voltage");
        //Serial.println(current_voltage);
        last_erro = erro;
        erro = fabs(target_voltage - current_voltage);
        //delay(5000);
        //Serial.println(current_voltage);
        if (fabs(last_erro)<fabs(erro)){
            //delay(1000);
            //Serial.println("END 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>");
            
            //Serial.print(last_erro);
           //Serial.print("<");
            //Serial.println(erro);
            //Serial.println(last_erro);
           // Serial.println(erro);
            found = true;
            if (last_mov_up) {
                Potentiometer::decrease(1);
            }
            else {
                Potentiometer::increase(1);
            }
            current_voltage = Potentiometer::getCorrectedVoltage();
            //Serial.println(current_voltage);
        }
        else if (fabs(last_erro)==fabs(erro)) {
            //Serial.println("END 2 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
            found = true;
        }
        delay(100);
    }

    return Potentiometer::getCorrectedVoltage();
}

void Potentiometer::pulse(int steps) 
{
    for(int i = 0; i< steps; i++){
        digitalWrite(POTENTIOMETER_PIN_INCREMENT, HIGH);
        delayMicroseconds(10);
        digitalWrite(POTENTIOMETER_PIN_INCREMENT, LOW);
        delayMicroseconds(10);
        
    }
}

inline void Potentiometer::increase(int steps) 
{
    digitalWrite(POTENTIOMETER_PIN_UP_DOWN, HIGH);
    digitalWrite(POTENTIOMETER_PIN_CS, LOW);
    
    pulse(steps);
	ESP_LOGI(TAG_POT, "Voltage up");

}


inline void Potentiometer::decrease(int steps) 
{
    digitalWrite(POTENTIOMETER_PIN_UP_DOWN, LOW);
    digitalWrite(POTENTIOMETER_PIN_CS, LOW);

    pulse(steps);

	ESP_LOGI(TAG_POT, "Voltage down");
}