#ifndef BLUETOOTH_MODULE
#define BLUETOOTH_MODULE

#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "../led/Led.h"

static const char* TAG_BLU = "BLU";

class Bluetooth
{
private:
    static bool connected;
public:
    Bluetooth() = delete;
    ~Bluetooth() = delete;
    static void init();
    static String readData();
    static bool sendData(String &data);
    static bool sendRawData(const uint8_t* data, size_t length);  // NEW: Binary protocol support
    static bool isConnected();
	static void waitForConnection();
};

#endif