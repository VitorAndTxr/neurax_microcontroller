#include "MessageHandler.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/session/Session.h"

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
                int code = jsonDoc["cd"];
                String method = jsonDoc["mt"];
                
                // aqui não vai ter case 'w'
                switch (code) {
                    case 1:
                        switch (method[0]) {
                            // gyroscope data request from app
                            case 'r':
                                // send gyroscope data to app

                                break;
                            // gyroscope measurement request from app
                            case 'x':
                                // acquire gyroscope data and send to app
                                break;
                            // acknowledgement gyroscope data received from app
                            case 'a':

                                break;
                            default:
                                break;
                        }
                    break;
                    case 2:
                        switch (method[0]) {
                            case 'w':
                                float amplitude = jsonDoc["a"];
                                float frequency = jsonDoc["f"];
                                float pulse_width = jsonDoc["pw"];
                                float difficulty = jsonDoc["df"];
                                float pulse_duration = jsonDoc["pd"];
                            
                                break;
                            case 'r':
                                float amplitude = jsonDoc["a"];
                                break;
                            case 'x':
                                //parameters
                                float amplitude = jsonDoc["bd"]["parameters"]["a"];
                                float frequency = jsonDoc["bd"]["parameters"]["f"];
                                float pulse_width = jsonDoc["bd"]["parameters"]["pw"];
                                float difficulty = jsonDoc["bd"]["parameters"]["df"];
                                float pulse_duration = jsonDoc["bd"]["parameters"]["pd"];
                                //status
                                int complete_stimuli_amount = jsonDoc["bd"]["status"]["csa"];
                                int interrupted_stimuli_amount = jsonDoc["bd"]["status"]["isa"];
                                int time_last_trigger = jsonDoc["bd"]["status"]["tlt"];
                                int session_duration = jsonDoc["bd"]["status"]["sd"];
                                break;
                            case 'a':
                                // bluetooth handshake
                                break;
                            default:
                                break;
                        }
                    break;
                    case 3:
                        switch (method[0]) {
                            case 'r':
                                break;
                            case 'x':
                                break;
                            case 'a':
                                break;
                            default:
                                break;
                        }
                        break;
                    case 4:
                        switch (method[0]) {
                            case 'r':
                                // app receved gyroscope data
                                break;
                            case 'x':
                                // app receved stimuli params and session status
                                break;
                            case 'a':
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}