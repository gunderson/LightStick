#include <ArduinoOSC.h>

#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 90

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 3
#define CLOCK_PIN 13



const char* masterHost = "http://displayhost.local";

const char* ssid = "G7";
const char* pwd = "1234qwerasdfzxcv";

int i; float f; String s;

void setup()
{
    // WiFi stuff
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, pwd);
     if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    MDNS.begin(host);
    setupWebserver();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    //registerMaster();
  } else {
    Serial.println("WiFi Failed");
  }

    // subscribe osc packet and directly bind to variable
    OscWiFi.subscribe(bind_port, "/bind/values", i, f, s);

    // publish osc packet in 30 times/sec (default)
    OscWiFi.publish(host, publish_port, "/publish/value", i, f, s);
    // function can also be published
    OscWiFi.publish(host, publish_port, "/publish/func", &millis, &micros)
        ->setFrameRate(1); // and publish it once per second
}

void loop()
{
    OscWiFi.update(); // should be called to subscribe + publish osc
}
