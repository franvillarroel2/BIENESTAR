#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// --- Definición de Pines (ESP8266) ---
#define PIN_SPI_CS    15  // D8
#define PIN_RESET     16  // D0
#define RADIO_DIO0     4  // D2 (GPIO4) - Pin físico DIO0 conectado a D2

// Inicialización en modo seguro sin interrupciones nativas (RADIOLIB_NC)
SX1276 radio = new Module(PIN_SPI_CS, RADIOLIB_NC, PIN_RESET);

// --- Configuración WiFi ---
const char* ssid     = "Villarroel";
const char* password = "42751348";

// Dirección IP de tu computadora donde corre la API (Modificar de ser necesario)
const char* api_url  = "http://127.0.0.1:8000/data";

/**
 * Función para extraer los datos del String recibido por LoRa.
 * Formato esperado: "ID:1,T:25.5,H:60.0,F:1.0,A:0.0,G:350"
 */
bool parsePayload(String payload, int &id, float &t, float &h, float &f, float &a, int &g) {
  int iID = payload.indexOf("ID:");
  int iT  = payload.indexOf(",T:");
  int iH  = payload.indexOf(",H:");
  int iF  = payload.indexOf(",F:");
  int iA  = payload.indexOf(",A:");
  int iG  = payload.indexOf(",G:");

  if (iID < 0 || iT < 0 || iH < 0 || iF < 0 || iA < 0 || iG < 0) return false;

  id = payload.substring(iID + 3, iT).toInt();
  t  = payload.substring(iT + 3, iH).toFloat();
  h  = payload.substring(iH + 3, iF).toFloat();
  f  = payload.substring(iF + 3, iA).toFloat(); 
  a  = payload.substring(iA + 3, iG).toFloat();
  g  = payload.substring(iG + 3).toInt();

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Pin D2 como entrada manual de interrupción
  pinMode(RADIO_DIO0, INPUT);

  Serial.println("\n=================================");
  Serial.println("  BIENESTAR - INICIANDO MASTER   ");
  Serial.println("=================================");

  // Inicialización de Red WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP Local: " + WiFi.localIP().toString());

  // Inicialización del módulo de radio
  Serial.print("Iniciando LoRa Master en modo seguro... ");
  int state = radio.begin(915.0, 250.0, 7, 5, 0x12, 17, 8);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("Fallo LoRa de hardware, código: " + String(state));
    while (true);
  }
  
  // Encendemos la escucha en segundo plano
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("¡LoRa RX listo y escuchando!");
  } else {
    Serial.println("Error al activar RX: " + String(state));
  }
}

void loop() {
  // Datos a enviar a la API
  int nodo_id = 1;
  float temperatura = 25.9;
  int humedad = 33;
  bool presencia = true;
  int gas = 3333;

  // Estructura del JSON
  String json = "{"
    "\"nodo_id\":" + String(nodo_id) + ","
    "\"temperatura\":" + String(temperatura, 1) + ","
    "\"humedad\":" + String(humedad) + ","
    "\"presencia\":" + String(presencia ? "true" : "false") + ","
    "\"gas\":" + String(gas) +
  "}";

  // Envío de datos HTTP a la API local si hay red disponible
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, api_url);
    http.setTimeout(5000); // Evitamos bloqueos prolongados de red
    http.addHeader("Content-Type", "application/json");

    Serial.println("\n--- Enviando datos a API ---");
    Serial.println("JSON: " + json);
    int httpCode = http.POST(json);

    if (httpCode > 0) {
      Serial.println("✅ Respuesta del servidor: " + String(httpCode));
    } else {
      Serial.println("❌ Error HTTP: " + http.errorToString(httpCode));
    }
    http.end();
  } else {
    Serial.println("❌ Error: WiFi desconectado. No se puede reportar a la API.");
  }

  // Esperar 10 segundos antes de enviar nuevamente
  delay(10000);

  // Garantiza estabilidad de las tareas de segundo plano de la pila de red del chip
  yield();
}