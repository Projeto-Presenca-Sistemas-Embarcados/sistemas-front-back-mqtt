# Solução Alternativa: ESP32 via Serial + Script Node.js

Como o ESP32 está tendo problemas com WiFi (reset ao inicializar), esta solução usa:
- **ESP32**: Apenas lê cartões RFID e envia via Serial
- **Script Node.js**: Recebe dados Serial e envia via MQTT

## Vantagens

✅ ESP32 não precisa de WiFi (evita problemas de hardware)  
✅ Mais confiável (Serial é mais estável)  
✅ Fácil de debugar (vê tudo no terminal)  
✅ Funciona mesmo se WiFi do ESP32 falhar  

## Instalação

### 1. Instalar dependências

```bash
cd "/Users/caramurulaptop/Documents/Sistemas Embarcados"
npm install
```

### 2. Upload do código no ESP32

1. Abra `esp32-rfid-serial.ino` no Arduino IDE
2. Faça upload no ESP32
3. Abra Serial Monitor (115200 baud) para verificar se está funcionando

### 3. Executar o script bridge

```bash
node serial-to-mqtt.js
```

O script vai:
- Encontrar automaticamente a porta Serial do ESP32
- Conectar ao MQTT broker
- Receber dados do ESP32 e enviar via MQTT

## Como funciona

1. **ESP32 detecta cartão RFID**
   - Lê o UID do cartão
   - Envia via Serial: `TAG:CA:93:C1:01|ROOM:Sala 101|ESP32:esp32-rfid-001`

2. **Script Node.js recebe**
   - Lê a linha Serial
   - Parseia os dados
   - Cria payload JSON

3. **Script envia via MQTT**
   - Publica no tópico: `presenca/attendance/Sala 101/esp32-rfid-001/tag-read`
   - Backend recebe normalmente

## Teste

1. Execute o script: `node serial-to-mqtt.js`
2. Aproxime um cartão no ESP32
3. Você deve ver no terminal:
   ```
   📨 ESP32: ----- CARTÃO DETECTADO -----
   📨 ESP32: UID: CA:93:C1:01
   📨 ESP32: TAG:CA:93:C1:01|ROOM:Sala 101|ESP32:esp32-rfid-001
   ✅ Tag enviada: CA:93:C1:01 -> presenca/attendance/...
   ```

## Configuração

Edite `serial-to-mqtt.js` para alterar:
- `MQTT_BROKER`: IP do broker MQTT
- `ROOM_NAME`: Nome da sala
- `ESP32_ID`: ID do ESP32

## Troubleshooting

**Porta Serial não encontrada:**
- Verifique se o ESP32 está conectado
- Verifique se nenhum outro programa está usando a porta (Arduino IDE Serial Monitor fechado)

**MQTT não conecta:**
- Verifique se o broker MQTT está rodando
- Verifique o IP do broker no código

**Dados não aparecem:**
- Verifique se o Serial Monitor do Arduino IDE está fechado
- Verifique se a velocidade está em 115200 baud



