#include "Bluetooth.h"

bool Bluetooth::connected = false;
HardwareSerial BTSerial(2); // Use UART2 (TX2, RX2) for Bluetooth

void Bluetooth::init() {
    BTSerial.begin(9600);  // Bluetooth module baud rate
    BTSerial.write(BLUETOOTH_AT_SET_MODULE_NAME);
    BTSerial.write(BLUETOOTH_AT_SET_BAUDRATE_115200);
    BTSerial.end(true);
    BTSerial.begin(115200);  
    Bluetooth::waitForConnection();
}

String Bluetooth::readData() {
    String data = "";
    if (BTSerial.available()) {
        data = BTSerial.readStringUntil('\0');
        return data;
    }
    return data;
}

bool Bluetooth::waitForConnection() {
    DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);

    while (!Bluetooth::connected){
        String handshake = readData();

        DeserializationError error = deserializeJson(jsonDoc, handshake);

        if (error) {
            Serial.print("Erro no parsing do JSON: ");
            Serial.println(error.c_str());
        } 
        else {
            int code = jsonDoc["code"];
            if (code == BLUETOOTH_JSON_CONNECTED_CODE) {
                Bluetooth::connected = true;
            }
        }
    }
    
    return true;;
}

bool Bluetooth::isConnected() {
    return Bluetooth::connected;
}

void Bluetooth::sendData(const String &data) {
    BTSerial.print(data); 
}
