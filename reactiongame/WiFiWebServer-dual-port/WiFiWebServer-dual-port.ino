#include <Arduino.h>

/*
 *  This sketch demonstrates how to set up a simple server, which
 *  listens to both port 80 (HTTP) and port 8080 at the same time.
 *  It will print the received message to the serial port, but 
 *  it will read only one port at a time.
 *  
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>

const char* ssid = "commons|lab";
const char* password = "u8eb6vhk";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server80(80);
WiFiServer server8080(8080);

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare built-in led (in off state)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the servers
  server80.begin();
  server8080.begin();
  Serial.println("Servers started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client80 = server80.available();
  WiFiClient client8080 = server8080.available();

  if (client80) {
   
    Serial.println("");
    Serial.println("*************************************");
    Serial.println("* New client connected to port 80 ! *");
    Serial.println("*************************************");
    Serial.println("");

    while (client80.status()){
      // Read until End-Of-String (NULL character)
      String req = client80.readStringUntil('\0');
      Serial.print(req);
    }

    Serial.println("");
    Serial.println("*****************************");
    Serial.println("* Client has DISCONNECTED ! *");
    Serial.println("*****************************");
    Serial.println("");
  
  }

  if (client8080) {
   
    Serial.println("");
    Serial.println("**************************************");
    Serial.println("* New client connected to port 8080! *");
    Serial.println("**************************************");
    Serial.println("");

    while (client8080.status()){
      // Read until End-Of-String (NULL character)
      String req = client8080.readStringUntil('\0');
      Serial.print(req);
    }

    Serial.println("");
    Serial.println("*****************************");
    Serial.println("* Client has DISCONNECTED ! *");
    Serial.println("*****************************");
    Serial.println("");
  
  }

}

