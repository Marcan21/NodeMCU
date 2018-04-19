/* PROJET DE FIN D'ÉTUDES - GPA 793
 *  INTERNET OF THINGS
 *  FAIT PAR 
 *      PHILLIPE DUBOIS DUBP19089203,
 *      MARC-ANTOINE DUMAS DUMM18079205,
 *      SAMUEL KONG KONS15099304, 
 *      FRANCIS ROUSSEAU ROUF23049304,
 *  REMIS A
 *      TONY WONG 
 *  SESSION AUTOMNE 2017  
 *  REMIS LE 22 DECEMBRE 2017
 * */
#include "FS.h"
#include <EthernetClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "DHT.h"
#include "ThingSpeak.h"
extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
/*
// Possible improvement : Timer interrupt

// Maybe TODO bouton de sélection de mode, soit qui recherche le wifi 
// automatiquement, soit qu'il faut appuyer sur un bouton pour faire la
// connexion qui fait un update de thingspeak et donc beaucoup moins longtemps a ON (mode 2)

// Warning : Time values do not work without sleep between loops.  
*/

Adafruit_7segment matrix = Adafruit_7segment();

//Initialisation des parametres 
struct RtcData {
uint32_t elapsedTime;
uint32_t lastUpdateTime;    
uint32_t lastAcquisitionTime;
uint32_t lastConnectionTime;
uint32_t acquiredDataCount; 
uint32_t sent;
uint32_t toBeSent;
uint32_t inFile;
uint32_t sleepingTimeValue;
uint32_t acquisitionTimeInterval;
uint32_t opMode;
} ;

RtcData rtcData;

// Pour le wifi
char SSID[] = "iPhone de Philippe";
char pass[] = "Projet1!!";
//char SSID[] = "PFE"; // Wifi de sam
//char pass[] = "Projet1!";
//char SSID[] = "LEEEEROOOOY";
//char pass[] = "Dumas323";
int Wifi_Status = WL_IDLE_STATUS;
WiFiClient client;

const int analogIn = A0;
int analogValue = 0;
const int interruptFreqSelectPin = 12; // GPIO12 -- D6
const int displayEnable = 13; // GPIO2 -- D7 
bool freqSelectInterruptDetected = false;
unsigned int opModeBtnFreq;

// Pour le wifi manager
const int interruptWiFiModePin = 14; // GPIO14 -- D5
bool wifiModeIsAP = 0;
bool defaultWifi = 0;

// Pour ThingSpeak
char TS_WKEY[] = "0FI7KCPNJT5OHNQA";
unsigned long TS_CHNUMBER = 332349;
const char * server = "api.thingspeak.com"; //"https://api.thingspeak.com/channels/332349/bulk_update.json"; // ThingSpeak Server

// Pour le DHT11
//#define DHT_PIN 2; //GPIO02 // D4
#define DHT_TYPE DHT11
DHT dht(2, DHT_TYPE);

// Pour le transfert des données
#define T_WAIT 3000
//unsigned int acquisitionTimeInterval = 20000;  //TODO +10 a 15 seconde variable MAYBE add interrupt   200000
//TODO variable deepsleep time according to next acquisition time 
//TODO dont wake up without any value to send in economy mode
const unsigned long updateInterval = 20L * 1000L; // Update once every 15 seconds

char jsonBuffer[5000] = "["; //TODO change max size to max available 

bool dontSendDataToTS = false; // true;
bool delaySendData = false; // false;
unsigned const int delaySendTestConst = 3;

// Pour les parametres AP 
String frequenceAcq = "";
String frequenceDS = "";
String frequenceTS = ""; 
unsigned int newDS = 1;
unsigned int newAcq = 1;

/********************************************************
 *  Fonction Setup                                      *
 *  Fait l'initialisation du programme                  *
 ********************************************************/
