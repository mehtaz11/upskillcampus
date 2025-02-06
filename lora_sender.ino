#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 setup
#define DS18B20_PIN 4
OneWire oneWire(DS18B20_PIN);
DallasTemperature DBT(&oneWire);

// GSR sensor setup
#define GSR_PIN A0
int gsrValue = 0;

// ECG sensor setup
#define ECG_PIN A1
int ecgValue = 0;

// LoRa module pins
#define ss 10
#define miso 12
#define rst 9
#define sck 13
#define mosi 11
#define dio0 2

int counter = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize DS18B20 sensor
  DBT.begin();

  // Initialize LoRa module
  Serial.println("Initializing LoRa...");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(866E6)) {
    Serial.println("LoRa initialization failed! Check connections.");
    while (1); // Halt if initialization fails
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa initialized successfully!");

  Serial.println("ECG, GSR, and DS18B20 Sensor Integration Started!");
}

void loop() {
  // Read GSR sensor value
  gsrValue = analogRead(GSR_PIN);

  // Read ECG sensor value
  ecgValue = analogRead(ECG_PIN);

  // Request temperature from DS18B20
  DBT.requestTemperatures();
  float temperature = DBT.getTempFByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: DS18B20 sensor not detected!");
    delay(2000); // Retry after a delay
    return;
  }

  // Log sensor data to Serial Monitor
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.print(" *F, GSR Value = ");
  Serial.print(gsrValue);
  Serial.print(", ECG Value = ");
  Serial.println(ecgValue);

  // Transmit data over LoRa
  LoRa.beginPacket();
  LoRa.print("Temperature = ");
  LoRa.print(temperature);
  LoRa.print(" *F, GSR Value = ");
  LoRa.print(gsrValue);
  LoRa.print(", ECG Value = ");
  LoRa.print(ecgValue);
  LoRa.endPacket();

  counter++;
  delay(3000); // Delay between transmissions
}