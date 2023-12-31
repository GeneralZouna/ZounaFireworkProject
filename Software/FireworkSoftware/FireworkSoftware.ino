#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#define DEBUG

/*
Pins for connection
*/
#define DATA 11
#define CLK 12
#define LATCH 8
#define R_PIN 9

#define FLIPPED_OUTPUTS //pins are NC

/*
Set true if every board is flipped by pins:
Q7 is first LSB; Q1 is MSB
*/
#define FLIPPED_PORTS

/*
Number of pins that may be on at one time
WARNING:Bigger number solows down code
*/
#define MAX_ON_PINS 50

//On time for each output in ms
int ON_TIME = 500;  

//number of outputs
#define OUTPUTS 16


#ifndef APSSID
#define APSSID "Firework Control"
#define APPSK "" //password"
#endif

/*
Get data from controller
Read SD
Update outputs
*/

int TimerArr_time[MAX_ON_PINS]; //array for storing ticks before switching off
int TimerArr_pin[MAX_ON_PINS];  //array for storing what pin is connected to a timer

int PortLength = OUTPUTS;
unsigned long old_time;

bool ProgramActive  = false;

const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);
 /*
  *   Input commands:
  *
  *   ports: <int>      Sets number of outputs
  *   mode: <byte/flag> chooses mode Program/Manual/Random
  *   stop              turns all outputs to 0, stops execution of programs
  *   start             starts the program
  *   on_time           sets ON_TIME
  *  
  *
  */



  /*
   * The connection of shift registers and pins is:
   * arduino  -> shift register  ->    shift register  ->  ...
   * pins:       1|2|3|4|5|6|7|8 -> 9|10|11|12|13|14|15|16
   */
void SetPinOn(int pin,int onTime = ON_TIME){
 if (pin == 0) {return;}

 for (int i = 0; i < MAX_ON_PINS; i++) {
    /*
    *  If you find pin reset timer,
    *  else find empty spot and update it
    */
    if (TimerArr_pin[i] == pin){
     TimerArr_time[i] = onTime;
#ifdef DEBUG
      Serial.print("Updated on-time for pin ");
      Serial.println(pin);
#endif

     return;
    }
    else if (TimerArr_pin[i] == 0) {
      TimerArr_time[i] = onTime;
      TimerArr_pin[i] = pin;
#ifdef DEBUG
      Serial.print("Updated on-time for pin ");
      Serial.println(pin);
#endif
      return;
    }
  }
}



bool TestOutput(int pin) {
  // pin 0 doesn't exist --> code for empty
  if (pin == 0) {
    return false;
  }

  //Find pin and timer; if it's time is pass update it
  for (int i = 0; i < MAX_ON_PINS; i++) {

    /*
    *  If you find pin updte it
    *  If time has elapsed turn it off
    */

    if (TimerArr_pin[i] == pin) {
      return true;
    }
  }

  return false;
}

void updateTimers(int deltaTime = (int)(millis() - old_time)){
  old_time = millis();
  for (int i = 0; i < MAX_ON_PINS; i++) {
    if (TimerArr_pin[i]) {
      TimerArr_time[i] -= deltaTime;
      if (TimerArr_time[i] <= 0){
        TimerArr_time[i] = 0;
        TimerArr_pin[i] = 0;
      }
    }
  }
}

void updateOutputs() {
  unsigned char outputbyte;
  unsigned char addBit;
  int8_t byteCounter;
  updateTimers();
  //digitalWrite(R_PIN,HIGH);
  //digitalWrite(R_PIN,LOW);
 //loop thru all the pins
  for (int i = PortLength; i >0; i--) {
    byteCounter ++;
    addBit = TestOutput(i) ? 1 : 0;
    outputbyte = (outputbyte << 1) + addBit;

    
    if ( (byteCounter) % 8 == 0) {
     byteCounter = 0;
#ifdef DEBUG
    Serial.print(" ");
    Serial.print(i);
    Serial.print(": ");
#endif
     
#ifdef FLIPPED_OUTPUTS
    outputbyte = outputbyte ^ 255;
#endif

#ifdef FLIPPED_PORTS
      shiftOut(DATA, CLK, LSBFIRST, outputbyte);
#else
      shiftOut(DATA, CLK, MSBFIRST, outputbyte);
#endif

    Serial.print(" -> ");
    Serial.print((byte)outputbyte, BIN);
    Serial.print(" <- ");
      outputbyte = 0;
    }
#ifdef DEBUG
    Serial.print(addBit, BIN);
#endif
  }
 #ifdef DEBUG
    Serial.println("");
#endif
  digitalWrite(LATCH,HIGH);
  digitalWrite(LATCH,LOW);
}


void setup() {
  Serial.begin(115200);
  pinMode(DATA,OUTPUT);
  pinMode(CLK,OUTPUT);
  pinMode(LATCH,OUTPUT);
  pinMode(R_PIN, OUTPUT);

  digitalWrite(R_PIN,HIGH);
  updateOutputs();
 
 //wifi Setup
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if(ProgramActive){Start_launch();}
  server.handleClient();
  updateOutputs();
  delay(50);
}