void setup() {

  Serial.begin(9600);
  noInterrupts();
  //Serial.setDebugOutput(true);
  
  #ifndef __AVR_ATtiny85__
    Serial.println("7 Segment Backpack Test");
  #endif
  matrix.begin(0x70);
  //matrix.setBright  
  
  Serial.println("*********************************************************");
  
  pinMode(interruptWiFiModePin, INPUT);
  pinMode(interruptFreqSelectPin, INPUT);
  pinMode(displayEnable, OUTPUT);
  detachInterrupt(interruptWiFiModePin);
  detachInterrupt(interruptFreqSelectPin);
  
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData));
  rst_info *rsti;
  rsti = ESP.getResetInfoPtr();
  
  //Serial.println(ESP.getResetReason());
  if (rsti->reason == 6)  // rsti reason 6 --> External System (Re-upload) / Powerup / rst button
  {                       // rsti reason 5 --> Deep-Sleep Wake
    Serial.println("PROGRAM START") ;
    //WiFi.begin(SSID,pass);
    defaultWifi= false;
    rtcData.inFile = 0;
    rtcData.elapsedTime = 0;
    rtcData.lastUpdateTime = 0;
    rtcData.lastAcquisitionTime = 0;
    rtcData.lastConnectionTime = 0;
    rtcData.acquiredDataCount = 0; 
    rtcData.sent = 0;
    rtcData.toBeSent = 0;
    rtcData.sleepingTimeValue = 2000000; 
    rtcData.acquisitionTimeInterval = 3600000;
    rtcData.opMode = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
    bool fs = SPIFFS.begin();
    //SPIFFS.format();
    if(!fs) {
      SPIFFS.format();  
      fs = SPIFFS.begin();
    }
    if(fs) {
      sizeFile();
      Serial.println("DATA IN FILE AT START: " + String(rtcData.inFile));
      rtcData.acquiredDataCount =  rtcData.inFile;      
    }
    SPIFFS.end();   
    //WiFiManager wifiManager;
    //wifiManager.resetSettings(); // TODO
    //doSettingsReset = true;
  }
  else if (rsti->reason == 5) { // rsti reason 5 --> Deep-Sleep Wake
    
    float timeValue = 0;
    
    /*if (rtcData.opMode == (uint32_t)1) { // other buttons could be other opModes
      noInterrupts();
      rtcData.opMode = (uint32_t)0;
      rtcData.acquisitionTimeInterval = (uint32_t)(60 * timeValue);
      ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
      ESP.deepSleep(100000);  
    }
    else */
    if (rtcData.opMode == (uint32_t)2) { 
      
      bool buttonDetected = false;
      bool buttons2Detected = false;
      
      Serial.println("UX Menu");
      noInterrupts();
      //pinMode(displayEnable, OUTPUT);
      //delay(100);
      timeValue = rtcData.acquisitionTimeInterval / 60000.0; //TODO
      matrix.print(timeValue);
      matrix.writeDisplay();
      digitalWrite(displayEnable, HIGH); // TODO rename displayEnable to analogLevelEnable
      delay(500);
      matrix.print(timeValue);
      matrix.writeDisplay();
      matrix.blinkRate(1);
      delay(1000);
      if (digitalRead(interruptFreqSelectPin)){
        buttonDetected = true;
        if (digitalRead(interruptWiFiModePin)) {
          buttons2Detected = true;
        }
      }
      else delay(1000);
      if (digitalRead(interruptFreqSelectPin)) {
        buttonDetected = true;
        if (digitalRead(interruptWiFiModePin)) {
          buttons2Detected = true;
        }
      }
      else delay(1000);
      if (digitalRead(interruptFreqSelectPin)) {
        buttonDetected = true;
        if (digitalRead(interruptWiFiModePin)) {
          buttons2Detected = true;
        }
      }
      else delay(1000);  
      if ((digitalRead(interruptWiFiModePin) && digitalRead(interruptFreqSelectPin)) || buttons2Detected) {  // RESET MODE
            
        Serial.println("RESET MODE") ;
        matrix.print(20000);
        matrix.writeDisplay();
        matrix.blinkRate(1);
        //WiFi.begin(SSID,pass);
        defaultWifi= false;
        rtcData.inFile = 0;
        rtcData.elapsedTime = 0;
        rtcData.lastUpdateTime = 0;
        rtcData.lastAcquisitionTime = 0;
        rtcData.lastConnectionTime = 0;
        rtcData.acquiredDataCount = 0; 
        rtcData.sent = 0;
        rtcData.toBeSent = 0;
        rtcData.sleepingTimeValue = 10000000; 
        rtcData.acquisitionTimeInterval = 20000;
        rtcData.opMode = 0;
        ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
        SPIFFS.format();  
        matrix.blinkRate(0);

        // TODO https://arduino.stackexchange.com/questions/1477/reset-an-arduino-uno-in-code reset chip
      }
      else if (digitalRead(interruptFreqSelectPin) || buttonDetected) {
          
        float timeValue = 0;
        float lastValue = 0;
        unsigned int timerStart = millis();
    
        matrix.blinkRate(2);
        //pinMode(displayEnable, OUTPUT);
        //delay(100);
        digitalWrite(displayEnable, HIGH);
        timerStart = millis();
        while ((millis() - timerStart) < 8000) // 5 seconds without significant value change
        {
          delay(400);
          timeValue = getAnalogValue();
          if (abs(lastValue-timeValue) > 2)
            timerStart = millis();
          if (abs(lastValue-timeValue) < 1.3)
            timeValue = lastValue;
          lastValue = timeValue;
          matrix.print(timeValue);
          matrix.writeDisplay();
        }
        //matrix.drawColon(true); 
        rtcData.acquisitionTimeInterval = (uint32_t)(60000 * timeValue);
      
        rtcData.opMode = (uint32_t)0;
        ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
        ESP.deepSleep(100000);  
      }
      matrix.blinkRate(0);
    }
  }
  matrix.print(rtcData.acquisitionTimeInterval / 60000.0);
  matrix.writeDisplay();
}

