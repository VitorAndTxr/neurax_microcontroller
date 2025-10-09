# Documentação do NeuroEstimulator

## Índice
1. [Visão Geral](#visão-geral)
2. [Arquitetura do Sistema](#arquitetura-do-sistema)
3. [Módulos Principais](#módulos-principais)
4. [Comunicação Bluetooth](#comunicação-bluetooth)
5. [Protocolo de Transmissão de Dados](#protocolo-de-transmissão-de-dados)
6. [Fluxos Principais](#fluxos-principais)
7. [Deploy e Teste](#deploy-e-teste)
8. [Configurações](#configurações)

---

## Visão Geral

O **NeuroEstimulator** é um firmware desenvolvido para ESP32 que implementa um dispositivo de neuroestimulação funcional (FES - Functional Electrical Stimulation) integrado com sensores de eletromiografia de superfície (sEMG). O sistema permite:

- **Estimulação elétrica funcional** controlada por parâmetros ajustáveis (amplitude, frequência, largura de pulso)
- **Detecção de sinais sEMG** com filtragem e processamento em tempo real
- **Comunicação via Bluetooth** com aplicativo móvel usando protocolo JSON
- **Monitoramento de bateria** para alimentação principal e de estimulação
- **Leitura de giroscópio** (MPU6050) para análise de posicionamento
- **Sistema de sessões** com detecção e estimulação automática

### Componentes de Hardware Suportados
- **Plataforma**: ESP32 (esp32doit-devkit-v1)
- **ADC externo**: ADS1115 (I2C, endereço 0x48)
- **Giroscópio**: MPU6050 (I2C)
- **Módulo Bluetooth**: HC-05/HC-06 (UART2)
- **Ponte H**: Para controle de estimulação
- **Potenciômetro digital**: Controle de amplitude
- **Sensor sEMG**: AD8232

---

## Arquitetura do Sistema

### Diagrama de Módulos

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32 Main                          │
│                    (main.cpp)                            │
└────────────┬────────────────────────────────────────────┘
             │
             ├─► MessageHandler (RTOS Task - Core 1)
             │   └─► Bluetooth Module (UART2)
             │
             ├─► Session (RTOS Task - Core 1)
             │   ├─► sEMG Module
             │   └─► FES Module
             │
             ├─► Gyroscope (MPU6050)
             ├─► ADC (ADS1115)
             ├─► Battery Monitor
             ├─► Emergency Button
             └─► LED Indicators
```

### Arquitetura de Tarefas FreeRTOS

O firmware utiliza FreeRTOS para gerenciamento de tarefas concorrentes:

| Tarefa | Prioridade | Core | Função |
|--------|-----------|------|--------|
| MessageHandler | 20 | 1 | Processa mensagens Bluetooth |
| Session | 20 | 1 | Gerencia sessões de detecção/estimulação |
| FES Loop | Alta | - | Executa pulsos de estimulação |
| sEMG Sampling | Timer | - | Amostragem de sinais sEMG |

---

## Módulos Principais

### 1. **Bluetooth** (`src/modules/bluetooth/`)

**Responsabilidades:**
- Comunicação serial via UART2 com módulo Bluetooth
- Controle de conexão e status
- Envio e recebimento de dados JSON
- Sincronização com semáforos para acesso seguro

**Principais Funções:**
```cpp
void Bluetooth::init()              // Inicializa módulo Bluetooth
bool Bluetooth::isConnected()       // Verifica status de conexão
String Bluetooth::readData()        // Lê dados do buffer serial
void Bluetooth::sendData(String)    // Envia dados via Bluetooth
void Bluetooth::waitForConnection() // Aguarda conexão com app
```

**Pinos:**
- `BLUETOOTH_MODULE_STATUS_PIN`: GPIO23 (status de conexão)
- TX2/RX2: Comunicação UART2

**Baudrate:** 9600 bps (padrão), 115200 bps (opcional via comandos AT)

### 2. **MessageHandler** (`src/modules/message_handler/`)

**Responsabilidades:**
- Interpretação de mensagens JSON recebidas via Bluetooth
- Roteamento de comandos para módulos apropriados
- Serialização e envio de respostas
- Loop principal executado em tarefa FreeRTOS

**Principais Funções:**
```cpp
void MessageHandler::init()                    // Inicializa e aguarda conexão BT
void MessageHandler::start()                   // Cria task FreeRTOS
void MessageHandler::loop(void*)              // Loop principal de mensagens
void MessageHandler::sendMessage(DynamicJsonDocument*) // Envia mensagem JSON
void MessageHandler::interpretMessage(String)  // Interpreta mensagem recebida
```

**Fluxo de Processamento:**
1. Recebe dados do módulo Bluetooth
2. Deserializa JSON
3. Identifica código e método da mensagem
4. Executa ação correspondente (ver tabela de protocolo)
5. Envia resposta quando necessário

### 3. **Session** (`src/modules/session/`)

**Responsabilidades:**
- Gerenciamento de sessões de neuroestimulação
- Controle de estados (iniciada, pausada, parada)
- Detecção de triggers via sEMG
- Execução de estimulação FES
- Envio de status para aplicativo

**Estados da Sessão:**
```cpp
SessionStatus {
    short complete_stimuli_amount;      // Quantidade de estímulos completos
    short interrupted_stimuli_amount;   // Quantidade de estímulos interrompidos
    bool paused;                        // Sessão pausada
    bool ongoing;                       // Sessão em andamento
    uint32_t time_of_last_trigger;      // Timestamp do último trigger
    uint32_t session_duration;          // Duração da sessão
}
```

**Principais Funções:**
```cpp
void Session::init()                     // Inicializa módulo
void Session::start()                    // Inicia sessão
void Session::stop()                     // Para sessão
void Session::pauseFromSession()         // Pausa da própria sessão
void Session::pauseFromMessageHandler()  // Pausa via comando externo
void Session::resume()                   // Retoma sessão
void Session::detectionAndStimulation()  // Loop de detecção e estimulação
void Session::sendSessionStatus()        // Envia status via Bluetooth
```

### 4. **FES (Functional Electrical Stimulation)** (`src/modules/fes/`)

**Responsabilidades:**
- Geração de pulsos elétricos para estimulação
- Controle de parâmetros (amplitude, frequência, largura de pulso)
- Interface com ponte H
- Controle de potenciômetro digital para amplitude

**Parâmetros de Estimulação:**
```cpp
FesParameters {
    int fes_duration_s;      // Duração total em segundos
    int pulse_width_us;      // Largura do pulso em microsegundos
    float frequency;         // Frequência em Hz
    float amplitude;         // Amplitude em Volts
}
```

**Principais Funções:**
```cpp
void Fes::init()                        // Inicializa GPIO e potenciômetro
void Fes::begin()                       // Inicia estimulação
void Fes::stopFes()                     // Para estimulação
void Fes::setParameters(...)            // Define parâmetros de estimulação
void Fes::fesLoop()                     // Loop de geração de pulsos
```

**Pinos:**
- `H_BRIDGE_INPUT_1`: GPIO32
- `H_BRIDGE_INPUT_2`: GPIO33

### 5. **sEMG (Surface Electromyography)** (`src/modules/semg/`)

**Responsabilidades:**
- Leitura de sinais eletromiográficos
- Filtragem passa-banda (10-40 Hz)
- Detecção de triggers baseada em threshold
- Amostragem por timer (1.16 ms)
- Controle de dificuldade

**Parâmetros sEMG:**
```cpp
SemgParameters {
    float gain;         // Ganho do sensor
    float difficulty;   // Dificuldade (1-100%)
    float threshold;    // Threshold para trigger
}
```

**Principais Funções:**
```cpp
void Semg::init()                       // Inicializa ADC e filtros
bool Semg::isTrigger()                  // Verifica se houve trigger
float Semg::acquireAverage(int)         // Adquire média de leituras
void Semg::setDifficulty(int)           // Define dificuldade
void Semg::testTrigger(int)             // Teste de detecção de trigger
void Semg::startSamplingTimer()         // Inicia amostragem
void Semg::samplingCallback(TimerHandle_t) // Callback de amostragem
```

**Filtragem:**
- Filtro passa-banda Butterworth de 2ª ordem
- Frequência de corte inferior: 10 Hz
- Frequência de corte superior: 40 Hz
- Taxa de amostragem: ~860 Hz (1.162 ms/amostra)

### 6. **ADC** (`src/modules/adc/`)

**Responsabilidades:**
- Interface com ADS1115 via I2C
- Leitura de múltiplos canais analógicos
- Conversão de valores para voltagem

**Canais Utilizados:**
- Canal 0: sEMG
- Canal 2: Bateria principal
- Canal 3: Bateria de estimulação

### 7. **Gyroscope** (`src/modules/gyroscope/`)

**Responsabilidades:**
- Leitura do sensor MPU6050
- Cálculo de ângulo de inclinação (pitch)
- Calibração do sensor
- Envio de dados via Bluetooth

**Principais Funções:**
```cpp
void Gyroscope::init()                  // Inicializa MPU6050
float Gyroscope::calculatePitch()       // Calcula ângulo de inclinação
float Gyroscope::gyroscopeRoutine()     // Rotina completa de leitura
void Gyroscope::sendLastValue()         // Envia último valor lido
```

### 8. **Battery Monitor** (`src/modules/battery_monitor/`)

**Responsabilidades:**
- Monitoramento de tensão das baterias
- Alertas de bateria baixa
- Leitura via ADC externo

**Thresholds:**
- Bateria principal: 3.0V (após atenuação)
- Bateria de estimulação: 3.0V (após atenuação)

### 9. **Potentiometer** (`src/modules/potentiometer/`)

**Responsabilidades:**
- Controle de potenciômetro digital
- Ajuste de amplitude de estimulação
- Correção de voltagem para linearização

**Principais Funções:**
```cpp
void Potentiometer::init()              // Inicializa pinos SPI
void Potentiometer::increase(int)       // Incrementa resistência
void Potentiometer::decrease(int)       // Decrementa resistência
void Potentiometer::voltageSet(float)   // Define voltagem alvo
float Potentiometer::getCorrectedVoltage() // Lê voltagem corrigida
```

**Pinos:**
- `POTENTIOMETER_PIN_INCREMENT`: GPIO14
- `POTENTIOMETER_PIN_UP_DOWN`: GPIO12
- `POTENTIOMETER_PIN_CS`: GPIO13

### 10. **LED** (`src/modules/led/`)

**Responsabilidades:**
- Controle de LEDs indicadores
- Feedback visual de estados

**LEDs Disponíveis:**
- `LED_PIN_TRIGGER`: GPIO26 (indica trigger sEMG)
- `LED_PIN_FES`: GPIO25 (indica estimulação)
- `LED_PIN_POWER`: GPIO27 (indica alimentação)

---

## Comunicação Bluetooth

### Conectores Bluetooth Compatíveis

O firmware foi desenvolvido para trabalhar com módulos Bluetooth clássicos (SPP - Serial Port Profile):

#### **HC-05 / HC-06**
- **Protocolo**: Bluetooth 2.0 + EDR
- **Baudrate**: 9600 bps (padrão), configurável até 115200 bps
- **Modo**: Slave (aceita conexões)
- **Comandos AT**: Suportados para configuração
- **Pinos**: TX, RX, VCC, GND, STATE

**Configuração Inicial:**
```cpp
// Definido em platformio.ini
BLUETOOTH_AT_SET_MODULE_NAME="AT+NAME=NeuroEstimulator"
BLUETOOTH_AT_SET_BAUDRATE_115200="AT+UART=115200,0,0"
```

#### **Outros Módulos Compatíveis**
- HC-09
- JDY-31 SPP-C
- Qualquer módulo Bluetooth SPP compatível com UART

### Conexão no Aplicativo Móvel

1. **Pareamento**: Parear dispositivo "NeuroEstimulator" nas configurações Bluetooth
2. **Conexão**: App abre socket SPP na UUID padrão
3. **Handshake**: Opcional, código 0 (NE_HANDSHAKE)
4. **Comunicação**: Troca de mensagens JSON via serial

### Indicadores de Conexão

- **LED Power piscando**: Aguardando conexão
- **LED Power aceso**: Conectado
- **Status Pin (GPIO23)**: HIGH quando conectado

---

## Protocolo de Transmissão de Dados

### Estrutura de Mensagens JSON

Todas as mensagens seguem o formato:

```json
{
  "cd": <código>,
  "mt": "<método>",
  "bd": { /* corpo opcional */ }
}
```

**Campos:**
- `cd` (code): Código numérico que identifica o tipo de mensagem
- `mt` (method): Método da mensagem (`r`=read, `w`=write, `x`=execute)
- `bd` (body): Corpo da mensagem com dados adicionais (opcional)

### Métodos Disponíveis

| Método | Símbolo | Descrição |
|--------|---------|-----------|
| Read | `r` | Solicita leitura de dados |
| Write | `w` | Envia dados para escrita |
| Execute | `x` | Executa comando/ação |
| Acknowledge | `a` | Confirmação de recebimento |

### Tabela de Códigos de Mensagem

| Código | Descrição | Métodos | Origem | Destino | Corpo | Notas |
|--------|-----------|---------|--------|---------|-------|-------|
| **1** | **Leitura de Giroscópio** | `r`, `x`, `w` | App↔ESP32 | App↔ESP32 | `r`/`x`: sem corpo<br>`w`: `{"a": <ângulo>}` | `r`: lê último valor<br>`x`: executa leitura e envia<br>`w`: ESP32 envia valor |
| **2** | **Iniciar Sessão** | `x` | App | ESP32 | Sem corpo | Inicia detecção e estimulação |
| **3** | **Parar Sessão** | `x` | App | ESP32 | Sem corpo | Finaliza sessão |
| **4** | **Pausar Sessão** | `x`, `w` | App↔ESP32 | App↔ESP32 | Sem corpo | `x`: App pausa<br>`w`: ESP32 notifica pausa |
| **5** | **Retomar Sessão** | `x` | App | ESP32 | Sem corpo | Retoma sessão pausada |
| **6** | **Estímulo Único** | `x` | App | ESP32 | Sem corpo | Gera um pulso de estimulação |
| **7** | **Parâmetros FES** | `w` | App | ESP32 | `{"a": <amplitude>,`<br>`"f": <frequência>,`<br>`"pw": <largura_pulso>,`<br>`"df": <dificuldade>,`<br>`"pd": <duração>}` | Define parâmetros de estimulação e dificuldade sEMG |
| **8** | **Status da Sessão** | `w` | ESP32 | App | `{"parameters": {...},`<br>`"status": {`<br>`"csa": <completos>,`<br>`"isa": <interrompidos>,`<br>`"tlt": <tempo_trigger>,`<br>`"sd": <duração_sessão>`<br>`}}` | Envia status periódico |
| **9** | **Trigger sEMG** | `w`, `x` | App↔ESP32 | App↔ESP32 | Sem corpo | `w`: ESP32 detectou trigger<br>`x`: App solicita teste de trigger |

### Exemplos de Mensagens

#### 1. App Solicita Leitura de Giroscópio
```json
{
  "cd": 1,
  "mt": "r"
}
```

#### 2. ESP32 Responde com Ângulo
```json
{
  "cd": 1,
  "mt": "w",
  "bd": {
    "a": 45.7
  }
}
```

#### 3. App Define Parâmetros de Estimulação
```json
{
  "cd": 7,
  "mt": "w",
  "bd": {
    "a": 3.0,
    "f": 38.0,
    "pw": 12.0,
    "df": 5,
    "pd": 5
  }
}
```

**Significado:**
- `a`: Amplitude = 3.0V
- `f`: Frequência = 38 Hz
- `pw`: Largura de pulso = 12 ms
- `df`: Dificuldade sEMG = 5%
- `pd`: Duração do pulso = 5 segundos

#### 4. App Inicia Sessão
```json
{
  "cd": 2,
  "mt": "x"
}
```

#### 5. ESP32 Notifica Trigger Detectado
```json
{
  "cd": 9,
  "mt": "w"
}
```

#### 6. ESP32 Envia Status da Sessão
```json
{
  "cd": 8,
  "mt": "w",
  "bd": {
    "parameters": {
      "a": 3.0,
      "f": 38.0,
      "pw": 12.0,
      "df": 5,
      "pd": 5
    },
    "status": {
      "csa": 10,
      "isa": 2,
      "tlt": 45000,
      "sd": 120000
    }
  }
}
```

---

## Fluxos Principais

### Fluxo 1: Inicialização do Sistema

```
1. setup() - main.cpp
   ├─► Serial.begin(115200)
   ├─► Desabilita Watchdog Timers
   ├─► LED_POWER.set(true)
   ├─► Gyroscope::init()
   ├─► Adc::init()
   ├─► Fes::init()
   ├─► Semg::init()
   ├─► Potentiometer::init()
   ├─► Session::init()
   ├─► MessageHandler::init()
   │   ├─► Bluetooth::init()
   │   └─► Bluetooth::waitForConnection()
   └─► MessageHandler::start()
       └─► Cria Task FreeRTOS no Core 1
```

### Fluxo 2: Processamento de Mensagens

```
MessageHandler::loop() [Task FreeRTOS]
   │
   ├─► Bluetooth::isConnected()?
   │   ├─► NÃO: Bluetooth::waitForConnection()
   │   └─► SIM: Continua
   │
   ├─► Bluetooth::readData()
   │   └─► Retorna String JSON ou vazio
   │
   ├─► Se dados recebidos:
   │   ├─► deserializeJson()
   │   ├─► getMessageCode()
   │   └─► switch(code):
   │       ├─► 1: handleGyroscopeMessage()
   │       ├─► 2: Session::start()
   │       ├─► 3: Session::stop()
   │       ├─► 4: Session::pauseFromMessageHandler()
   │       ├─► 5: Session::resume()
   │       ├─► 6: Fes::begin()
   │       ├─► 7: handleSessionParametersMessage()
   │       └─► 9: Semg::testTrigger()
   │
   └─► Loop infinito
```

### Fluxo 3: Sessão de Neuroestimulação

```
Session::start()
   ├─► Cria Task FreeRTOS
   └─► Session::loop()
       │
       └─► while(SessionStatus::ongoing)
           │
           ├─► Se pausado: suspende task
           │
           ├─► Semg::isTrigger()?
           │   ├─► NÃO: Continua loop
           │   └─► SIM:
           │       ├─► Semg::sendTriggerMessage()
           │       ├─► Fes::begin()
           │       │   ├─► Cria Timer FreeRTOS
           │       │   ├─► fesLoop()
           │       │   │   └─► Gera pulsos por duração configurada
           │       │   └─► stopFes()
           │       ├─► SessionStatus::complete_stimuli_amount++
           │       ├─► Session::sendSessionStatus()
           │       └─► delayBetweenStimuli()
           │
           └─► Repete
```

### Fluxo 4: Detecção de Trigger sEMG

```
Semg::init()
   └─► Cria Timer de Amostragem (1.162 ms)

Timer Callback (860 Hz)
   ├─► Adc::getValue(SEMG_ADC_PIN)
   ├─► Armazena em raw_value[]
   └─► Incrementa sample_amount

Semg::isTrigger()
   ├─► Se sample_amount < SEMG_SAMPLES_PER_VALUE: return false
   ├─► filterSamplesArray()
   │   └─► Aplica filtro Butterworth a cada amostra
   ├─► Calcula média das amostras filtradas
   ├─► Compara com threshold
   ├─► Se > threshold:
   │   ├─► startLedTrigger()
   │   └─► return true
   └─► return false
```

### Fluxo 5: Geração de Pulsos FES

```
Fes::begin()
   ├─► Verifica se não está estimulando
   ├─► hBridgeReset()
   ├─► xTaskCreate(fesLoopTaskWrapper)
   └─► Task FES:
       │
       ├─► stimulating = true
       ├─► Calcula período e duty cycle
       │   ├─► T = 1 / frequency
       │   ├─► t_high = pulse_width_us
       │   └─► t_low = T - t_high
       │
       ├─► Cria Timer para stopFes() após fes_duration_s
       │
       ├─► while(stimulating):
       │   ├─► digitalWrite(H_BRIDGE_INPUT_1, HIGH)
       │   ├─► delayMicroseconds(t_high)
       │   ├─► digitalWrite(H_BRIDGE_INPUT_1, LOW)
       │   ├─► delayMicroseconds(t_low)
       │   └─► Se emergency_stop: break
       │
       └─► hBridgeReset()
```

---

## Deploy e Teste

### Requisitos

#### Hardware
- Placa ESP32 (esp32doit-devkit-v1)
- Cabo USB para programação
- Módulo Bluetooth HC-05 ou HC-06
- Sensor MPU6050 (giroscópio)
- ADC ADS1115
- Sensor sEMG AD8232
- Ponte H para FES
- Potenciômetro digital
- Fonte de alimentação adequada

#### Software
- [PlatformIO](https://platformio.org/) instalado
- VS Code com extensão PlatformIO (recomendado)
- Driver USB-to-Serial (CH340/CP2102) instalado

### Passo 1: Clonar o Repositório

```bash
git clone <url-do-repositorio>
cd InteroperableResearchsEMGDevice
```

### Passo 2: Abrir no PlatformIO

1. Abrir VS Code
2. Extensão PlatformIO → Open Project
3. Selecionar pasta do projeto

### Passo 3: Configurar Parâmetros

Editar `platformio.ini` conforme necessário:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
monitor_speed = 115200

build_flags =
    -D DEBUG=true              # Ativar logs detalhados
    -D FES_MODULE_ENABLE=true  # Habilitar módulo FES
    -D ADC_MODULE_ENABLE=true  # Habilitar ADC
    # ... outros parâmetros
```

### Passo 4: Build do Projeto

#### Via PlatformIO IDE:
- Clicar no ícone de "check" (Build) na barra inferior

#### Via Terminal:
```bash
pio run
```

### Passo 5: Upload para ESP32

#### Via PlatformIO IDE:
- Conectar ESP32 via USB
- Clicar na seta "→" (Upload) na barra inferior

#### Via Terminal:
```bash
pio run --target upload
```

**Observação:** Se houver erro de porta serial:
```bash
# Listar portas disponíveis
pio device list

# Upload especificando porta (Windows)
pio run --target upload --upload-port COM3

# Upload especificando porta (Linux/Mac)
pio run --target upload --upload-port /dev/ttyUSB0
```

### Passo 6: Monitor Serial

#### Via PlatformIO IDE:
- Clicar no ícone de "plug" (Serial Monitor) na barra inferior

#### Via Terminal:
```bash
pio device monitor
```

**Saída Esperada:**
```
[MAIN] Iniciando firmware NeuroEstimulator...
[GYRO] Initializing MPU6050...
[ADC] Initializing ADS1115...
[FES] Initializing FES module...
[sEMG] Initializing sEMG module...
[MSG] Configuring Bluetooth Module...
[MSG] Waiting for connection...
```

### Passo 7: Teste de Conexão Bluetooth

#### No Smartphone/Tablet:
1. Ativar Bluetooth
2. Buscar dispositivos
3. Parear com "NeuroEstimulator" (senha padrão: 1234 ou 0000)
4. Abrir aplicativo compatível
5. Conectar ao dispositivo

#### Logs Esperados:
```
[BLU] Connected to app!
[MSG] Starting Message Handler loop
```

### Passo 8: Teste do Protocolo de Comunicação

#### Usando Terminal Serial Bluetooth (App móvel):
Enviar mensagens JSON via app de terminal Bluetooth:

**Teste 1: Leitura de Giroscópio**
```json
{"cd":1,"mt":"x"}
```

**Resposta Esperada:**
```json
{"cd":1,"mt":"w","bd":{"a":0.5}}
```

**Teste 2: Definir Parâmetros**
```json
{"cd":7,"mt":"w","bd":{"a":3.0,"f":38.0,"pw":12.0,"df":5,"pd":5}}
```

**Logs Esperados:**
```
[MSG] SESSION_COMMANDS::PARAMETERS
[MSG] Parsing received session parameters
[MSG] Amplitude: 3.000000
[MSG] Frequency: 38.000000
[MSG] Pulse width (ms): 12.000000
[MSG] FES stimulation duration (ms): 5.000000
[MSG] SEMG difficulty: 5.000000
```

**Teste 3: Iniciar Sessão**
```json
{"cd":2,"mt":"x"}
```

**Logs Esperados:**
```
[MSG] SESSION_COMMANDS::START
[SESSION] Creating Session task...
[SESSION] Success creating Session task.
[SESSION] Starting Session loop
```

**Teste 4: Parar Sessão**
```json
{"cd":3,"mt":"x"}
```

### Passo 9: Teste de Módulos Individuais

#### Função de Teste no `main.cpp`:

Descomentar funções de teste no `loop()`:

```cpp
void loop() {
    // Descomentar para testar:

    // testRotinaGiroscopio();    // Testa giroscópio
    // testeMedidasSEMG();         // Testa leituras sEMG
    // testeControleTensao();      // Testa potenciômetro
    // testTrigger();              // Testa detecção de trigger
}
```

**Recompilar e fazer upload após editar.**

#### Teste do sEMG:
```cpp
void testeMedidasSEMG() {
    Serial.println(Semg::acquireAverage());
    if (Semg::isTrigger()) {
        Serial.println("TRIGGER DETECTADO!");
    }
    delay(300);
}
```

**Logs Esperados:**
```
0.123
0.145
0.167
TRIGGER DETECTADO!
0.089
```

#### Teste do Giroscópio:
```cpp
void testRotinaGiroscopio() {
    Serial.println("Começando rotina");
    Serial.println(Gyroscope::gyroscopeRoutine());
    Serial.println("Fim da rotina");
    delay(500);
}
```

**Logs Esperados:**
```
Começando rotina
15.7
Fim da rotina
```

### Passo 10: Teste de Estimulação (CUIDADO!)

**⚠️ ATENÇÃO:** Testes de estimulação elétrica devem ser realizados com extremo cuidado:
- Use cargas resistivas (resistores) ao invés de conexão humana inicialmente
- Verifique polaridade da ponte H
- Comece com parâmetros mínimos
- Tenha botão de emergência configurado

#### Teste com Carga Resistiva:
1. Conectar resistor de 1kΩ na saída da ponte H
2. Conectar osciloscópio ou multímetro
3. Enviar comando de estímulo único:

```json
{"cd":7,"mt":"w","bd":{"a":2.0,"f":20.0,"pw":10.0,"df":5,"pd":3}}
{"cd":6,"mt":"x"}
```

4. Verificar forma de onda no osciloscópio:
   - Frequência: ~20 Hz
   - Largura de pulso: ~10 ms
   - Amplitude: ~2V

### Troubleshooting

#### Problema: ESP32 não conecta
**Solução:**
- Verificar porta serial com `pio device list`
- Segurar botão BOOT durante upload
- Instalar drivers CH340/CP2102

#### Problema: Bluetooth não pareia
**Solução:**
- Verificar conexão física TX/RX (cruzada!)
- Verificar alimentação do módulo (3.3V ou 5V conforme módulo)
- Testar comandos AT diretamente na serial
- Verificar STATUS_PIN está conectado

#### Problema: sEMG não detecta trigger
**Solução:**
- Verificar conexão do AD8232
- Ajustar dificuldade para valor baixo (ex: 5%)
- Verificar eletrodos bem fixados
- Monitorar valores brutos com `testeMedidasSEMG()`
- Verificar se SEMG_ENABLE_PIN está funcionando

#### Problema: FES não gera pulsos
**Solução:**
- Verificar FES_MODULE_ENABLE=true no platformio.ini
- Verificar conexões da ponte H
- Verificar potenciômetro digital está configurado
- Testar com amplitude baixa inicialmente
- Verificar logs de erro no serial monitor

#### Problema: Mensagens JSON não são reconhecidas
**Solução:**
- Verificar formato JSON válido
- Adicionar `\0` ao final da string
- Verificar baudrate (9600 ou 115200)
- Aumentar JSON_BUFFER_SIZE se mensagens grandes

---

## Configurações

Todas as configurações são definidas via `build_flags` no arquivo `platformio.ini`:

### Configurações Gerais

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `SDA_PIN` | GPIO21 | Pino I2C SDA |
| `SCL_PIN` | GPIO22 | Pino I2C SCL |
| `ADC_I2C_ADDR` | 0x48 | Endereço I2C do ADC |
| `DEBUG` | true | Ativa logs detalhados |
| `LOG_LOCAL_LEVEL` | ESP_LOG_DEBUG | Nível de log |

### Configurações de Bateria

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `STIMULI_BATTERY_INPUT_PIN` | 3 | Canal ADC para bateria de estimulação |
| `MAIN_BATTERY_INPUT_PIN` | 2 | Canal ADC para bateria principal |
| `STIMULI_BATTERY_THRESHOLD` | 3.0 | Threshold de bateria baixa (V) |
| `MAIN_BATTERY_THRESHOLD` | 3.0 | Threshold de bateria baixa (V) |

### Configurações de LEDs

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `LED_PIN_TRIGGER` | GPIO26 | LED indicador de trigger |
| `LED_PIN_FES` | GPIO25 | LED indicador de FES |
| `LED_PIN_POWER` | GPIO27 | LED indicador de alimentação |

### Configurações FES

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `FES_MODULE_ENABLE` | true | Habilita módulo FES |
| `H_BRIDGE_INPUT_1` | GPIO32 | Entrada 1 da ponte H |
| `H_BRIDGE_INPUT_2` | GPIO33 | Entrada 2 da ponte H |
| `DEFAULT_STIMULI_DURATION` | 0 | Duração padrão (s) |
| `DEFAULT_PULSE_WIDTH` | 0 | Largura de pulso padrão (ms) |
| `DEFAULT_FREQUENCY` | 0 | Frequência padrão (Hz) |
| `DEFAULT_AMPLITUDE` | 7 | Amplitude padrão (V) |

### Configurações sEMG

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `SEMG_ADC_PIN` | 0 | Canal ADC para sEMG |
| `SEMG_ENABLE_PIN` | GPIO18 | Pino de habilitação do AD8232 |
| `SEMG_FILTER_LOW_CUTOFF_FREQUENCY` | 10.0 | Corte inferior do filtro (Hz) |
| `SEMG_FILTER_HIGH_CUTOFF_FREQUENCY` | 40.0 | Corte superior do filtro (Hz) |
| `SEMG_SAMPLING_TIME` | 0.01 | Tempo de amostragem (s) |
| `SEMG_DEFAULT_GAIN` | 10 | Ganho padrão |
| `SEMG_DIFFICULTY_DEFAULT` | 50 | Dificuldade padrão (%) |
| `SEMG_DIFFICULTY_MINIMUM` | 1 | Dificuldade mínima |
| `SEMG_DIFFICULTY_MAXIMUM` | 100 | Dificuldade máxima |
| `SEMG_SAMPLES_PER_VALUE` | 50 | Amostras por leitura |
| `SEMG_SAMPLING_PERIOD` | 1.162790698 | Período de amostragem (ms) |

### Configurações de Potenciômetro

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `POTENTIOMETER_PIN_INCREMENT` | GPIO14 | Pino de incremento |
| `POTENTIOMETER_PIN_UP_DOWN` | GPIO12 | Pino de direção |
| `POTENTIOMETER_PIN_CS` | GPIO13 | Pino Chip Select |
| `DEFAULT_POTENTIOMETER_STEPS` | 1 | Passos por incremento |
| `MAXIMUM_POTENTIOMETER_STEPS` | 100 | Máximo de passos |

### Configurações Bluetooth

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `BLUETOOTH_MODULE_STATUS_PIN` | GPIO23 | Pino de status de conexão |
| `JSON_BUFFER_SIZE` | 512 | Tamanho do buffer JSON |

### Configurações de Sessão

| Flag | Valor Padrão | Descrição |
|------|--------------|-----------|
| `SESSION_TASK_PRIORITY` | 20 | Prioridade da task de sessão |
| `SESSION_DEFAULT_TIME_BETWEEN_STIMULI` | 5 | Tempo entre estímulos (s) |

---

## Referências

### Bibliotecas Utilizadas

- **FreeRTOS**: Sistema operacional em tempo real (incluído no ESP32)
- **ArduinoJson**: Parsing e serialização JSON (v6.21.3)
- **Adafruit ADS1X15**: Driver para ADC ADS1115 (v2.4.0)
- **Adafruit MPU6050**: Driver para giroscópio MPU6050 (v2.2.4)
- **Adafruit BusIO**: Biblioteca de comunicação I2C/SPI (v1.14.5)
- **libFilter**: Biblioteca de filtros digitais (MartinBloedorn)

### Links Úteis

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [Página do Projeto](https://dynamic-vacuum-96a.notion.site/NeuraEstimulator-Blog-5549a27e7c814812b0851a2f0c69d579)

### Suporte

Para dúvidas ou problemas, consulte:
- Issues do repositório
- Documentação do PlatformIO
- Logs detalhados via serial monitor com `DEBUG=true`

---

**Desenvolvido para o projeto NeuroEstimulator - PRISM Lab**

*Versão da Documentação: 1.0*
*Última Atualização: 2025-01-09*
