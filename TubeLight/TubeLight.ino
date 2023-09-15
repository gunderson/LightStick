
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#include <WiFi.h>
#include <WiFiClient.h>
// #include <WebServer.h>
#include <ESPmDNS.h>
// #include <Update.h>
#include <esp_wifi.h>

#include <ArduinoJson.h>

#include <FastLED.h>

//int LED_BUILTIN = 2;

// FastLED
#define DATA_PIN 21
#define CLOCK_PIN 22
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    70
#define BRIGHTNESS    200

// OSC
const unsigned int localPort = 57121;        // local port to listen for UDP packets (here's where we send the packets)
// const unsigned int outPort = 57120;          // remote port (not needed for receive)
OSCErrorCode error;
//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP Udp;

const char* masterHost = "displayhost";

IPAddress masterHostIp;// = IPAddress(192 | 168 << 8 | 0 << 16 | 16 << 11);
int mdnsServiceQueryInterval = 2000;
int mdnsServiceQueryTick = 0;

String dns = "light-";
byte mac[6];
const char* ssid = "G7";
const char* password = "1234qwerasdfzxcv";
IPAddress myIP;

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

// WebServer server(80);

// Define the array of leds
CRGB leds[NUM_LEDS];


/* ---------------------------------------------------------------------------------- */

void setup(void) {
  setupSerial();
  setupFastLED();
  connectToWifi();
  setupOSC();
//  setupWebserver();
}

void setupSerial(){
  Serial.begin(921600);
  Serial.println();
  Serial.println("Booting Sketch...");
  Serial.println(masterHostIp);
}

void setupFastLED(){
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  floodColor(CRGB::Blue);
}

void connectToWifi(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps (WIFI_PS_NONE);
  // WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  WiFi.macAddress(mac);
  dns += String(mac[1],HEX) + String(mac[0],HEX);
  Serial.println("MDNS: "+ dns);
  printmac();
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    const char* dnsCopy = dns.c_str();
    MDNS.begin(dnsCopy);
    MDNS.addService("http", "tcp", 80);


    Serial.printf("Ready! Open http://%s.local in your browser\n", dns);
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());    
  } else {
    Serial.println("WiFi Failed");
  }
}