float getAnalogValue() {
  
  float timeValue = 0;
  float scaled = 0;
  analogValue = analogRead(analogIn);
  if (analogValue > 1002.0) analogValue = 1002.0;
  scaled = (analogValue - 5.0)/1002.0 ;
  if (scaled <= 0) scaled = 0.01; 
  else if (scaled > 1) scaled = 1;
  timeValue = 100.49 - (99.99 * scaled);
  Serial.println(analogValue); // range 5 -- 785
  Serial.println(timeValue);
  return timeValue; 
}

/* ******************************************************
 *  Fonction Loop                                       *
 *  Programme principale                                *
 ********************************************************/
void loop() {

  attachInterrupt(digitalPinToInterrupt(interruptWiFiModePin), wifiModeInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptFreqSelectPin), freqSelectInterrupt, RISING);
  interrupts();
  //noInterrupts();
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData));
  
  if(wifiModeIsAP) {
    noInterrupts();
    matrix.clear();
    //matrix.writeDigitRaw(1, 8); // A
    //matrix.writeDigitRaw(2, 16); // P
    
    matrix.writeDigitRaw(3, 119); // A
    matrix.writeDigitRaw(4, 115); // P
    matrix.writeDisplay();
    
    WiFiManager wifiManager;
    String newSSID = "nouveauSSID";
    String newPW=  "nouveauPass";
    
    wifiManager.resetSettings();
    wifiManager.setConfigPortalTimeout(60);
    WiFiManagerParameter freqAcq("freqAcq","frequence acquisition (s)","", 40); 
    WiFiManagerParameter freqDS("freqDS","frequence deep sleep (s)","", 40); 
    wifiManager.addParameter(&freqAcq);
    wifiManager.addParameter(&freqDS);
    
    wifiManager.startConfigPortal("PFE IoT");
    Serial.println("Captive portal started");
    
    wifiModeIsAP = false;
    newSSID = wifiManager.getConfigPortalSSID();
    newPW = wifiManager.getConfigPortalPW();

    SSID[newSSID.length()+1];
    strcpy(SSID,newSSID.c_str());
    pass[newPW.length()+1];
    strcpy(pass,newPW.c_str());

    frequenceAcq = freqAcq.getValue();
    frequenceDS = freqDS.getValue();
    Serial.println("GG MARC parameter ACQ : " + frequenceAcq +" DS : " + frequenceDS);
    //interrupts();
    
    if (frequenceDS != "") {
      newDS = frequenceDS.toInt();
      Serial.println("new DEEP SLEEP TIME " + String(newDS));
      rtcData.sleepingTimeValue = newDS * 1000000;
      Serial.println("new sleeping time value " + String(rtcData.sleepingTimeValue));
    }
    if (frequenceAcq != ""){
      newAcq = frequenceAcq.toInt();
      Serial.println("new ACQ " + String(newAcq));
      rtcData.acquisitionTimeInterval = newAcq * 1000;
      Serial.println("new acquisition time " + String(rtcData.acquisitionTimeInterval));
    }   
    ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));  
    matrix.print(rtcData.acquisitionTimeInterval / 60000.0);
    matrix.writeDisplay();
  }
  else {
    noInterrupts();
    readFile();
     
    if (((getElapsedTime()) - rtcData.lastAcquisitionTime > rtcData.acquisitionTimeInterval) 
      || rtcData.lastAcquisitionTime == 0) {
        //if (rtcData.inFile < 125)
            dhtAcquire(); // use FAKEdhtAcquire() for le board pas de capteur
        //else 
         //   Serial.println("File is full");
    }
    Serial.println("Acq :");
    Serial.println(rtcData.acquiredDataCount);  
    // If update time has reached 15 seconds, then update the jsonBuffer
    if (((getElapsedTime() - rtcData.lastUpdateTime >=  updateInterval) && 
       ((rtcData.acquiredDataCount > 0 && !delaySendData) || (rtcData.acquiredDataCount >= delaySendTestConst))) 
       && (rtcData.acquiredDataCount - rtcData.sent) > 0){  
      sendAcquiredData();
    }
    rtcData.opMode = (uint32_t)0;
    rtcData.elapsedTime = getElapsedTime() + rtcData.sleepingTimeValue/1000; //TODO Handle Elapsed Time without sleep 
    ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
    //interrupts();
    interrupts();
    
    if (!wifiModeIsAP) {
      Serial.print("DeepSleep ");
      Serial.print(rtcData.sleepingTimeValue); 
      Serial.print(" microsecondes");
      if (rtcData.sleepingTimeValue != 0)
      {
        matrix.clear();
        matrix.writeDisplay();
        //pinMode(displayEnable, OUTPUT);
        ESP.deepSleep(rtcData.sleepingTimeValue);  
      }
    else
      Serial.print("GG FOOL PROOF - NO MORE MAGIC");
    }
    // don't put anything after deepSleep
  }
}

