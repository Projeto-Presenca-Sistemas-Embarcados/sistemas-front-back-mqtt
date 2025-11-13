#!/usr/bin/env node
/**
 * Script para receber dados do ESP32 via Serial
 * e enviar via MQTT para o backend
 * 
 * Uso: node serial-to-mqtt.js
 * 
 * Requer: npm install serialport mqtt
 */

const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const mqtt = require('mqtt');

// Configuração MQTT
const MQTT_BROKER = process.env.MQTT_BROKER_URL || 'mqtt://localhost:1883';
const ROOM_NAME = process.env.ROOM_NAME || '101'; // Nome exato da sala no banco de dados

// ID do ESP32 será detectado automaticamente via Serial
let detectedESP32Id = null;

// Conectar ao MQTT
console.log('Conectando ao MQTT broker...');
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on('connect', () => {
  console.log('✅ MQTT conectado!');
});

mqttClient.on('error', (error) => {
  console.error('❌ Erro MQTT:', error);
});

// Encontrar porta Serial do ESP32
async function findESP32Port() {
  const ports = await SerialPort.list();
  
  console.log('\nPortas Serial disponíveis:');
  ports.forEach((port, i) => {
    console.log(`  ${i + 1}. ${port.path} - ${port.manufacturer || 'Desconhecido'}`);
  });
  
  // Tentar encontrar ESP32 (geralmente tem "USB" ou "Serial" no nome)
  const esp32Port = ports.find(port => 
    port.manufacturer?.includes('Silicon') ||
    port.manufacturer?.includes('CH340') ||
    port.path.includes('usbserial') ||
    port.path.includes('ttyUSB') ||
    port.path.includes('tty.usbserial')
  );
  
  if (esp32Port) {
    console.log(`\n✅ ESP32 encontrado: ${esp32Port.path}`);
    return esp32Port.path;
  }
  
  // Se não encontrar, usar a primeira porta
  if (ports.length > 0) {
    console.log(`\n⚠️  Usando primeira porta disponível: ${ports[0].path}`);
    return ports[0].path;
  }
  
  throw new Error('Nenhuma porta Serial encontrada!');
}

// Variável global para armazenar o caminho da porta
let portPath = null;

// Processar mensagem do ESP32
function processMessage(line, currentPortPath) {
  // Detectar ID do ESP32 quando ele envia no boot
  if (line.startsWith('ESP32_ID:')) {
    detectedESP32Id = line.replace('ESP32_ID:', '').trim();
    console.log(`\n✅ ESP32 identificado: ${detectedESP32Id}`);
    console.log(`   Porta: ${currentPortPath || 'desconhecida'}\n`);
    return;
  }
  
  // Formato esperado: TAG:UID|ROOM:Sala 101|ESP32:esp32-XXXX
  if (!line.includes('TAG:')) return;
  
  const parts = line.split('|');
  let tagId = '';
  const room = ROOM_NAME;
  let esp32Id = detectedESP32Id || 'esp32-unknown';
  
  parts.forEach(part => {
    if (part.startsWith('TAG:')) {
      tagId = part.replace('TAG:', '').trim();
    } else if (part.startsWith('ESP32:')) {
      const msgEsp32Id = part.replace('ESP32:', '').trim();
      if (msgEsp32Id) {
        esp32Id = msgEsp32Id;
      }
    }
  });
  
  if (!tagId) return;
  
  // Criar payload JSON
  const payload = {
    tagId: tagId,
    room: room,
    esp32Id: esp32Id,
    timestamp: Date.now()
  };
  
  // Publicar no MQTT
  const topic = `presenca/attendance/${room}/${esp32Id}/tag-read`;
  
  if (mqttClient.connected) {
    mqttClient.publish(topic, JSON.stringify(payload), (err) => {
      if (err) {
        console.error('❌ Erro ao publicar MQTT:', err);
      } else {
        console.log(`✅ Tag enviada: ${tagId} -> ${topic}`);
      }
    });
  } else {
    console.warn('⚠️  MQTT desconectado, tag não enviada:', tagId);
  }
}

// Inicializar
async function main() {
  try {
    portPath = await findESP32Port();
    
    console.log(`\nConectando à porta ${portPath}...`);
    const port = new SerialPort({
      path: portPath,
      baudRate: 115200,
    });
    
    const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));
    
    port.on('open', () => {
      console.log('✅ Porta Serial aberta!');
      console.log('Aguardando identificação do ESP32...\n');
    });
    
    port.on('error', (err) => {
      console.error('❌ Erro Serial:', err);
    });
    
    parser.on('data', (line) => {
      const trimmed = line.toString().trim();
      
      // Mostrar todas as mensagens do ESP32
      if (trimmed.length > 0) {
        console.log('📨 ESP32:', trimmed);
        
        // Processar identificação do ESP32 ou tags RFID
        if (trimmed.startsWith('ESP32_ID:') || trimmed.includes('TAG:')) {
          processMessage(trimmed, portPath);
        }
      }
    });
    
    console.log('\n✅ Sistema pronto!');
    console.log('Aguardando identificação do ESP32...');
    console.log('Aproxime um cartão RFID no ESP32...\n');
    
  } catch (error) {
    console.error('❌ Erro:', error.message);
    process.exit(1);
  }
}

main();

