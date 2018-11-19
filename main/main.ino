#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Tlv493d.h>

#define DEBUG
#define MAX_MQTT_PAYLOAD 100
#define CUBE_ID 0

#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_UP 3
#define DIR_DOWN 4
uint8_t direction;

char ssid[] = "asus-2.4g";        // your network SSID (name)
char pass[] = "en2ZP5Jm2fxD9uB6"; // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;

// Tlv493d Opject
Tlv493d Tlv493dMagnetic3DSensor = Tlv493d();

WiFiClient wifiClient;

IPAddress server(192,168,1,15);
PubSubClient client(wifiClient);
char message[MAX_MQTT_PAYLOAD];
String cubeId;

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

  // Initialize Tlv493d Magnetic 3D Sensor
  Tlv493dMagnetic3DSensor.begin();
  Tlv493dMagnetic3DSensor.setAccessMode(Tlv493dMagnetic3DSensor.MASTERCONTROLLEDMODE);
  Tlv493dMagnetic3DSensor.disableTemp();
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

  // Read a X-Y-Z value from the sensor
  delay(Tlv493dMagnetic3DSensor.getMeasurementDelay());
  Tlv493dMagnetic3DSensor.updateData();
  float x = Tlv493dMagnetic3DSensor.getX();
  float y = Tlv493dMagnetic3DSensor.getY();

  // Process sensing data
  if(fabsf(x) > fabsf(y)) {
    (x < 0)  ? direction = DIR_RIGHT : direction = DIR_LEFT;
  }

  if(fabsf(x) < fabsf(y)) {
    (y < 0)  ? direction = DIR_UP : direction = DIR_DOWN;
  }


  // Wrap message into Json format
  StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cubeId"] = CUBE_ID;
  root["gesture"] = direction;
  root.printTo(message, MAX_MQTT_PAYLOAD);
  // Send message to toolkit
  client.publish("sensorData", message);
  Serial.println("gesture data is sent");
  switch (direction) {
    case DIR_UP:   //3
      Serial.println("UP");
      break;
    case DIR_DOWN: //4
      Serial.println("DOWN");
      break;
    case DIR_LEFT: //1
      Serial.println("LEFT");
      break;
    case DIR_RIGHT://2
      Serial.println("RIGHT");
      break;
    default: //0
      Serial.println("NONE");
  }

  // Wait some time
  delay(2000);
}

/**************************************************************************/
/*
    Reconnects MQTT server when disconnecting
*/
/**************************************************************************/
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