void printmac(){
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

/* ---------------------------------------------------------------------------------- */

long lastMillis = 0;
int frameDuration = 5;

void loop(void) {
  long thisMillis = millis();
  


  if (thisMillis - lastMillis >= frameDuration){
    // Serial.println("frameloop");
    // Serial.println(thisMillis - lastMillis)
    // server.handleClient();
    loopMDNS();
    loopOSC();
    // delay(12);
    lastMillis = thisMillis;
  }
}

long mdnsQueryTime = millis();
bool isScanning = true;

void loopMDNS(){
  if (!isScanning) return;
  long now = millis();
  long mdnsQueryTimeDelta = now - mdnsQueryTime;
  if (mdnsQueryTimeDelta >= mdnsServiceQueryInterval){
    mdnsQueryTime = now;
    if (!masterHostIp){
      Serial.print("Querying Master Host: ");
      Serial.println(masterHost);
      floodColor(CRGB::Yellow);
      masterHostIp = MDNS.queryHost(masterHost);  
      if (masterHostIp){
        Serial.print("Master Host: ");
        Serial.println(masterHost);
        Serial.print("  ip: ");
        Serial.println(masterHostIp);
        floodColor(CRGB::Green);
        isScanning = false;
        
        // MDNS.end();
        registerMaster();
      }
    } else {
      floodColor(CRGB::Magenta);
      isScanning = false;
      //registerMaster();

    }
  }
}

/* ---------------------------------------------------------------------------------- */

void registerMaster(){
    Serial.print("connecting to ");
    Serial.println(masterHost);
    Serial.println(masterHostIp);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 3000;
    if (!client.connect(masterHostIp, httpPort)) {
        Serial.print("connection failed");
        Serial.println(masterHostIp);
        return;
    }

    // We now create a URI for the request
    String url = "/register";
    url += "?address=";
    url += WiFi.localIP().toString();
    url += "&port=";
    url += localPort;
    url += "&numleds=";
    url += NUM_LEDS;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + dns + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
    
  Udp.begin(localPort);
}

/* ---------------------------------------------------------------------------------- */

void floodColor(CRGB color){
  for( int j = 0; j < NUM_LEDS; j++) {
      leds[j] = color;
    }
    FastLED.show();
}

/* ---------------------------------------------------------------------------------- */

void setupOSC(){

}


void loopOSC(){
    OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    // Serial.println("Got Message");
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {

      msg.route("/led", led);
      msg.route("/draw", draw);
    } else {
      error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}

int lastLEDTime = millis();

void led(OSCMessage &msg, int addrOffset) {
  // Serial.println("led");
  // if it's my address
  boolean isBlob = msg.isBlob(0);
  int blobLength = msg.getBlobLength(0);
  int blobCounter = blobLength;
  uint8_t blob[blobLength];
  while(blobCounter--){
    int index = blobLength - blobCounter - 1;
    msg.getBlob(index, &blob[index]);
//    Serial.printf("Line %d: %d\n", index, blob[index]);
  }



//  floodColor(CRGB(blob[0],blob[1],blob[2]));

  if (blobLength / 3 != NUM_LEDS){
    Serial.printf("LED count mismatch: %d / %d \n", blobLength / 3, NUM_LEDS);
  }

  for( int i = 0; i < blobLength; i += 3) {
    leds[i/3] = CRGB(blob[i],blob[i+1],blob[i+2]);
    // Serial.printf("color: %d, %d, %d \n",blob[i], blob[i+1], blob[i+2]);
  }
  
  // int ledDelta  = millis() - lastLEDTime;
  // Serial.print("LED Delta:");
  // Serial.println(ledDelta);
  // lastLEDTime = millis();
  
  FastLED.show();
  
}

int lastDrawTime = millis();

void draw(OSCMessage &msg, int addrOffset) {
  // Serial.println("draw");

  int drawDelta  = millis() - lastDrawTime;
  // Serial.print("Draw Delta:");
  // Serial.println(drawDelta);
  FastLED.show();
  lastDrawTime = millis();
}

void latch(OSCMessage &msg, int addrOffset) {
  
}

/* ---------------------------------------------------------------------------------- */

// void setupWebserver(){
//   server.on("/", HTTP_GET, []() {
//     server.sendHeader("Connection", "close");
//     server.send(200, "text/html", serverIndex);
//   });


//   server.on("/flood", HTTP_GET, [](){
//     server.sendHeader("Connection", "close");
//     if(server.hasArg("color")){
//       Serial.printf("Color Arg: %s\n", server.arg("color"));
//       CRGB color = CRGB(server.arg("color").toInt());
//       Serial.print("color: ");
//       Serial.println(server.arg("color").toInt());
//       floodColor(color);
//       server.send(200, "text/plain", String(server.arg("color").toInt()));
//       return;
//     }
//     server.send(200, "text/plain", "FAIL");
//   });

//   server.on("/setleds", HTTP_POST, [](){
    
//   });

  
//   server.on("/update", HTTP_POST, []() {
//     server.sendHeader("Connection", "close");
//     server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
//     ESP.restart();
//   }, []() {
//     HTTPUpload& upload = server.upload();
//     if (upload.status == UPLOAD_FILE_START) {
//       Serial.setDebugOutput(true);
//       Serial.printf("Update: %s\n", upload.filename.c_str());
//       if (!Update.begin()) { //start with max available size
//         Update.printError(Serial);
//       }
//     } else if (upload.status == UPLOAD_FILE_WRITE) {
//       if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
//         Update.printError(Serial);
//       }
//     } else if (upload.status == UPLOAD_FILE_END) {
//       if (Update.end(true)) { //true to set the size to the current progress
//         Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
//       } else {
//         Update.printError(Serial);
//       }
//       Serial.setDebugOutput(false);
//     } else {
//       Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
//     }
//   });
//   server.begin();
// }

void browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("no services found");
        floodColor(CRGB::Red);
    } else {
        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");

            if (MDNS.hostname(i) == masterHost){
              masterHostIp = MDNS.IP(i);
            }
            registerMaster();
        }
    }
    Serial.println();
}

void nameFound(const char* name, IPAddress ip)
{
  if (ip != INADDR_NONE) {
    Serial.print("The IP address for '");
    Serial.print(name);
    Serial.print("' is ");
    Serial.println(ip);
  } else {
    Serial.print("Resolving '");
    Serial.print(name);
    Serial.println("' timed out.");
  }
}
