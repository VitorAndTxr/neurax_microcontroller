# Plano de Implementação: Streaming sEMG via Bluetooth

## 1. Análise da Arquitetura Atual

### Sistema sEMG Existente
- **Taxa de amostragem**: ~860 Hz (período de 1.162 ms)
- **Processamento atual**:
  - Buffer de 50 amostras (`SEMG_SAMPLES_PER_VALUE`)
  - Filtragem Butterworth passa-banda (10-40 Hz)
  - Cálculo de média RMS
  - Detecção de trigger baseada em threshold

### Limitações Identificadas
1. **Bluetooth baudrate**: 9600 bps (limitação crítica de banda)
2. **JSON overhead**: Formato atual consome muitos bytes
3. **Processamento síncrono**: `acquireAverage()` bloqueia até completar leitura
4. **Sem buffer circular**: Amostras não são armazenadas para transmissão contínua

---

## 2. Requisitos do Streaming

### Casos de Uso
1. **Visualização em tempo real**: App exibe gráfico de sinal sEMG
2. **Análise offline**: App armazena dados para processamento posterior
3. **Calibração**: Usuário ajusta threshold observando sinal
4. **Debug**: Desenvolvedor verifica qualidade do sinal

### Parâmetros Configuráveis
- **Taxa de envio**: Amostras por segundo (ex: 50 Hz, 100 Hz, 200 Hz)
- **Tipo de dados**: Raw (bruto), Filtered (filtrado), RMS (envelope)
- **Formato**: JSON compacto ou binário
- **Modo**: Contínuo ou burst (N amostras)

---

## 3. Protocolo de Comunicação

### Novos Códigos de Mensagem

Adicionar ao `CommunicationProtocol.h`:

```cpp
namespace SEMG_STREAMING {
    const int START_STREAM = 11;   // Iniciar streaming
    const int STOP_STREAM = 12;    // Parar streaming
    const int STREAM_DATA = 13;    // Dados do stream
    const int CONFIG_STREAM = 14;  // Configurar parâmetros
}
```

### Mensagens JSON

#### 3.1. Configurar Stream (App → ESP32)
```json
{
  "cd": 14,
  "mt": "w",
  "bd": {
    "rate": 50,        // Hz - amostras por segundo
    "type": "filtered", // "raw", "filtered", "rms"
    "format": "compact" // "compact" ou "binary"
  }
}
```

#### 3.2. Iniciar Stream (App → ESP32)
```json
{
  "cd": 11,
  "mt": "x"
}
```

#### 3.3. Parar Stream (App → ESP32)
```json
{
  "cd": 12,
  "mt": "x"
}
```

#### 3.4. Dados Streaming (ESP32 → App)

**Formato Compacto** (recomendado para 9600 baud):
```json
{
  "cd": 13,
  "mt": "w",
  "bd": {
    "t": 1234567,     // timestamp (ms desde boot)
    "v": [1.2, 1.5, 1.3, 1.4]  // array de valores (até 10 amostras)
  }
}
```

**Tamanho estimado**: ~60-80 bytes para 4 amostras

**Formato Binário** (futuro, se necessário):
- Header: 4 bytes (código + timestamp)
- Dados: 2 bytes por amostra (float comprimido como int16)
- Total: 4 + (2 × N) bytes

---

## 4. Cálculos de Desempenho

### Taxa de Transmissão Bluetooth

**Baudrate**: 9600 bps = 1200 bytes/s (teórico)
**Real**: ~960 bytes/s (overhead de protocolo serial)

### Cenários de Streaming

| Taxa (Hz) | Amostras/pacote | Pacotes/s | Bytes/pacote | Banda total | Viável? |
|-----------|----------------|-----------|--------------|-------------|---------|
| **50 Hz**  | 5 | 10 | 70 | 700 B/s | ✅ **SIM** |
| **100 Hz** | 10 | 10 | 90 | 900 B/s | ⚠️ **LIMITE** |
| **200 Hz** | 10 | 20 | 90 | 1800 B/s | ❌ **NÃO** |
| **860 Hz** | 50 | 17.2 | 200+ | 3440+ B/s | ❌ **NÃO** |

