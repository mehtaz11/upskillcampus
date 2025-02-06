#include <ESP8266WiFi.h>   // ESP8266 Wi-Fi library
#include <SPI.h>           // SPI library
#include <LoRa.h>          // LoRa library
#include <PubSubClient.h>  // MQTT Client library

// LoRa module pins for ESP8266
#define rst  D0   // GPIO0  (Reset)
#define ss   D8   // GPIO15 (Chip Select)
#define sck  D5   // GPIO14 (Serial Clock)
#define miso D6   //GPIO12  (Master-In-Slave-Out)
#define mosi D7   //GPIO13  (Master-Out-Slave-In)
#define intt  D2   //GPIO4   (Interrupt)

// Buzzer pin
#define buzzer D1  // GPIO5

// Threshold values
const float tempThreshold = 75.00;    // Temperature threshold 
const int gsrThreshold = 300;     // GSR threshold 
//const int ecgThreshold = 500;     // ECG threshold 

// WiFi and MQTT configurations
const char* ssid = "Redmond";  // Wifi Name
const char* password = "asdfghjkl";  // Wifi Password
const char* mqtt_server = "192.168.218.55";  // MQTT Broker IP Address
const char* mqtt_topic = "sensor/data";  // Topic to publish received data
const char* mqtt_status_topic = "sensor/status";  // Status topic for alerts

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  pinMode(buzzer, OUTPUT);  // Set buzzer pin as output
  digitalWrite(buzzer, LOW); // Ensure buzzer is off initially

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");

  // Set up MQTT
  client.setServer(mqtt_server, 1883);  // Use your local MQTT broker IP
  while (!client.connected()) {
    if (client.connect("LoRaReceiver")) {
      Serial.println("MQTT connected");
    } else {
      delay(1000);
      Serial.println("Retrying MQTT connection...");
    }
  }

  // Initialize LoRa module
  LoRa.setPins(ss, rst, intt);
  if (!LoRa.begin(866E6)) {  // Initialize LoRa with frequency of 866 MHz
    Serial.println("LoRa initialization failed!");
    while (1);  // Halt if initialization fails
  }
  LoRa.setSyncWord(0xF3);  // Set sync word for network identification
  Serial.println("LoRa Receiver Ready!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";

    // Read the received packet
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    Serial.print("Received: ");
    Serial.println(receivedData);

    // Variables to store parsed temperature, GSR, and ECG data
    float tempValue = 0.0;
    int gsrValue = 0, ecgValue = 0;

    // Parse the received data for temperature, GSR, and ECG
    int matched = sscanf(receivedData.c_str(), "Temperature = %f *F, GSR Value = %d, ECG Value = %d", &tempValue, &gsrValue, &ecgValue);

    // Trigger alert if threshold exceeded
    if (matched == 3 && (tempValue > tempThreshold || gsrValue < gsrThreshold /*|| ecgValue < ecgThreshold*/)) {
      Serial.println("Warning: Threshold exceeded! Turning on buzzer for 1 second...");
      digitalWrite(buzzer, HIGH);  // Turn on buzzer
      delay(1000);                 // Keep buzzer on for 1 second
      digitalWrite(buzzer, LOW);   // Turn buzzer off

      // Publish status message to MQTT broker
      if (client.connected()) {
        client.publish(mqtt_status_topic, "Critical Alert! Take action!");
      }
    }

    // Publish received data to MQTT broker
    if (client.connected()) {
      client.publish(mqtt_topic, receivedData.c_str());
    }

    delay(1000);  // Updates every second
  }
  // Maintain MQTT connection
  client.loop();
}