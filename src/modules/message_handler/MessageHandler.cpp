#include "MessageHandler.h"
#include "modules/bluetooth/Bluetooth.h"

DynamicJsonDocument jsonDoc(JSON_BUFFER_SIZE);

void MessageHandler::init() {
    while (true) {
        String data = Bluetooth::readData();
        if (!data.isEmpty()) {
            // Analisar o JSON
            DeserializationError error = deserializeJson(jsonDoc, data);
            // Verificar se houve erros no parsing do JSON
            if (error) {
                Serial.print("Erro no parsing do JSON: ");
                Serial.println(error.c_str());
            } else {
                int valor1 = jsonDoc["code"];
                // aqui chamamos as funções de acordo com o codigo
            }
        }
    }
}