### Recomendação
**Taxa ideal: 50 Hz com 5 amostras por pacote**
- Banda utilizada: ~73% (margem de segurança)
- Latência: ~100 ms (aceitável para visualização)
- Suficiente para análise de sEMG (banda útil: 10-40 Hz)

---

## 5. Arquitetura de Implementação

### 5.1. Buffer Circular para Amostras

**Objetivo**: Armazenar amostras contínuas sem bloquear amostragem

```cpp
// Em Semg.h
#define STREAMING_BUFFER_SIZE 100  // Buffer circular

class Semg {
private:
    static float streaming_buffer[STREAMING_BUFFER_SIZE];
    static volatile int buffer_write_index;
    static volatile int buffer_read_index;
    static volatile bool streaming_active;

public:
    static void enableStreaming();
    static void disableStreaming();
    static int getAvailableSamples();
    static void readStreamingSamples(float* output, int count);
};
```

### 5.2. Task de Streaming

**Nova task FreeRTOS** (Core 1, mesma do MessageHandler):

```cpp
// Em Semg.cpp
void Semg::streamingTask(void* parameters) {
    StreamingConfig* config = (StreamingConfig*)parameters;
    TickType_t last_send = xTaskGetTickCount();

    while (streaming_active) {
        // Aguardar intervalo baseado na taxa configurada
        TickType_t interval = pdMS_TO_TICKS(1000 / config->packets_per_second);
        vTaskDelayUntil(&last_send, interval);

        // Verificar amostras disponíveis
        int available = getAvailableSamples();
        if (available >= config->samples_per_packet) {
            float samples[MAX_SAMPLES_PER_PACKET];
            readStreamingSamples(samples, config->samples_per_packet);

            // Construir e enviar mensagem
            sendStreamingMessage(samples, config->samples_per_packet);
        }
    }

    vTaskDelete(NULL);
}
```

### 5.3. Modificações no Timer de Amostragem

**Callback atual** (`samplingCallback`):
```cpp
void Semg::samplingCallback(TimerHandle_t xTimer) {
    vTaskResume(Semg::task_handle);  // Atual
}
```

**Callback modificado**:
```cpp
void Semg::samplingCallback(TimerHandle_t xTimer) {
    // Modo normal (Session ativa)
    if (Session::status.ongoing) {
        vTaskResume(Semg::task_handle);
    }

    // Modo streaming (somente sEMG)
    if (streaming_active) {
        // Adicionar amostra ao buffer circular
        streaming_buffer[buffer_write_index] = Adc::getValue(SEMG_ADC_PIN);
        buffer_write_index = (buffer_write_index + 1) % STREAMING_BUFFER_SIZE;
    }
}
```

### 5.4. Integração com MessageHandler

**Adicionar casos** em `MessageHandler::interpretMessage()`:

```cpp
case SEMG_STREAMING::CONFIG_STREAM:
    handleStreamingConfigMessage(message);
    break;

case SEMG_STREAMING::START_STREAM:
    Semg::enableStreaming();
    break;

case SEMG_STREAMING::STOP_STREAM:
    Semg::disableStreaming();
    break;
```

**Nova função**:
```cpp
void MessageHandler::handleStreamingConfigMessage(DynamicJsonDocument &message) {
    JsonObject config = message[MESSAGE_KEYS::BODY].as<JsonObject>();

    int rate = config["rate"];
    String type = config["type"];
    String format = config["format"];

    Semg::configureStreaming(rate, type, format);
}
```

---

## 6. Plano de Implementação Detalhado

### Fase 1: Estrutura Básica (2-3 horas)
**Objetivo**: Criar infraestrutura de streaming sem envio de dados

