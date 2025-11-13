# Sistema de Presença com RFID

Sistema completo de controle de presença usando ESP32 com leitor RFID, MQTT, e aplicação web.

## 📋 Pré-requisitos

- Node.js (v18 ou superior)
- npm ou yarn
- Arduino IDE (para ESP32)
- Mosquitto MQTT Broker (ou outro broker MQTT)
- ESP32 com módulo RFID MFRC522

## 🚀 Como Executar

### 1. Bridge Serial → MQTT (serial-to-mqtt.js)

Este script recebe dados do ESP32 via Serial (USB) e envia via MQTT para o backend.

#### Instalação

```bash
cd "/Users/caramurulaptop/Documents/Sistemas Embarcados"
npm install
```

#### Configuração

Edite `serial-to-mqtt.js` se necessário:

```javascript
const MQTT_BROKER = process.env.MQTT_BROKER_URL || 'mqtt://localhost:1883';
const ROOM_NAME = process.env.ROOM_NAME || '101'; // Nome da sala no banco de dados
```

Ou use variáveis de ambiente:

```bash
export MQTT_BROKER_URL="mqtt://localhost:1883"
export ROOM_NAME="101"
```

#### Execução

```bash
node serial-to-mqtt.js
```

**Importante:**
- Certifique-se de que o ESP32 está conectado via USB
- Feche o Serial Monitor do Arduino IDE (se estiver aberto)
- O script detecta automaticamente a porta Serial do ESP32

**O que você verá:**
```
Conectando ao MQTT broker...
✅ MQTT conectado!
✅ ESP32 encontrado: /dev/tty.usbserial-0001
✅ Sistema pronto!
Aguardando identificação do ESP32...
```

---

### 2. Backend (projeto-presenca-backend)

API REST que processa as mensagens MQTT e gerencia o banco de dados.

#### Instalação

```bash
cd projeto-presenca-backend
npm install
```

#### Configuração do Banco de Dados (Prisma)

**Primeira vez ou após mudanças no schema:**

```bash
# Gerar cliente Prisma
npx prisma generate

# Aplicar migrações
npx prisma migrate deploy
```

**Se precisar criar novas migrações:**

```bash
# Depois de alterar prisma/schema.prisma
npx prisma migrate dev --name nome_da_migracao
```

#### Configuração MQTT

Crie um arquivo `.env` no diretório `projeto-presenca-backend/`:

```env
MQTT_BROKER_URL=mqtt://localhost:1883
```

#### Execução

```bash
npm run dev
```

O backend estará disponível em `http://localhost:3001`

**O que você verá:**
```
Server listening at http://127.0.0.1:3001
✅ Cliente MQTT conectado ao broker
📡 Inscrito em: presenca/attendance/+/+/tag-read
```

---

### 3. Frontend (projeto-presenca-frontend)

Interface web para gerenciar aulas e visualizar presenças em tempo real.

#### Instalação

```bash
cd projeto-presenca-frontend
npm install
```

#### Execução

```bash
npm run dev
```

O frontend estará disponível em `http://localhost:3000`

**Acesse:**
- Login: `http://localhost:3000/auth`
- Dashboard: `http://localhost:3000/dashboard`

---

## 📦 Ordem de Execução Recomendada

1. **Inicie o MQTT Broker (Mosquitto):**
   ```bash
   mosquitto -p 1883 -v
   ```

2. **Inicie o Backend:**
   ```bash
   cd projeto-presenca-backend
   npm run dev
   ```

3. **Inicie o Bridge Serial → MQTT:**
   ```bash
   cd "/Users/caramurulaptop/Documents/Sistemas Embarcados"
   node serial-to-mqtt.js
   ```

4. **Inicie o Frontend:**
   ```bash
   cd projeto-presenca-frontend
   npm run dev
   ```

---

## 🔧 Configuração do ESP32

1. Abra `esp32-rfid-serial.ino` no Arduino IDE
2. Instale as bibliotecas necessárias:
   - **MFRC522** (GithubCommunity)
3. Faça upload do código para o ESP32
4. Abra o Serial Monitor (115200 baud) para verificar

Veja `SERIAL_SETUP.md` para mais detalhes.

---

## 📝 Variáveis de Ambiente

### Backend (projeto-presenca-backend/.env)
```env
MQTT_BROKER_URL=mqtt://localhost:1883
MQTT_BROKER_USERNAME=  # Opcional
MQTT_BROKER_PASSWORD=  # Opcional
```

### Bridge Serial → MQTT
```bash
export MQTT_BROKER_URL="mqtt://localhost:1883"
export ROOM_NAME="101"
```

---

## 🐛 Troubleshooting

### Bridge Serial → MQTT não conecta
- Verifique se o Mosquitto está rodando: `ps aux | grep mosquitto`
- Verifique se a porta Serial está livre (feche Arduino IDE Serial Monitor)

### Backend não recebe mensagens MQTT
- Verifique se `MQTT_BROKER_URL` está configurado no `.env`
- Verifique os logs do backend para erros de conexão

### Frontend não atualiza
- Verifique se o backend está rodando em `localhost:3001`
- Abra o console do navegador (F12) para ver erros

### Prisma não funciona
- Execute `npx prisma generate` após instalar dependências
- Execute `npx prisma migrate deploy` para aplicar migrações

---

## 📚 Documentação Adicional

- `SERIAL_SETUP.md` - Detalhes sobre a configuração Serial
- `projeto-presenca-backend/MQTT_SETUP.md` - Configuração MQTT
- `projeto-presenca-backend/API_DOCS.md` - Documentação da API
- `projeto-presenca-frontend/API_DOCS.md` - Documentação do Frontend

---

## 🏗️ Arquitetura

```
ESP32 (RFID) 
    ↓ Serial (USB)
Node.js Bridge (serial-to-mqtt.js)
    ↓ MQTT
Backend (Fastify + Prisma)
    ↓ HTTP REST API
Frontend (Next.js)
```

---

## 📄 Licença

Veja `projeto-presenca-backend/LICENSE` para detalhes.

