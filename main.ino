#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
char ssid[] = "dlink-8EF8";    // your network SSID (name)
char pass[] = "hzzaq63354";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

//char server[] = "www.google.com";    // name address for server (using DNS)
IPAddress server(192,168,0,11);  // numeric IP for server (no DNS)
#define SERVER_PORT 8082
WiFiClient client;

/*
    BNO055 Orientation Sensor Connection Setting
    ===========
    Connect SCL to analog 5
    Connect SDA to analog 4
    Connect VDD to 3-5V DC
    Connect GROUND to common ground
*/
/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (1000)
Adafruit_BNO055 bno = Adafruit_BNO055(55);

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
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
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
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  // you're connected now, so print out the status:
  printWiFiStatus();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, SERVER_PORT)) {
    Serial.println("connected to server");
  }
  /* Initialise the orientation sensor */
  Serial.println("Initialise the orientation sensor..."); Serial.println("");  
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  delay(1000);
  /* Display some basic information on this sensor */
  //displaySensorDetails();
  /* Optional: Display current status */
  //displaySensorStatus();
  bno.setExtCrystalUse(true);
}

/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete (your own code
    should go here)
*/
/**************************************************************************/
void loop(void)
{
  
  while (!client.connected()) {
    Serial.println();
    Serial.print("Attempting to connect to Server...");
    // if you get a connection, report back via serial:
    if (client.connect(server, SERVER_PORT)) {
      Serial.println("connected to server");
    }
    delay(1500);
  };

  /* Get a new sensor event */
  sensors_event_t event;
  bno.getEvent(&event);
  char* payload;

  /* Wrap the floating point sensor data in Json format 
  {"records":[
      {"value":
        {"x": "1", "y": "23", "z": "312"}
      }
    ]
  } 
  */
  StaticJsonBuffer<100> jsonBufferData;
  JsonObject& data  = jsonBufferData.createObject();
  data["x"] = event.orientation.x;
  data["y"] = event.orientation.y;
  data["z"] = event.orientation.z;
  StaticJsonBuffer<150> jsonBufferValues;
  JsonObject& values = jsonBufferValues.createObject();
  values["value"] = data;
  StaticJsonBuffer<200> jsonBufferRecords;
  JsonObject& root  = jsonBufferRecords.createObject();
  JsonArray& records = root.createNestedArray("records");
  records.add(values);
  
  // Sending a POST request to server:
  // Send headers
  client.println("POST /topics/i-rotation HTTP/1.1");
  client.println("Host: http://192.168.0.11:8082");
  client.println("Accept: application/vnd.kafka.v2+json");
  client.println("Connection: close\r\nContent-Type: application/vnd.kafka.json.v2+json");
  client.print("Content-Length: ");
  client.println(root.measureLength());
  // Terminate headers
  client.println();

  // Send body
  root.printTo(client);
  // Terminate Body
  client.println();

  // Print out sent data
  Serial.println();
  Serial.println("Sensor data is sent");
  root.prettyPrintTo(Serial);

  // Close connection to refresh 
  client.stop();
  //WiFi.end();

  /* Wait the specified delay before requesting next data */
  delay(BNO055_SAMPLERATE_DELAY_MS);

  //Re-connect WiFi in case of closing Wifi (optional)
  //WiFi.begin(ssid, pass);

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

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
void displaySensorDetails(void)
{
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

/**************************************************************************/
/*
    Display some basic info about the sensor status
*/
/**************************************************************************/
void displaySensorStatus(void)
{
  /* Get the system status values (mostly for debugging purposes) */
  uint8_t system_status, self_test_results, system_error;
  system_status = self_test_results = system_error = 0;
  bno.getSystemStatus(&system_status, &self_test_results, &system_error);
  /* Display the results in the Serial Monitor */
  Serial.println("");
  Serial.print("System Status: 0x");
  Serial.println(system_status, HEX);
  Serial.print("Self Test:     0x");
  Serial.println(self_test_results, HEX);
  Serial.print("System Error:  0x");
  Serial.println(system_error, HEX);
  Serial.println("");
  delay(500);
}