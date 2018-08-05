#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <Dps310.h>
#include <PubSubClient.h>

//#define DEBUG
#define MAX_MQTT_PAYLOAD 100

//char ssid[] = "Connectify-thang";    // your network SSID (name)
//char pass[] = "12345678";    // your network password (use for WPA, or use as key for WEP)
char ssid[] = "asus-2.4g";    // your network SSID (name)
char pass[] = "en2ZP5Jm2fxD9uB6";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
String cubeId;

int32_t temperature;
int32_t pressure;
int16_t oversampling = 7;
int16_t ret;
String lampUrl;
// Dps310 Object
Dps310 Dps310PressureSensor = Dps310();

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
  Serial.println("Initialize DPS310 Pressure Sensor...");
  Dps310PressureSensor.begin(Wire);
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  //String myTopic(topic); 
  if (strcmp("lampUrl", topic)==0) {
    StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((const char*)payload);
    lampUrl = root["lampUrl"].as<String>();
    client.unsubscribe(topic);
  }
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

  // Get pressure data
  ret = Dps310PressureSensor.measurePressureOnce(pressure, oversampling);
  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("Dps310 FAIL! ret = ");
    Serial.println(ret);
  }

  // Wrap message into Json format
  StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cubeId"] = cubeId;
  (lampUrl == "") ? (root["cubeState"] = false) : (root["cubeState"] = true);
  root["cubeLampUrl"] = lampUrl;
  root.printTo(message, MAX_MQTT_PAYLOAD);
  // Send message to dashboard
  client.publish("cubeList", message);
  
  if (lampUrl != "") {
    StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cubeId"] = cubeId;
    root["lampUrl"] = lampUrl;
    root["pressure"] = pressure;
    root.printTo(message, MAX_MQTT_PAYLOAD);
    // Send message to toolkit
    client.publish("sensorData", message);
    Serial.println(message);
    Serial.println("Pressure data is sent");
  } else {
    Serial.println("Cube doesn't connect yet");
  }
  
  // Wait some time
  delay(100);
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
      Serial.println("subscribing some topics ...");
      // subscribe
      client.subscribe("lampUrl");
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