#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "Adafruit_VL53L0X.h"

//#define DEBUG
#define MAX_MQTT_PAYLOAD 100
#define CUBE_ID 0

//char ssid[] = "Connectify-thang";    // your network SSID (name)
//char pass[] = "12345678";    // your network password (use for WPA, or use as key for WEP)
char ssid[] = "asus-2.4g";    // your network SSID (name)
char pass[] = "en2ZP5Jm2fxD9uB6";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
uint32_t range;
String cubeId;

// VL53L0X Object
VL53L0X_RangingMeasurementData_t measure;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

//char server[] = "www.google.com";    // name address for server (using DNS)
IPAddress server(10,8,0,198);  // numeric IP for server (no DNS)

WiFiClient wifiClient;
PubSubClient client(wifiClient);
char message[MAX_MQTT_PAYLOAD];

/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/
void setup(void)
{
  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);
  
  Serial.begin(9600);

  #ifdef DEBUG
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  #endif

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 5 seconds for connection:
    delay(5000);
  }

  Serial.println("Connected to wifi");
  // you're connected now, so print out the status:
  printWiFiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  client.setServer(server, 1883);
  client.setCallback(callback);

  // Init DPS310 sensor
  Serial.println("Initialize VL53L0X Time-to-Flight Sensor...");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete (your own code
    should go here)
*/
/**************************************************************************/
void loop(void)
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  //Featch range data
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    range = measure.RangeMilliMeter;
    Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
  } else {
    Serial.println(" out of range ");
  }

  // Wrap message into Json format
  StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cubeId"] = CUBE_ID;
  root["range"] = range;
  root.printTo(message, MAX_MQTT_PAYLOAD);
  // Send message to toolkit
  client.publish("sensorData", message);
  Serial.println("Range data is sent");
  
  // Wait some time
  delay(500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    cubeId = "cubeClient-";
    cubeId += String(random(0xffff), HEX);

    // Attempt to connect
    if (client.connect(cubeId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**************************************************************************/
/*
    Displays information of WiFi network
*/
/**************************************************************************/
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}