/**
 * BatteryDrain.ino
 *
 * Copyright (C) 2016 by CommonsLab <info@commonslab.gr>
 * 
 * Repeatedly request a web page and blink an LED in order to see how long a battery will last.
 * (Based on LGPLv2 licensed example code of github.com/esp8266/arduino.)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti MyWiFi;

void setup() {
    pinMode(D1, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

    Serial.begin(115200);

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    MyWiFi.addAP("commons|lab", "PASSWORD");

}

void loop() {
    // wait for WiFi connection
    if((MyWiFi.run() == WL_CONNECTED)) {

        digitalWrite(D1, HIGH);   // Turn the LED on 

        HTTPClient http;

        Serial.print("[HTTP] begin...\n");
        // configure traged server and url
        //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
        //http.begin("http://192.168.1.12/test.html"); //HTTP
        http.begin("http://etherpad.commonslab.gr/"); //HTTP

        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == 503) {
                String payload = http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    digitalWrite(D1, LOW);  // Turn the LED off 

    delay(1000);
}

