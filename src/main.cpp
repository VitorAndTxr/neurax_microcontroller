#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/bluetooth/Bluetooth.h"

void setup() {
  Adc::init();
  Fes::init();
  Semg::init();
  Bluetooth::init();
}

void loop() {

}




