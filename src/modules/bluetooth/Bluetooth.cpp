#include "Bluetooth.h"

bool Bluetooth::connected = false;
HardwareSerial BTSerial(2); // Use UART2 (TX2, RX2) for Bluetooth

void Bluetooth::init() {
    pinMode(BLUETOOTH_MODULE_STATUS_PIN, INPUT); 
    BTSerial.begin(9600);  // Bluetooth module baud rate
    //BTSerial.write("AT+NAMENeuroEstimulator");
    //delay(500);
    //BTSerial.flush();
    //BTSerial.write("AT+BAUD4");
    //BTSerial.write(BLUETOOTH_AT_SET_BAUDRATE_115200);
    /* BTSerial.end(true);
    BTSerial.begin(115200); */
    //Bluetooth::waitForConnection();
}

bool Bluetooth::isConnected() {
    return digitalRead(BLUETOOTH_MODULE_STATUS_PIN);
}

void Bluetooth::waitForConnection() {
    Serial.println("[BLU] Esperando conexao...");
	while (!Bluetooth::isConnected()) {}
    Serial.println("[BLU] Conectado");
}

String Bluetooth::readData() {
    String data = "";
    if (BTSerial.available()) {
        data = BTSerial.readStringUntil('\0');
        BTSerial.flush();
        return data;
    }
    return data;
}

void Bluetooth::sendData(const String &data) {
    if (BTSerial.availableForWrite()) {
        BTSerial.println(data);
        BTSerial.flush(); 
    }
}
