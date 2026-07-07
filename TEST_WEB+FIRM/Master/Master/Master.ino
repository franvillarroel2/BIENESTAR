#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <LoRa.h>

// Pines LoRa en el ESP8266 Maestro
#define SS_PIN   15  // D8
#define RST_PIN  16  // D0
#define DIO0_PIN  4  // D2

// Credenciales de Red WiFi de Mendoza
const char* ssid = "Villarroel";
const char* password = "42751348";

// Endpoint de tu API de FastAPI encargada de recibir las métricas
const char* apiUrl = "http://127.0.0.1:8000/data"; 

struct __attribute__((__packed__)) SensorData {
  int nodo_id;
  float temperatura;
  float humidity;
  bool presencia;
  int gas;
};

void setup() {
  Serial.begin(115200);
  
  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // Inicialización de Receptor LoRa
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(915E6)) {
    Serial.println("Error al iniciar LoRa en Maestro.");
    while (1);
  }
  Serial.println("Maestro LoRa escuchando paquetes...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == sizeof(SensorData)) {
    SensorData data;
    
    // Leer los bytes directamente del aire colocándolos en la estructura
    LoRa.readBytes((uint8_t*)&data, sizeof(SensorData));
    
    Serial.println("\n--- ¡Paquete Recibido desde Esclavo! ---");
    
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      
      http.begin(client, apiUrl);
      http.addHeader("Content-Type", "application/json");

      // Construcción manual del String JSON optimizado para la colmena
      String jsonPayload = "{\"nodo_id\":" + String(data.nodo_id) + 
                           ",\"temperatura\":" + String(data.temperatura, 2) + 
                           ",\"humedad\":" + String(data.humidity, 2) + 
                           ",\"presencia\":" + (data.presencia ? "true" : "false") + 
                           ",\"gas\":" + String(data.gas) + "}";

      Serial.println("Enviando JSON a la API: " + jsonPayload);
      int httpResponseCode = http.POST(jsonPayload);
      
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("Código API: "); Serial.println(httpResponseCode);
        Serial.print("Respuesta: "); Serial.println(response);
      } else {
        Serial.print("Error en envío HTTP POST: "); Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("Envío cancelado: WiFi desconectado.");
    }
  }
}