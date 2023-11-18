#include "Led.h"
#include <Arduino.h>

Led::Led(int pin) {
    this->pin = pin;
    pinMode(pin, OUTPUT);

    this->set(false);
}

Led::~Led() {
}

void Led::setOn() {
	set(true);
}

void Led::toggle() {
    this->status = !this->status;
    this->set(this->status);
}

void Led::blink(int amount, int interval) {
    for (int i = 0; i < amount; i++) {
        this->set(true);
        delay(interval);
        this->set(false);
        delay(interval);
    }    
}

void Led::set(bool on) {
    digitalWrite(this->pin, on);
    this->status = on;
}

void Led::setTriggerLedOff(void * obj) {
	digitalWrite(LED_PIN_TRIGGER, LOW);
    ESP_LOGI("LED", "Starting timer");
}

void Led::turnOnFor(int ms) {
    timer = xTimerCreate(
        "LED timer",           // Nome do temporizador (para fins de depuração)
        pdMS_TO_TICKS(ms),  // Período em milissegundos
        pdFALSE,              // Modo autoreload, o temporizador será recarregado automaticamente
        (void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
        setTriggerLedOff        // Função a ser chamada quando o temporizador expirar
    );
    if (timer != NULL) {
        xTimerStart(timer, 0);
        //this->set(true);
	    digitalWrite(LED_PIN_TRIGGER, HIGH);

		ESP_LOGI("LED", "Starting timer");
    } else {
		ESP_LOGW("LED", "Error creating timer!");
    }
}