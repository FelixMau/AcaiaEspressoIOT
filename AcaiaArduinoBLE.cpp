/*
  AcaiaArduinoBLE.cpp - Library for connecting to 
  an Acaia Scale using the ArduinoBLE library.
  Created by Tate Mazer, December 13, 2023.
  Released into the public domain.

  Adding Generic Scale Support, Pio Baettig
*/
#include "Arduino.h"
#include "AcaiaArduinoBLE.h"
#include <ArduinoBLE.h>

byte IDENTIFY[20]               = { 0xef, 0xdd, 0x0b, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x9a, 0x6d };
byte HEARTBEAT[7]               = { 0xef, 0xdd, 0x00, 0x02, 0x00, 0x02, 0x00 };
byte NOTIFICATION_REQUEST[14]   = { 0xef, 0xdd, 0x0c, 0x09, 0x00, 0x01, 0x01, 0x02, 0x02, 0x05, 0x03, 0x04, 0x15, 0x06 };
byte START_TIMER[7]             = { 0xef, 0xdd, 0x0d, 0x00, 0x00, 0x00, 0x00 };
byte STOP_TIMER[7]              = { 0xef, 0xdd, 0x0d, 0x00, 0x02, 0x00, 0x02 };
byte RESET_TIMER[7]             = { 0xef, 0xdd, 0x0d, 0x00, 0x01, 0x00, 0x01 };
byte TARE_ACAIA[6]              = { 0xef, 0xdd, 0x04, 0x00, 0x00, 0x00 };
byte TARE_GENERIC[1]            = { 0x54 };

AcaiaArduinoBLE::AcaiaArduinoBLE(){
    _currentWeight = 0;
    _connected = false;
}

bool AcaiaArduinoBLE::init(String mac){
    unsigned long start = millis();

    if (mac == ""){
        BLE.scan();
    }else if (!BLE.scanForAddress(mac)){
        Serial.print("Failed to find ");
        Serial.println(mac);
        return false;
    }
    
    do{
        BLEDevice peripheral = BLE.available();
        if (peripheral && isScaleName(peripheral.localName())) {
            BLE.stopScan();

            Serial.println("Connecting ...");
            if (peripheral.connect()) {
                Serial.println("Connected");
            } else {
                Serial.println("Failed to connect!");
                return false;
            }

            Serial.println("Discovering attributes ...");
            if (peripheral.discoverAttributes()) {
                Serial.println("Attributes discovered");;
            } else {
                Serial.println("Attribute discovery failed!");
                peripheral.disconnect();
                return false;
            }

            // Determine type of scale
            if(peripheral.characteristic(READ_CHAR_OLD_VERSION).canSubscribe()){
                Serial.println("Old version Acaia Detected");
                _type   = OLD;
                _write  = peripheral.characteristic(WRITE_CHAR_OLD_VERSION);
                _read   = peripheral.characteristic(READ_CHAR_OLD_VERSION);
            }else if(peripheral.characteristic(READ_CHAR_NEW_VERSION).canSubscribe()){
                Serial.println("New version Acaia Detected");
                _type   = NEW;
                _write  = peripheral.characteristic(WRITE_CHAR_NEW_VERSION);
                _read   = peripheral.characteristic(READ_CHAR_NEW_VERSION);
            } else if(peripheral.characteristic(READ_CHAR_GENERIC).canSubscribe()){
                Serial.println("Generic Scale Detected");
                _type   = GENERIC;
	            _write  = peripheral.characteristic(WRITE_CHAR_GENERIC);
	            _read   = peripheral.characteristic(READ_CHAR_GENERIC);
            }
            else{
                Serial.println("unable to determine scale type");
                return false;
            }

            if(!_read.canSubscribe()){
                Serial.println("unable to subscribe to READ");
                return false;
            }else if(!_read.subscribe()){
                Serial.println("subscription failed");
                return false;
            }else {
                Serial.println("subscribed!");
            }
        
            if(_write.writeValue(IDENTIFY, 20)){
                Serial.println("identify write successful");
            }else{
                Serial.println("identify write failed");
                return false; 
            }
            if(_write.writeValue(NOTIFICATION_REQUEST,14)){
                Serial.println("notification request write successful");
            }else{
                Serial.println("notification request write failed");
                return false; 
            }
            _connected = true;
            return true;
        }
    }while(millis() - start < 10000);

    Serial.println("failed to find scale");
    return false;    
}

