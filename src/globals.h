#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

static const BaseType_t session_cpu = 0;
static const BaseType_t secondary_cpu = 1;
static SemaphoreHandle_t i2cMutex;

#endif