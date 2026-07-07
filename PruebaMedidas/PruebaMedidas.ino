#include <Arduino.h>
#include <DHT.h>

// ==== CONFIGURACIÓN DE PINES (ESP32-C3) ====
#define DHT_PIN      3      // GPIO3
#define MQ135_PIN    2      // GPIO2 (ADC1_CH2)
#define MIC_PIN      1      // GPIO1 (ADC1_CH1)

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// Constantes para el micrófono
const int sampleWindow = 50; // Ventana de 50ms para medir amplitud (20Hz)
unsigned int sample;

void setup() {
  Serial.begin(9600);
  // En la C3 el puerto Serial suele tardar un poco en estar listo
  delay(2000); 
  
  dht.begin();
  
  // Configuración de resolución del ADC (0-4095)
  analogReadResolution(12); 
  
  Serial.println("--- ESP32-C3 SuperMini: Inicio de mediciones ---");
}

void loop() {
  // 1. LECTURA DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // 2. LECTURA MQ135 (GAS)
  int gasRaw = analogRead(MQ135_PIN);

  // 3. LECTURA MAX9814 (SONIDO - Amplitud Pico a Pico)
  unsigned long startMillis = millis();
  unsigned int peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  // Recolectar datos por 50ms
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(MIC_PIN);
    if (sample < 4095) { // Omitir lecturas erróneas
      if (sample > signalMax) signalMax = sample;
      else if (sample < signalMin) signalMin = sample;
    }
  }
  peakToPeak = signalMax - signalMin; 

  // ==== MOSTRAR RESULTADOS EN SERIAL ====
  Serial.println("---------------------------------");
  
  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR: No se pudo leer el sensor DHT11");
  } else {
    Serial.print("Temperatura: "); Serial.print(t); Serial.println(" °C");
    Serial.print("Humedad: "); Serial.print(h); Serial.println(" %");
  }

  Serial.print("Calidad de Aire (MQ135): "); Serial.println(gasRaw);
  Serial.print("Amplitud Sonido (MAX9814): "); Serial.println(peakToPeak);

  // Esperar 2 segundos antes de la siguiente lectura
  delay(2000);
}