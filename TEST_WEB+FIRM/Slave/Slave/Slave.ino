#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <DHT.h>
#include <arduinoFFT.h>

// ==== ID del Nodo ====
const int NODO_ID = 1; 

// ==== Configuración de Detección (Umbrales) ====
const double FREQ_MIN = 200.0;    // Rango típico de abejas
const double FREQ_MAX = 600.0;
const double AMP_UMBRAL = 50.0;   // Ajusta según sensibilidad de tu micrófono

// ==== Pines LoRa ====
#define PIN_SPI_MISO  21
#define PIN_SPI_MOSI  8
#define PIN_SPI_SCK   7
#define PIN_SPI_CS    6
#define PIN_RESET     10
#define RADIO_DIO1    4

SX1276 radio = new Module(PIN_SPI_CS, RADIO_DIO1, PIN_RESET);

// ==== Pines sensores ====
#define DHT_PIN       0     // GPIO0
#define MIC_PIN       1     // GPIO1 (ADC1_CH0)
#define MQ135_PIN     2     // GPIO2 (ADC1_CH1)

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// ==== Configuración de FFT ====
const uint16_t samples = 128;
const double samplingFrequency = 5000.0;
double vReal[samples];
double vImag[samples];
ArduinoFFT FFT = ArduinoFFT(vReal, vImag, samples, samplingFrequency);

void setup() {
  Serial.begin(115200);
  delay(1500);
  
  dht.begin();
  Serial.println("Inicializando Esclavo...");

  // Inicialización explícita del bus SPI para el ESP32-C3
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS);

  // Inicialización de RadioLib SX1276
  int state = radio.begin(915.0, 250.0, 7, 5, 0x12, 17, 8);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Error LoRa: ");
    Serial.println(state);
  } else {
    Serial.println("LoRa listo.");
  }
}

void loop() {
  // 1. Lectura del micrófono y FFT
  for (int i = 0; i < samples; i++) {
    vReal[i] = analogRead(MIC_PIN);
    vImag[i] = 0;
    delayMicroseconds(1000000 / samplingFrequency);
  }

  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();
  double peakFreq = FFT.majorPeak();

  // Amplitud RMS
  double sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += sq(vReal[i] - 2048); 
  }
  double amplitude = sqrt(sum / samples);

  // 2. Lógica de Presencia (F)
  float presenciaParaMaster = 0.0;
  if (amplitude > AMP_UMBRAL && peakFreq >= FREQ_MIN && peakFreq <= FREQ_MAX) {
    presenciaParaMaster = 1.0; // Cambia a 1.0 si detecta actividad de abejas
  }

  // 3. Lectura de otros sensores
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int mq135Raw = analogRead(MQ135_PIN);

  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity)) humidity = 0.0;

  // 4. Armar mensaje EXACTO para el parsePayload del Master
  String payload = "ID:" + String(NODO_ID)
                 + ",T:" + String(temperature, 1)
                 + ",H:" + String(humidity, 1)
                 + ",F:" + String(presenciaParaMaster, 1) 
                 + ",A:0.0"                               
                 + ",G:" + String(mq135Raw);

  // 5. Mostrar y Transmitir
  Serial.println("Datos locales -> T: " + String(temperature) + " P: " + (presenciaParaMaster > 0 ? "SI" : "NO"));
  Serial.println("Enviando String por RadioLib: " + payload);
  
  int txState = radio.transmit(payload);
  if (txState == RADIOLIB_ERR_NONE) {
    Serial.println("OK.");
  } else {
    Serial.println("Error TX: " + String(txState));
  }

  delay(5000); 
}