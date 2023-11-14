#include "Potentiometer.h"
#include "modules/adc/Adc.h"

Potentiometer::Potentiometer()
{
}

void Potentiometer::init()
{
    pinMode(POTENTIOMETER_PIN_INCREMENT, OUTPUT);
    pinMode(POTENTIOMETER_PIN_UP_DOWN, OUTPUT);
    pinMode(POTENTIOMETER_PIN_CS, OUTPUT);

}

Potentiometer::~Potentiometer()
{
}

float Potentiometer::getCorrectedVoltage()
{
    //read the voltage of the potentiometer using the adc 
    float voltage_adc = Adc::getValue(0);
    
    //corrige o valor devido a divisão de tensão
    float current_voltage = voltage_adc;

    return current_voltage;
}

float Potentiometer::voltageSet(float target_voltage)
{
    // 
    volatile bool last_mov_up = false;
    volatile float erro, last_erro;
    last_erro = 20;
    
    volatile bool found = false;
    volatile float current_voltage;
    while (!found) {
        current_voltage = Potentiometer::getCorrectedVoltage();

        // Simula ajustes na tensão (aumentar ou diminuir)
        if (current_voltage < target_voltage) {
            Potentiometer::increase(1);
            last_mov_up = true;
        } else {
            Potentiometer::decrease(1);
            last_mov_up = false;
        }

        current_voltage = Potentiometer::getCorrectedVoltage();
 
        last_erro = erro;
        erro = target_voltage - current_voltage;

        if (abs(last_erro)<abs(erro)){
            //delay(1000);
            /*Serial.println("erros");
            Serial.println(last_erro);
            Serial.println(erro);*/
            found = true;
            if (last_mov_up){
                Potentiometer::decrease(1);
            }
            else{
                Potentiometer::increase(1);
            }
        }
        else if (abs(last_erro)==abs(erro)){
            Serial.println("erros2");
            found = true;
        }
        
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

    //printDebug("Voltage up");
}


inline void Potentiometer::decrease(int steps) 
{
    digitalWrite(POTENTIOMETER_PIN_UP_DOWN, LOW);
    digitalWrite(POTENTIOMETER_PIN_CS, LOW);

    pulse(steps);

    //printDebug("Voltage down");
}