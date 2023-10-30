#include <Arduino.h>
#include "modules/bluetooth/Bluetooth.h"
#include "modules/fes/Fes.h"
#include "modules/adc/Adc.h"

static const Bluetooth bluetooth;

void setup() {
  Adc::init();
  Fes::init();
}

void loop() {

}




