#include <ESP8266WiFi.h>
#include <WiFiClient.h>
 
// WiFi information
const char WIFI_SSID[] = "Karsh";//"AtomicFi";
const char WIFI_PSK[] = "Karsh246";
 
// Remote site information
const char http_site[] = "google.com";
const int http_port = 80;
 
// Pin definitions
const int LED_PIN = LED_BUILTIN;
 
// Global variables
WiFiClient client;
 
void setup() {
  
  // Set up serial console to read web page
  Serial.begin(115200);
  Serial.print("HTTP GET Example");
  
  // Set up LED for debugging
  pinMode(LED_PIN, OUTPUT);
  
  // Connect to WiFi
  String wifi_connect = strcat("Connecting to WiFi ", WIFI_SSID);
  Serial.println(wifi_connect);
  connectWiFi();
  Serial.println("Connected to WiFi");
  
  // Attempt to connect to website
  if ( !getPage() ) {
    Serial.println("GET request failed");
  }
}
 
void loop() {
  
}
 
// Attempt to connect to WiFi
void connectWiFi() {
  
  byte led_status = 0;
  
  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);
  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  
  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
    digitalWrite(LED_PIN, led_status);
    led_status ^= 0x01;
    delay(200);
  }
  
  // Turn LED on when we are connected
  digitalWrite(LED_PIN, HIGH);
}
 
// Perform an HTTP GET request to a remote page
bool getPage() {
  Serial.println("Getting page..");
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_site, http_port) ) {
    return false;
  }
  
  // Make an HTTP GET request
  client.println("GET /index.html HTTP/1.1");
  client.print("Host: ");
  client.println(http_site);
  client.println("Connection: close");
  client.println();
  
  delay(10);
  // Read all the lines of the reply from server and print them to Serial
  Serial.println("Response:");
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  
  return true;
}