unsigned int getElapsedTime() {
  
  return rtcData.elapsedTime + millis();
}

//*******************************************************//
//Wifi manager AP mode
//https://github.com/tzapu/WiFiManager
// WiFiManager - WiFi Connection manager with web captive portal. 
// If it can’t connect, it starts AP mode and a configuration portal 
// so you can choose and enter WiFi credentials.
//*******************************************************//

/* ******************************************************
 *  Fonction Wifi Mode Interrupt                        *
 *  Interruption, programme active le mode AP           *
 ********************************************************/
void wifiModeInterrupt() {
  Serial.println("wifiModeInterrupt!!!!!!!!!!");
  wifiModeIsAP = true;
  //rtcData.opMode = (uint32_t)3;
}

void freqSelectInterrupt() {
  Serial.println("freqSelectInterrupt!!!!!!!!!!");
  bool buttonHeld = false;
  rtcData.opMode = (uint32_t)2;
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
  ESP.deepSleep(1000);  
}

//*******************************************************//
// dhtAcquire
// http://www.esp8266.com/viewtopic.php?f=32&t=8114&p=41933#p41933
// read sensor value and write the value in the temp.txt
// file from the SPIFFS filesystem.
//*******************************************************//

/* Fonction dhtAcquire 
 *    Fait l'acquisition de la temperature et l'humidite 
 */