**Arquivos a modificar**:
- `src/modules/message_handler/CommunicationProtocol.h`
- `src/modules/semg/Semg.h`
- `src/modules/semg/Semg.cpp`

**Tarefas**:
1. ✅ Adicionar constantes de protocolo (`SEMG_STREAMING::*`)
2. ✅ Adicionar variáveis de estado em `Semg`:
   - `streaming_buffer[]`
   - `buffer_write_index`, `buffer_read_index`
   - `streaming_active`
   - `streaming_config` struct
3. ✅ Implementar funções básicas:
   - `enableStreaming()` / `disableStreaming()`
   - `getAvailableSamples()`
   - `readStreamingSamples()`

**Teste**: Compilar sem erros

---

### Fase 2: Amostragem para Buffer (1-2 horas)
**Objetivo**: Modificar timer callback para preencher buffer circular

**Tarefas**:
1. ✅ Modificar `samplingCallback()` para escrever no buffer
2. ✅ Implementar lógica de buffer circular (wrap-around)
3. ✅ Adicionar proteção contra overflow (verificar espaço disponível)

**Teste**:
```cpp
// Em main.cpp loop() para debug
void loop() {
    Semg::enableStreaming();
    delay(1000);
    int available = Semg::getAvailableSamples();
    Serial.printf("Amostras disponíveis: %d\n", available);
    // Esperado: ~860 amostras/segundo
}
```

---

### Fase 3: Envio de Mensagens (2-3 horas)
**Objetivo**: Implementar task de streaming e envio via Bluetooth

**Tarefas**:
1. ✅ Criar `streamingTask()` com loop temporizado
2. ✅ Implementar `sendStreamingMessage()`:
   - Criar DynamicJsonDocument
   - Adicionar timestamp
   - Adicionar array de valores
   - Chamar `MessageHandler::sendMessage()`
3. ✅ Adicionar controle de taxa (packets per second)

**Teste via Bluetooth**:
```json
// Enviar
{"cd":11,"mt":"x"}

// Receber (repetidamente)
{"cd":13,"mt":"w","bd":{"t":12345,"v":[1.2,1.3,1.4,1.5,1.6]}}
```

---

### Fase 4: Comandos de Controle (1 hora)
**Objetivo**: Permitir start/stop/config via Bluetooth

**Tarefas**:
1. ✅ Implementar `handleStreamingConfigMessage()`
2. ✅ Adicionar casos em `interpretMessage()`:
   - `START_STREAM`: chama `enableStreaming()`
   - `STOP_STREAM`: chama `disableStreaming()`
   - `CONFIG_STREAM`: chama `handleStreamingConfigMessage()`
3. ✅ Implementar `configureStreaming(rate, type, format)`

**Teste**:
```json
// Configurar 50 Hz
{"cd":14,"mt":"w","bd":{"rate":50,"type":"raw","format":"compact"}}

// Iniciar
{"cd":11,"mt":"x"}

// (recebe dados...)

// Parar
{"cd":12,"mt":"x"}
```

---

### Fase 5: Tipos de Dados (1-2 horas)
**Objetivo**: Suportar raw, filtered, RMS

**Tarefas**:
1. ✅ Adicionar enum `StreamingDataType` em Semg.h:
   ```cpp
   enum StreamingDataType { RAW, FILTERED, RMS };
   ```
2. ✅ Modificar `samplingCallback()` para aplicar filtro se necessário
3. ✅ Implementar `applyStreamingFilter()`:
   - RAW: valor direto do ADC
   - FILTERED: aplicar filtro Butterworth
   - RMS: calcular envelope (média móvel de N amostras)

**Teste**: Comparar sinais RAW vs FILTERED no app

---

### Fase 6: Otimizações (1-2 horas)
**Objetivo**: Melhorar desempenho e confiabilidade

**Tarefas**:
1. ✅ Reduzir tamanho JSON:
   - Usar chaves curtas ("t", "v")
   - Limitar precisão float (1 casa decimal)
