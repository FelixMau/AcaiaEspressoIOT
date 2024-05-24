#include <AcaiaArduinoBLE.h>
//acaia_max = "00:1C:97:1D:38:C4"
AcaiaArduinoBLE scale;
int relaispin = 35;

// Load Wi-Fi library
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "MagentaWLAN-X85R";
const char* password = "abeRbr!nGb!eRm!T";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Bluetooth® Low Energy hardware
  BLE.begin();
  // Optionally add your Mac Address as an argument: acaia.init("##:##:##:##:##:##");
  scale.init();
  pinMode(relaispin, OUTPUT);
  
}

void loop() {
  // Send a heartbeat message to the acaia periodically to maintain connection
  if (!scale.isConnected()){
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    BLE.begin();
    // Optionally add your Mac Address as an argument: acaia.init("##:##:##:##:##:##");
    scale.init();
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (scale.heartbeatRequired()) {
    scale.heartbeat();
  }

  // always call newWeightAvailable to actually receive the datapoint from the scale,
  // otherwise getWeight() will return stale data
  if (scale.newWeightAvailable()) {
    int new_weight = scale.getWeight();
    if (new_weight>15){
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(relaispin, HIGH);
           
    }
    else{
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(relaispin, LOW);
      
    }

  }
  
}