bool AcaiaArduinoBLE::tare(){
    if(_write.writeValue((_type == GENERIC ? TARE_GENERIC : TARE_ACAIA), 20)){
          Serial.println("tare write successful");
          return true;
    }else{
        _connected = false;
        Serial.println("tare write failed");
        return false;
    }
}

bool AcaiaArduinoBLE::startTimer(){
    if(_write.writeValue(START_TIMER, 7)){
          Serial.println("start timer write successful");
          return true;
    }else{
        _connected = false;
        Serial.println("start timer write failed");
        return false;
    }
}

bool AcaiaArduinoBLE::stopTimer(){
    if(_write.writeValue(STOP_TIMER, 7)){
          Serial.println("stop timer write successful");
          return true;
    }else{
        _connected = false;
        Serial.println("stop timer write failed");
        return false;
    }
}

bool AcaiaArduinoBLE::resetTimer(){
    if(_write.writeValue(RESET_TIMER, 7)){
          Serial.println("reset timer write successful");
          return true;
    }else{
        _connected = false;
        Serial.println("reset timer write failed");
        return false;
    }
}

bool AcaiaArduinoBLE::heartbeat(){
    if(_write.writeValue(HEARTBEAT, 7)){
        _lastHeartBeat = millis();
        return true;
    }else{
        _connected = false;
        return false;
    }
}
float AcaiaArduinoBLE::getWeight(){
    return _currentWeight;
}

bool AcaiaArduinoBLE::heartbeatRequired(){
    if(_type == OLD || NEW){
        return (millis() - _lastHeartBeat) > HEARTBEAT_PERIOD_MS;
    } else {
    return 0;
    }
}
bool AcaiaArduinoBLE::isConnected(){
    return _connected;
}
bool AcaiaArduinoBLE::newWeightAvailable(){
    byte input[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
    if(NEW == _type 
      && _read.valueUpdated() 
      && _read.valueLength() == 13 
      && _read.readValue(input,13) 
      && input[4] == 0x05
      ){
        //Grab weight bytes (5 and 6) 
        // apply scaling based on the unit byte (9)
        // get sign byte (10)
        _currentWeight = (((input[6] & 0xff) << 8) + (input[5] & 0xff)) 
                        / pow(10,input[9])
                        * ((input[10] & 0x02) ? -1 : 1);
        return true;
    }else if(OLD == _type 
      && _read.valueUpdated() 
      && _read.valueLength() == 10
      && _read.readValue(input,10) 
      ){
        //Grab weight bytes (2 and 3),
        // apply scaling based on the unit byte (6)
        // get sign byte (7)
        _currentWeight = (((input[3] & 0xff) << 8) + (input[2] & 0xff)) 
                        / pow(10, input[6]) 
                        * ((input[7] & 0x02) ? -1 : 1);
        return true;
    }else if(GENERIC == _type 
      && _read.valueUpdated() 
      && _read.readValue(input,13) 
      ){
        //Grab weight bytes (3-8),
        // get sign byte (2)
	    _currentWeight = ( input[2] == 0x2B ? 1  : -1 )
        *(
          (input[3] -0x30)*1000 
        + (input[4] -0x30)*100 
        + (input[5] -0x30)*10 
        + (input[6] -0x30)*1 
        + (input[7] -0x30)*0.1 
        + (input[8] -0x30)*0.01
        );
        return true;
    }else{
        return false;
    }
}
bool AcaiaArduinoBLE::isScaleName(String name){
    String nameShort = name.substring(0,5);

    return nameShort == "CINCO"
        || nameShort == "ACAIA"
        || nameShort == "PYXIS"
        || nameShort == "LUNAR"
        || nameShort == "PROCH"
        || nameShort == "FELIC";
}