2. ✅ Adicionar contador de pacotes perdidos
3. ✅ Implementar timeout de streaming (auto-stop após X minutos)
4. ✅ Adicionar verificação de conexão Bluetooth antes de enviar

---

## 7. Configuração Recomendada

### platformio.ini
Adicionar flags de configuração:

```ini
build_flags =
    # ... flags existentes ...

    # Streaming sEMG
    -D STREAMING_BUFFER_SIZE=100
    -D MAX_SAMPLES_PER_PACKET=10
    -D DEFAULT_STREAMING_RATE=50
    -D STREAMING_TIMEOUT_MINUTES=10
```

---

## 8. Estrutura de Código Proposta

### Semg.h (adições)
```cpp
// Configuração de streaming
enum StreamingDataType { RAW, FILTERED, RMS };

struct StreamingConfig {
    int rate;                      // Hz
    StreamingDataType type;
    int samples_per_packet;
    int packets_per_second;
};

class Semg {
private:
    // Buffer circular
    static float streaming_buffer[STREAMING_BUFFER_SIZE];
    static volatile int buffer_write_index;
    static volatile int buffer_read_index;

    // Estado
    static volatile bool streaming_active;
    static StreamingConfig streaming_config;
    static TaskHandle_t streaming_task_handle;

    // Funções internas
    static void writeToBuffer(float value);
    static float applyStreamingFilter(float value);
    static void sendStreamingMessage(float* samples, int count);

public:
    // API pública
    static void configureStreaming(int rate, String type, String format);
    static void enableStreaming();
    static void disableStreaming();
    static bool isStreaming();

    // Streaming task
    static void streamingTask(void* parameters);

    // Buffer management
    static int getAvailableSamples();
    static void readStreamingSamples(float* output, int count);
};
```

---

## 9. Exemplo de Uso (App)

### Python (exemplo para teste via terminal Bluetooth)
```python
import serial
import json
import time

# Conectar ao Bluetooth
bt = serial.Serial('COM5', 9600)  # Ajustar porta

# Configurar streaming: 50 Hz, raw
config = {"cd": 14, "mt": "w", "bd": {"rate": 50, "type": "raw", "format": "compact"}}
bt.write((json.dumps(config) + '\0').encode())

# Iniciar streaming
start = {"cd": 11, "mt": "x"}
bt.write((json.dumps(start) + '\0').encode())

# Receber e plotar dados
samples = []
try:
    while True:
        line = bt.readline().decode('utf-8')
        data = json.loads(line)

        if data['cd'] == 13:  # STREAM_DATA
            timestamp = data['bd']['t']
            values = data['bd']['v']
            samples.extend(values)
            print(f"[{timestamp}] Recebidas {len(values)} amostras")

except KeyboardInterrupt:
    # Parar streaming
    stop = {"cd": 12, "mt": "x"}
    bt.write((json.dumps(stop) + '\0').encode())
    bt.close()
```

---

## 10. Testes de Validação

### Teste 1: Taxa de Amostragem
**Objetivo**: Verificar se taxa configurada é respeitada

**Procedimento**:
1. Configurar 50 Hz
2. Iniciar streaming
3. Registrar timestamps de 100 pacotes
4. Calcular média de intervalo

**Critério de sucesso**: Intervalo médio = 20 ms ± 5 ms

---

### Teste 2: Integridade de Dados
**Objetivo**: Verificar se amostras não são perdidas

**Procedimento**:
1. Gerar sinal senoidal no ADC (gerador de função)
2. Streaming de 1000 amostras
3. Verificar continuidade do sinal

**Critério de sucesso**: Sem gaps ou descontinuidades

---

### Teste 3: Desempenho Bluetooth
**Objetivo**: Verificar estabilidade em transmissão prolongada

**Procedimento**:
1. Streaming contínuo por 5 minutos
2. Contar pacotes recebidos
3. Verificar desconexões

