/*
  Grupo:
  Diego Tasso da Cunha Ferreira - 1230202973
  Lucas de Andrade Ramos Caramuru de Paiva - 1247907020
  Maria Julia Eduarda Maia Silva - 1240202810
  Pedro Henrique da Silva Novais - 1230119539
  Victor Jacques Freire Sampaio - 1230203770
  Daniel Nogueira da Silva - 1230109810
*/

// ESP32 RFID - Versão WiFi + MQTT Direto
// Conecta ao WiFi e envia dados diretamente para o broker MQTT

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// --- Configurações WiFi ---
const char* ssid = "GARURIS";
const char* password = "amarelo8792";

// --- Configurações MQTT ---
// Se estiver rodando local, use o IP do seu computador (ex: 192.168.1.X)
// localhost não funciona aqui porque o ESP32 é outro dispositivo na rede
const char* mqtt_server = "192.168.0.185"; // <--- ALTERE PARA O IP DO SEU PC
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";

// Tópicos
const char* topic_attendance = "presenca/attendance";
const char* room_name = "Sala 101";

// --- Pinos (mantendo a configuração que funcionou) ---
static const uint8_t PIN_SDA = 4;   // SS/SDA
static const uint8_t PIN_RST = 5;   // RST
static const uint8_t PIN_SCK = 18;  // SCK
static const uint8_t PIN_MISO = 19; // MISO
static const uint8_t PIN_MOSI = 23; // MOSI
static const uint8_t PIN_LED = 2;   // LED

// Objetos
WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 rfid(PIN_SDA, PIN_RST);

String esp32Id = "";
unsigned long lastCardRead = 0;
const unsigned long CARD_COOLDOWN = 3000;

// Função para obter ID único
String getESP32Id() {
  uint64_t chipid = ESP.getEfuseMac();
  char idStr[10];
  sprintf(idStr, "%04X", (uint16_t)(chipid >> 32));
  return String("esp32-") + String(idStr);
}

// Converte UID para String
String uidToString(const MFRC522::Uid *uid) {
  String s = "";
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
    if (i + 1 < uid->size) s += ":";
  }
  s.toUpperCase();
  return s;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(PIN_LED, !digitalRead(PIN_LED)); // Pisca enquanto conecta
  }

  digitalWrite(PIN_LED, LOW);
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até reconectar
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT...");
    
    // Cria um Client ID único
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      
      // Publica mensagem de boas-vindas
      String msg = "ESP32 " + esp32Id + " conectado!";
      client.publish("presenca/status", msg.c_str());
      
      digitalWrite(PIN_LED, HIGH);
      delay(500);
      digitalWrite(PIN_LED, LOW);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  
  Serial.begin(115200);
  delay(1000);
  
  esp32Id = getESP32Id();
  Serial.print("ESP32 ID: ");
  Serial.println(esp32Id);

  // Inicializar SPI e RFID
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SDA);
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.print("RC522 versao: 0x");
  Serial.println(version, HEX);
  
  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERRO: RC522 nao encontrado!");
  } else {
    Serial.println("RC522 OK!");
  }

  // Configurar WiFi e MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Verificar cartão
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Cooldown
  unsigned long now = millis();
  if (now - lastCardRead < CARD_COOLDOWN) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastCardRead = now;

  // Processar leitura
  String uid = uidToString(&rfid.uid);
  Serial.print("Cartao lido: ");
  Serial.println(uid);

  // Montar JSON manualmente para evitar dependência extra
  // Formato esperado pelo backend:
  // { "tagId": "UID", "esp32Id": "ID", "room": "Sala 101", "timestamp": 123... }
  
  String payload = "{";
  payload += "\"tagId\":\"" + uid + "\",";
  payload += "\"esp32Id\":\"" + esp32Id + "\",";
  payload += "\"room\":\"" + String(room_name) + "\"";
  payload += "}";

  // Tópico específico: presenca/attendance/Sala 101/esp32-XXXX/tag-read
  String topic = String(topic_attendance) + "/" + room_name + "/" + esp32Id + "/tag-read";
  
  // Substituir espaços por _ no tópico se necessário
  topic.replace(" ", "_");
  
  Serial.print("Publicando em: ");
  Serial.println(topic);
  Serial.println(payload);

  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Sucesso!");
    // Piscar LED 2x rápido
    for(int i=0; i<2; i++) {
      digitalWrite(PIN_LED, HIGH); delay(100);
      digitalWrite(PIN_LED, LOW); delay(100);
    }
  } else {
    Serial.println("Falha ao publicar.");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

