#include "Gyroscope.h"



void Gyroscope::init() {

}

angle Gyroscope::acquire() {
   return 0.0f; 
}

angle Gyroscope::getLastValue() {
    return Gyroscope::last_value;
}

void Gyroscope::sendLastValue() {
}