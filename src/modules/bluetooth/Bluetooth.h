#ifndef BLUETOOTH_MODULE
#define BLUETOOTH_MODULE

#include <HardwareSerial.h>
#include <ArduinoJson.h>

class Bluetooth
{
private:
    static bool connected;
public:
    Bluetooth() = delete;
    ~Bluetooth() = delete;
    static void init();
    static String readData();
    static void sendData(const String &data);
    static bool waitForConnection();
    static bool isConnected();
};

#endif