void dhtAcquire() {   
  
  int retry_limit = 3;
  float t = 0;  
  float h = 0; 
  do
  {
    Serial.println("Démarrer le module DHT...");
    dht.begin();
    delay(150);
    t = dht.readTemperature();  
    h = dht.readHumidity(); 
    if (isnan(h) || isnan(t)){
      --retry_limit; 
      Serial.println("Problème dans la lecture du DHT!");
      Serial.println(t);
      Serial.println(h);
    }
    else {
      writeData(t, h);  
      rtcData.lastAcquisitionTime = getElapsedTime();
    }
  } while ((isnan(h) && (retry_limit > 0)) || (isnan(t)) && (retry_limit > 0));
}

/*  fonction Fake DHT acquire
 *    pour les tests 
 */
void FAKEdhtAcquire() {
  
  Serial.println("FAKEdhtAcquire");
  float t = rtcData.acquiredDataCount;  
  float h = 1.00;
  writeData(t, h);  
  rtcData.lastAcquisitionTime = getElapsedTime();
}

//*******************************************************//
// writeData
// append the new data to the temp.txt file in the
// SPIFFS filesystem
//*******************************************************//

/* Fonction Write Data
 *  SPIFFS 
 *  max data : 125 
 */
void writeData(float t, float h) {

  int Tdelta = (getElapsedTime()-rtcData.lastAcquisitionTime)/1000; 
  Serial.print("Last Acquisition : ");
  Serial.println(rtcData.lastAcquisitionTime);
  Serial.print("elapsed  : ");
  Serial.println(getElapsedTime());
  Serial.print("Tdelta :");
  Serial.println(Tdelta);
  noInterrupts();
  SPIFFS.begin();
  // TODO Check out the max number 4,294,967,295 in uint32(4 bytes), so maybe we have to divide by 1000. 
  File f = SPIFFS.open("temp.txt", "a+");
  //f.println(String(t) + "|" + String(Tdelta));
  f.println(String(t) + "|" + String(h) + "|" + String(Tdelta)); //---------------------------------------------
  f.close();
  sizeFile(); 
  Serial.println("DATA IN FILE : " + String(rtcData.inFile));
  rtcData.acquiredDataCount = rtcData.acquiredDataCount + (uint32_t)1; 
  SPIFFS.end();
  interrupts();
}

//*******************************************************//
// sendAcquiredData
// WORK IN PROGRESS reads all accumulated data and sends it to Thingspeak
// https://www.mathworks.com/help/thingspeak/continuously-collect-data-and-bulk-update-a-thingspeak-channel-using-an-arduino-mkr1000-board-or-an-esp8266-board.html
//*******************************************************//

void sendAcquiredData() {
  
  connectWifi();
  int n = 0;
  while (rtcData.inFile > 0 && WiFi.status() == WL_CONNECTED ) //-------------------------------------------------
  { 
    Serial.println("SENDING..............");
    if (n>0)
      delay(updateInterval);    
    Serial.println("Before Upload----------" + String(rtcData.inFile));
    // TODO Handle maximum amount of data (json size limit of 3000 view thingspeak specs)
    updateJson(jsonBuffer);
    httpRequest(jsonBuffer);
    
    sizeFile(); 

    Serial.println("After Upload----------" + String(rtcData.inFile));
    n++;
  } 
  if (WiFi.status() != WL_CONNECTED)
     Serial.println("LOST CONNECTION ----------------------------------------------------");
}

// Updates the jsonBuffer with data



