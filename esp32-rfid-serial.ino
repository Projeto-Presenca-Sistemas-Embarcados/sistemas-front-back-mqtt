// ESP32 RFID - Envia dados via Serial (sem WiFi)
// O computador recebe via Serial e envia via MQTT
#define MFRC522_SPICLOCK 1000000

#include <SPI.h>
#include <MFRC522.h>

// Pinos
static const uint8_t PIN_SDA = 4;   // SS/SDA (GPIO4 - ADC2_0, OK para SPI)
static const uint8_t PIN_RST = 5;   // RST (GPIO5 - OK)
static const uint8_t PIN_SCK = 18;  // SCK (GPIO18 - VSPI_SCK padrão)
static const uint8_t PIN_MISO = 19; // MISO (GPIO19 - VSPI_MISO padrão)
static const uint8_t PIN_MOSI = 23; // MOSI (GPIO23 - VSPI_MOSI padrão)
static const uint8_t PIN_LED = 2;   // LED (GPIO2 - OK)

MFRC522 rfid(PIN_SDA, PIN_RST);

unsigned long lastCardRead = 0;
const unsigned long CARD_COOLDOWN = 3000;

String esp32Id = ""; // ID único do ESP32 (será definido no setup)

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

void blink(unsigned n = 2, unsigned onMs = 80, unsigned offMs = 80) {
  pinMode(PIN_LED, OUTPUT);
  for (unsigned i = 0; i < n; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(onMs);
    digitalWrite(PIN_LED, LOW);
    delay(offMs);
  }
}

String getESP32Id() {
  // Usar chip ID único do ESP32 (não requer WiFi)
  uint64_t chipid = ESP.getEfuseMac();
  
  // Converter para string hexadecimal (últimos 4 dígitos)
  char idStr[10];
  sprintf(idStr, "%04X", (uint16_t)(chipid >> 32));
  
  return String("esp32-") + String(idStr);
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  Serial.begin(115200);
  delay(1000);
  
  // Obter ID único do ESP32 (baseado no chip ID, sem WiFi)
  esp32Id = getESP32Id();
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("ESP32 RFID - Modo Serial");
  Serial.println("========================================");
  Serial.print("ESP32 ID: ");
  Serial.println(esp32Id);
  Serial.println();
  Serial.println("Este ESP32 apenas lê cartões RFID");
  Serial.println("e envia os dados via Serial.");
  Serial.println("O computador recebe e envia via MQTT.");
  Serial.println();
  
  // Enviar identificação do ESP32
  Serial.print("ESP32_ID:");
  Serial.print(esp32Id);
  Serial.println();
  
  // Inicializar RFID
  pinMode(PIN_SDA, OUTPUT);
  digitalWrite(PIN_SDA, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SDA);
  delay(50);
  
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
  
  Serial.println();
  Serial.println("Sistema pronto! Aproxime um cartao...");
  Serial.println();
  Serial.println("FORMATO: TAG:UID|ROOM:Sala 101|ESP32:esp32-XXXX");
  Serial.println();
  
  blink(3, 100, 100);
}

void loop() {
  // Verificar cartão
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(5);
    return;
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    delay(5);
    return;
  }
  
  // Cooldown
  unsigned long now = millis();
  if (now - lastCardRead < CARD_COOLDOWN) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastCardRead = now;
  
  // Ler UID
  String uid = uidToString(&rfid.uid);
  
  Serial.println("----- CARTÃO DETECTADO -----");
  Serial.print("UID: ");
  Serial.println(uid);
  
  MFRC522::PICC_Type t = rfid.PICC_GetType(rfid.uid.sak);
  Serial.print("Tipo: ");
  Serial.println(MFRC522::PICC_GetTypeName(t));
  Serial.println("----------------------------");
  
  // Enviar formato simples para o computador processar
  // Formato: TAG:UID|ROOM:Sala 101|ESP32:esp32-XXXX
  Serial.print("TAG:");
  Serial.print(uid);
  Serial.print("|ROOM:Sala 101|ESP32:");
  Serial.print(esp32Id);
  Serial.println();
  
  blink(5, 150, 100);
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  Serial.println();
}



