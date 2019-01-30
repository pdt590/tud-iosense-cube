#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SparkFun_APDS9960.h>

//#define DEBUG_APDS9960
#define MAX_MQTT_PAYLOAD 100
#define CUBE_ID 0

char ssid[] = "asus-2.4g";        // your network SSID (name)
char pass[] = "en2ZP5Jm2fxD9uB6"; // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;

// APDS9960 Object
SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint8_t gesture;

WiFiClient wifiClient;

IPAddress server(192,168,1,16);
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

  #ifdef DEBUG_APDS9960
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

  // Initialize APDS-9960 (configure I2C and initial values)
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
  // Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
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

  // Read a gesture from the device
  if ( apds.isGestureAvailable() ) {
    gesture = apds.readGesture();
    // Wrap message into Json format
    StaticJsonBuffer<MAX_MQTT_PAYLOAD> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cubeId"] = CUBE_ID;
    root["gesture"] = gesture;
    root.printTo(message, MAX_MQTT_PAYLOAD);
    // Send message to toolkit
    client.publish("sensorData", message);

    #ifdef DEBUG_APDS9960
    Serial.println("gesture data is sent");
    switch (gesture) {
      case DIR_UP:
        Serial.println("UP");
        break;
      case DIR_DOWN:
        Serial.println("DOWN");
        break;
      case DIR_LEFT:
        Serial.println("LEFT");
        break;
      case DIR_RIGHT:
        Serial.println("RIGHT");
        break;
      case DIR_NEAR:
        Serial.println("NEAR");
        break;
      case DIR_FAR:
        Serial.println("FAR");
        break;
      default:
        Serial.println("NONE");
    }
    #endif
  }

  // Wait some time
  delay(500);
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