void updateJson(char* jsonBuffer){
  
  noInterrupts();
  SPIFFS.begin();

  size_t len = 0;
  rtcData.toBeSent = 0;
  Serial.print("Elapsed time : ");
  Serial.println(getElapsedTime());
  
  File f = SPIFFS.open("temp.txt", "r");
  String lastDeltaT = "";
  String dataAcquisitionTime  = "";   
  String timeDeltaT = "";
  int totalTime = 0; 
  while (f.available() && rtcData.toBeSent < 5) // TODO check max TS message size 3000 ??
  {
    String str = f.readStringUntil('\n');   // TODO replace all String objects with char arrays    
    int delimiter = str.indexOf('|');
    String field1Temperature = str.substring(0, delimiter);

    int secondDelimiter = str.indexOf('|',delimiter+1);
    String field2Humidity = str.substring(delimiter+1,secondDelimiter);
      
    if (lastDeltaT=="")
      dataAcquisitionTime = "0";
    else 
      dataAcquisitionTime = lastDeltaT;

    int lastDelimiter = str.lastIndexOf('|');
    lastDeltaT = str.substring(lastDelimiter+1);  
    
    strcat(jsonBuffer,"{\"delta_t\":");
    unsigned long deltaT = dataAcquisitionTime.toInt();
    if (dataAcquisitionTime.toInt() > getElapsedTime())
      deltaT = 0;
    size_t lengthT = String(deltaT).length();
    char temp[4];
    String(deltaT).toCharArray(temp,lengthT+1);
    strcat(jsonBuffer,temp);
    strcat(jsonBuffer,",");
    strcat(jsonBuffer, "\"field1\":");
    lengthT = String(field1Temperature).length();
    String(field1Temperature).toCharArray(temp,lengthT+1);
    strcat(jsonBuffer,temp);
    strcat(jsonBuffer,",");
    strcat(jsonBuffer, "\"field2\":");
    lengthT = String(field2Humidity).length();
    String(field2Humidity).toCharArray(temp,lengthT+1);
    strcat(jsonBuffer,temp);
    strcat(jsonBuffer,"},");
    len = strlen(jsonBuffer);
    jsonBuffer[len-1] = ',';
    rtcData.lastUpdateTime = getElapsedTime(); // Update the last update time (a expliquer pk)
    rtcData.toBeSent += 1;
  }
  strcat(jsonBuffer,"{\"delta_t\":");


  while (f.available()){
    String timeStr = f.readStringUntil('\n'); 
    int timeDelimiter = timeStr.lastIndexOf('|');
    timeDeltaT = timeStr.substring(timeDelimiter+1);          
    totalTime = totalTime+timeDeltaT.toInt();   
  }
  
  
  lastDeltaT = totalTime;
  Serial.print("Total Time is : ");
  Serial.println(totalTime);
  size_t lengthDT = String(lastDeltaT).length();
  char tempo[8];
  String(lastDeltaT).toCharArray(tempo,lengthDT+1);
  strcat(jsonBuffer,tempo);
  strcat(jsonBuffer,"}]");
  f.close(); 
  
  SPIFFS.end();  
  interrupts();
}

// Updates the ThingSpeakchannel with data
unsigned int httpRequest(char* jsonBuffer) {
 
  char data[500] = "{\"write_api_key\":\"0FI7KCPNJT5OHNQA\",\"updates\":";
  strcat(data,jsonBuffer);
  strcat(data,"}");
  client.stop();  // Close any connection before sending a new request
  String data_length = String(strlen(data)+1); //Compute the data buffer length
  // POST data to ThingSpeak
  if(dontSendDataToTS)  //Dev tool
  {
    Serial.println(data);
  }
  else if (client.connect(server, 80)) {
    client.println("POST /channels/332349/bulk_update.json HTTP/1.1"); 
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: "+data_length);
    client.println();
    client.println(data);
    
    Serial.print("data : ");
    Serial.println(data);
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); //Wait to receive the response
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Response code:"+resp); // Print the response code. 202 indicates that the server has accepted the response
  jsonBuffer[0] = '['; // Reinitialize the jsonBuffer for next batch of data
  jsonBuffer[1] = '\0';
  if (resp == "202" || dontSendDataToTS) // Envoi à TS avec succès //Dev tool dontSendDataToTS might be removed for release...
  {
    rtcData.sent = rtcData.sent + rtcData.toBeSent;
    Serial.println("Sent : " + String(rtcData.sent));  
    rtcData.lastConnectionTime = getElapsedTime(); // Update the last connection time
    
    editFile();
  }
  else {
    rtcData.toBeSent = 0;
  }
}


