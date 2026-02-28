#include <SPI.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <DFRobotDFPlayerMini.h>
/*
  Self made StringBuffer object, not strictly necessary
  as I assume C++ garbage collection works fine.
*/
#include <StringBuffer.hpp>
#include "credentials.h"
/*
  Created a parse.h, just felt right
*/
#include "parse.h"

// base api queries
#define BQUERY "/api/v4/odpt:Train?odpt:operator=odpt.Operator:JR-East"
#define RAILWY "&odpt:railway=odpt.Railway:JR-East.Yamanote"
#define TRAIN  "&odpt:trainNumber=" // query will be modified after finding specific train
#define USRKEY "&acl:consumerKey="

// Will be populated once we find a train
const char *trainNumber;
// change to templates for git push
char serverDNS[] = "api-challenge.odpt.org";
IPAddress server(48, 218, 12, 77);

int status  = WL_IDLE_STATUS;
int traiNum = 0;

long lastConn        = 0;
const long postInter = 10000L;

bool jStart = false;
bool jEnd   = false;
bool train  = false;
bool empty  = false;

// need SSL to connect to HTTPS
WiFiSSLClient client;

// create stringbuffer object
StringBuffer buffer(800);

// create DFplayer object
DFRobotDFPlayerMini myDFPlayer;
/*

Arduino setup code inspired from:
https://github.com/witnessmenow/arduino-sample-api-request/blob/master/ESP32/HTTP_GET/HTTP_GET.ino
https://github.com/tuan-karma/ESP32_WiFiClientSecure/blob/main/examples/WiFiClientSecure/WiFiClientSecure.ino
https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/

*/

// For debugging only
int changes = 0;

void setup() {
  
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check wifi firmware
  String fv = WiFi.firmwareVersion();

  Serial.print("Wifi Version: ");
  Serial.println(fv);

  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // connect to wifi
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }

  printWifiStatus();

  Serial.println(F("Beginning DFplayer Startup"));
  Serial1.begin(9600);

  if (!myDFPlayer.begin(Serial1, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
  }

  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.volume(20);
}

/*
  Read data from the client and parse to play audio.
  Utilizing custom stringbuffer object which is not
  strictly necessary as testing indicates output is
  the same. More efficient? Probably not.
*/
void read_response() {

  int i = 0;
  char prev = '\0';
  
  while (client.available() && !jEnd) {
    
    // recieve data
    char c = client.read();

    // Handle if response is empty
    if (c == ']' && prev == '[') {
      Serial.println("Reseting train query");
      train = false;
      changes++;
      break;
    }

    // data reception includes http response
    if (c == '{') {
      jStart = true;
    }

    if (jStart) {
      buffer.append(c);
    }

    // only recieve first train -> data size limits
    if (c == '}') {
      jEnd = true;
    }
    prev = c;
  }
  
}

// loop and reset
void loop() {
  
  // temporary dummy ptr
  char *dummy;
  
  read_response();
  
  if (!buffer.isEmpty()) {

    dummy = buffer.toString();
    parseData(dummy, buffer.length());
    free(dummy);

    Serial.print("Number of Train Changes: ");
    Serial.println(changes);

    delay(postInter);
  }

  if (millis() - lastConn > postInter) {

    sendRequest();
    buffer.clear();
    jStart = false;
    jEnd   = false;
  }
  
}

// Send a request, modified once we find a specific train
void sendRequest() {

  client.stop();

  if (!client.connect(serverDNS, 443)) {
    // Add Wifi Reconnect?
    Serial.println("Error: Connection Failed");
  } else {
    Serial.println("Connected!");
    
    // Base GET
    client.print("GET ");
    client.print(BQUERY);
    client.print(RAILWY);

    // If we find a train, query the train
    if (train) {
      
      Serial.println("Printing with Train!");
      client.print(TRAIN);
      client.print(trainNumber);

    }

    // Private user key
    client.print(USRKEY);
    client.print(c_key);

    // GET data
    client.println(" HTTP/1.1");
    client.println("Host: api-challenge.odpt.org");
    client.println("Connection: close");
    client.println();
    
    lastConn = millis();
  }

}

// Prints Wifi Status, taken from arduino website
void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// delete this, debugging only
void printResponse(char *response, int len) {

  int i = 0;

  for (i = 0; i < len; i++) {

    Serial.print(response[i]);

    if (!(i % 80)) {
      Serial.println();
    }
  }
  Serial.println();
}
