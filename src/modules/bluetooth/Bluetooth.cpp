#include "Bluetooth.h"

bool Bluetooth::connected = false;
HardwareSerial BTSerial(2); // Use UART2 (TX2, RX2) for Bluetooth

void Bluetooth::init() {
    BTSerial.begin(9600);  // Bluetooth module baud rate
    BTSerial.write(BLUETOOTH_AT_SET_MODULE_NAME);
    BTSerial.write(BLUETOOTH_AT_SET_BAUDRATE_115200);
    /* BTSerial.end(true);
    BTSerial.begin(115200); */
    pinMode(BLUETOOTH_MODULE_STATUS_PIN, INPUT); 
    Bluetooth::waitForConnection();
}

bool Bluetooth::isConnected() {
    return digitalRead(BLUETOOTH_MODULE_STATUS_PIN);
}

void Bluetooth::waitForConnection() {
	while (!Bluetooth::isConnected()) {}
}

String Bluetooth::readData() {
    String data = "";
    if (BTSerial.available()) {
        data = BTSerial.readStringUntil('\0');
        return data;
    }
    return data;
}

void Bluetooth::sendData(const String &data) {
    BTSerial.print(data); 
}