void readFile() {

  noInterrupts();
  SPIFFS.begin();

  File f = SPIFFS.open("temp.txt", "r+");
  String str = f.readString();
  Serial.println("File temp");
  Serial.println(str);
  Serial.println("Done");
  f.close();
  
  SPIFFS.end();
  interrupts();
}

void editFile() {

  readFile();
  noInterrupts();
  SPIFFS.begin();

  File ff = SPIFFS.open("temp.txt", "r+");
  while (rtcData.toBeSent > 0)
  {
    ff.readStringUntil('\n');
    rtcData.toBeSent -= 1;
  }
  
  File tf = SPIFFS.open("temporary.txt", "w+");
  while (ff.available()){
    String S = ff.readStringUntil('\n');
    tf.println(String(S));  
  }
  tf.close();
  
  File tff = SPIFFS.open("temporary.txt", "r+");

  tff.close();
  ff.close();
  SPIFFS.remove("temp.txt");
  SPIFFS.rename("temporary.txt", "temp.txt");

  File fileend = SPIFFS.open("temp.txt", "r+");
  String t_end = fileend.readString();
    
  Serial.println("Deleting line");  
  Serial.println(t_end);
  Serial.println("Done");

  fileend.close();
  
  SPIFFS.end();
  interrupts();
}

//*******************************************************//
// printWiFiStatus
// https://github.com/bportaluri/WiFiEsp/blob/master/examples/WebClientSSL/WebClientSSL.ino
//
//*******************************************************//
void printWiFiStatus() {
  // Print the SSID of the network you're attached to:
  // TODO check what happens if printWifiStatus is called and wifi is not connected
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  // Print your device IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//*******************************************************//
// connectWifi
// Status check of wifi connection and connection established
// if possible with given SSID and password
//*******************************************************//
void connectWifi() {
  
  if(defaultWifi){
    Serial.println("DEFAULT WIFI");
      WiFi.begin(SSID,pass);
      defaultWifi=false;
  }
  else
      Serial.println("NEW WIFI");

  WiFi.status();
  unsigned int attempts = 0;

  while (WiFi.status() != WL_CONNECTED and attempts <1 ) {
    delay(10000);
    attempts = ++attempts;
    Serial.println("Tentative : " + String(attempts));
  
    switch (WiFi.status()) {
    case 0:
      Serial.println("WiFi Status: IDLE ");
      break;
    case 1:
      Serial.println("WiFi Status: NO_SSID_AVAIL");
      break; 
    case 2:
      Serial.println("WiFi Status: SCAN_COMPLETED");
      break; 
    case 3:
     Serial.println("WiFi Status: CONNECTED ");
     break;
    case 4:
     Serial.println("WiFi Status: CONNECT_FAILED");
     break; 
    case 5:
     Serial.println("WiFi Status: CONNECTION_LOST");
     break; 
    case 6:
     Serial.println("WiFi Status: DISCONNECTED"); 
    break;
   }
  }
  printWiFiStatus(); 
}

void sizeFile(){
  
  rtcData.inFile = 0;
  noInterrupts();
  SPIFFS.begin();  
  File initf = SPIFFS.open("temp.txt", "r");
  while (initf.available()){
    initf.readStringUntil('\n');
    rtcData.inFile = rtcData.inFile+1;
  }
  initf.close();
      
  SPIFFS.end();
  interrupts();
}
