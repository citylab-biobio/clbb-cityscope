#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <map>
#include <string>
#include <Adafruit_PN532.h>

// Definimos pines del Bus SPI estándar del ESP32 (Compartidos por todos)
#define PN532_SCK  18
#define PN532_MISO 19
#define PN532_MOSI 23

// Definimos el Array de pines Slave Select (SS) directos para los 7 sensores
const uint8_t ssPins[7] = {4, 16, 17, 21, 32, 25, 26};

// Instanciamos un Array de 7 objetos Adafruit_PN532
Adafruit_PN532 nfc[7] = {
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[0]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[1]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[2]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[3]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[4]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[5]),
  Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, ssPins[6])
};

const char* WIFI_SSID = "mmo";
const char* WIFI_PASSWORD = "10293847";
const char* API_URL = "http://10.124.136.31:8500/api/set_map_state/";

std::map<std::string, String> ID_list = {
  { "BBBE1406", "13" }, // Placa 1 actual
  { "3FBA5E02", "14" }, // Placa 2 actual
  { "6922EB18", "15" }, // Placa 3 actual
  { "6094FC55", "16" }, // Placa 4 actual
  { "4FA95802", "17" }, // Placa 5 actual
  { "70190B55", "18" }, // Placa 6 actual
  { "60D8F155", "19" }, // Placa 7 actual
  { "3FEC4002", "20" }, // Placa 1 futuro
  { "35284E06", "21" }, // Placa 2 futuro
  { "69043A18", "22" }, // Placa 3 futuro
  { "0BAE1706", "23" }, // Placa 4 futuro
  { "1C164C06", "24" }, // Placa 5 futuro
  { "60801355", "25" }, // Placa 6 futuro
  { "B963C855", "26" }, // Placa 7 futuro
};

// --- ARREGLOS DE MEMORIA PARA TOLERANCIA A FALLOS ---
String cacheEstados[7] = {"0", "0", "0", "0", "0", "0", "0"}; 

void enviarDatosAPI(String uidString) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String urlCompleta = String(API_URL) + "?slots=" + uidString;

    Serial.println("------------------------------------------------");
    Serial.println("[HTTP] Conectando a: " + urlCompleta);

    http.begin(urlCompleta);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.printf("[HTTP] Código respuesta: %d\n", httpResponseCode);
      String response = http.getString();
      Serial.println("[HTTP] Respuesta servidor: " + response);
    } else {
      Serial.printf("[HTTP] Error en envío: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end(); 
  } else {
    Serial.println("[WIFI] Error: El dispositivo no esta conectado a una red WiFi.");
  }
}

void setup() {
  Serial.begin(115200);

  // --- [BLOQUE] Configuración WiFi ---
  Serial.println("\n[WIFI] Limpiando configuraciones viejas...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000);

  WiFi.mode(WIFI_STA);
  Serial.print("[WIFI] Conectando a ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    intentos++;
    if (intentos > 20) {
      Serial.print("\n[ERROR] Estado: ");
      Serial.print(WiFi.status());
      if (WiFi.status() == 6) {
        Serial.println("[STATUS] (Reintentando WiFi.begin...)");
        WiFi.disconnect();
        WiFi.reconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
      intentos = 0;
    }
  }

  Serial.println("\n[WIFI] Conectado a la red WiFi");
  Serial.print("[WIFI] IP asignada: ");
  Serial.println(WiFi.localIP());

  // --- [BLOQUE] Inicialización Directa de Sensores ---
  Serial.println("[NFC] Iniciando escaneo de sensores en topología directa...");

  for (int i = 0; i < 7; i++) {
    // Retardo escalonado mitigador de brownout
    delay(100);
    
    nfc[i].begin(); 
    
    uint32_t versiondata = nfc[i].getFirmwareVersion(); 

    if (!versiondata) {
      Serial.print("[ERROR]: No se encontro el módulo PN53x en Canal ");
      Serial.println(i + 1);
    } else {
      Serial.print("[NFC] Sensor ENCONTRADO en Canal ");
      Serial.println(i + 1);
      
      Serial.print("[NFC] Chip encontrado: PN5");
      Serial.println((versiondata >> 24) & 0xFF, HEX);
      Serial.print("[NFC] Firmware ver. ");
      Serial.print((versiondata >> 16) & 0xFF, DEC);
      Serial.print('.');
      Serial.println((versiondata >> 8) & 0xFF, DEC);
      
      nfc[i].SAMConfig(); 
      nfc[i].setPassiveActivationRetries(0x01);
    }
  }

  Serial.println("[STATUS] Listo");
}

void loop() {
  String estadoFinal = ""; 
  
  // --- TELEMETRÍA DE ESTADO ---
  String sensoresCaidos = ""; 
  String sensoresActivos = "";

  Serial.println("\n[STATUS] Iniciando escaneo completo...");

  // Recorremos los 7 objetos instanciados
  for (int i = 0; i < 7; i++) {
  
    // --- AUDITORIA DE HARDWARE PRE-LECTURA ---
    uint32_t hardwareCheck = nfc[i].getFirmwareVersion();

    if (!hardwareCheck) {
      // [FALLA FÍSICA]
      sensoresCaidos += String(i + 1) + " ";
    } else {
      // [HARDWARE SANO]
      sensoresActivos += String(i + 1) + " ";
      
      boolean success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; 
      uint8_t uidLength;                       

      success = nfc[i].readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

      if (success) {
        Serial.print("[NFC] Tarjeta detectada en Canal "); 
        Serial.println(i + 1);

        String uidString = ""; 

        for (uint8_t j = 0; j < uidLength; j++) {
          if (uid[j] < 0x10) {
            uidString += "0";
          }
          uidString += String(uid[j], HEX); 
        }

        uidString.toUpperCase();

        std::string key = std::string(uidString.c_str()); 

        if (ID_list.count(key)) {
          String idLogico = ID_list[key]; 
          cacheEstados[i] = idLogico;     
        } else {
          Serial.print("[ERROR] Tarjeta desconocida en Canal "); 
          Serial.print(i + 1);
          Serial.print(" UID: "); 
          Serial.println(uidString);
          cacheEstados[i] = "0";          
        }

      } else {
        cacheEstados[i] = "0";            
      }
    }

    estadoFinal += cacheEstados[i];
    
    if (i < 6) {
      estadoFinal += ",";
    }
  }

  // --- REPORTE DE DIAGNÓSTICO PRE-TRANSMISIÓN ---
  Serial.println("------------------------------------------------");
  Serial.println("[DIAGNÓSTICO DE BUS SPI]");
  
  if (sensoresActivos.length() > 0) {
    Serial.println("  -> Sensores EN LÍNEA (Hardware OK): Canales [ " + sensoresActivos + "]");
  } else {
    Serial.println("  -> Sensores EN LÍNEA (Hardware OK): [ NINGUNO ]");
  }

  if (sensoresCaidos.length() > 0) {
    Serial.println("  -> Sensores FUERA DE LÍNEA (Fallo SPI): Canales [ " + sensoresCaidos + "]");
    Serial.println("[MITIGACIÓN] Inyectando último estado conocido en el payload para canales afectados.");
  }

  Serial.print("[STATUS] Cadena generada para API: ");
  Serial.println(estadoFinal);

  // INTENTO 1: Cargar al buffer
  Serial.println("[STATUS] Enviando carga...");
  enviarDatosAPI(estadoFinal);

  Serial.println("[STATUS] Ciclo completado. Esperando 2s...");
  delay(2000); 
}