**Critério de sucesso**:
- 0 desconexões
- Taxa de perda < 1%

---

### Teste 4: Filtros
**Objetivo**: Validar diferença entre RAW e FILTERED

**Procedimento**:
1. Aplicar sinal com ruído de 60 Hz
2. Comparar RAW (deve ter ruído) vs FILTERED (deve atenuar)

**Critério de sucesso**: Atenuação de 60 Hz > 20 dB

---

## 11. Considerações de Performance

### Memória RAM
**Uso adicional estimado**:
- Buffer circular: 100 × 4 bytes = 400 bytes
- Config struct: ~20 bytes
- Task stack: 2048 bytes (configurável)
- **Total**: ~2.5 KB

**Memória disponível ESP32**: ~520 KB
**Impacto**: < 0.5% ✅

### CPU
**Overhead de streaming**:
- Callback timer: +10 µs (escrita no buffer)
- Task streaming: executa a 10-50 Hz (baixo impacto)
- Serialização JSON: ~2 ms por pacote

**Impacto total**: < 5% CPU ✅

### Conflitos com Session
**Solução**: Streaming e Session são **mutuamente exclusivos**
- Session ativa → Streaming desabilitado automaticamente
- Streaming ativo → Session não pode iniciar

---

## 12. Melhorias Futuras (Opcional)

### Fase 7: Formato Binário (se 9600 baud for limitante)
- Implementar serialização binária
- Reduzir payload para ~12 bytes (vs 70 bytes JSON)
- Permitir 200 Hz de streaming

### Fase 8: Compressão Delta
- Enviar apenas diferença entre amostras consecutivas
- Reduzir banda em ~50% para sinais lentos

### Fase 9: Upgrade para Bluetooth LE
- Migrar para ESP32 BLE
- Baudrate efetivo: ~200 kbps
- Permitir streaming full-rate (860 Hz)

---

## 13. Checklist de Implementação

### Preparação
- [ ] Backup do código atual (git commit)
- [ ] Criar branch `feature/semg-streaming`
- [ ] Configurar ambiente de teste (osciloscópio/gerador)

### Desenvolvimento
- [ ] Fase 1: Estrutura básica
- [ ] Fase 2: Amostragem para buffer
- [ ] Fase 3: Envio de mensagens
- [ ] Fase 4: Comandos de controle
- [ ] Fase 5: Tipos de dados
- [ ] Fase 6: Otimizações

### Testes
- [ ] Teste 1: Taxa de amostragem
- [ ] Teste 2: Integridade de dados
- [ ] Teste 3: Desempenho Bluetooth
- [ ] Teste 4: Filtros

### Documentação
- [ ] Atualizar DOCUMENTACAO.md
- [ ] Atualizar CLAUDE.md
- [ ] Adicionar exemplos no README.md
- [ ] Atualizar message_handler/README.md

### Deploy
- [ ] Merge para main
- [ ] Tag de versão (v2.0.0-streaming)
- [ ] Teste em hardware final

---

## 14. Resumo Executivo

### O que será implementado?
Sistema de **streaming contínuo de sEMG** via Bluetooth para visualização em tempo real no app.

### Principais características:
- ✅ Taxa configurável (recomendado: 50 Hz)
- ✅ Múltiplos tipos de dados (raw, filtered, RMS)
- ✅ Comandos start/stop via Bluetooth
- ✅ Buffer circular para estabilidade
- ✅ Baixo overhead (< 5% CPU, < 0.5% RAM)

### Tempo estimado:
**8-12 horas** de desenvolvimento + testes

### Compatibilidade:
- ✅ Funciona com hardware existente (HC-05/HC-06)
- ✅ Não interfere com módulo FES
- ⚠️ Mutuamente exclusivo com Session (uso dedicado)

### Próximos passos:
1. Revisar plano com equipe
2. Iniciar Fase 1 (estrutura básica)
3. Testar incrementalmente a cada fase
