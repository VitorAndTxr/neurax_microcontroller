#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/bluetooth/Bluetooth.h"

static const Bluetooth bluetooth;

void setup() {
  Adc::init();
  Fes::init();
  Semg::init();
}

void loop() {

}




