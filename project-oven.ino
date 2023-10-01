// Board:             DOIT ESP32 (Node ESP?)
// Documentation URL: asdasd
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_ADS1X15.h>
#include <DHT.h>

// WIFI
const char* ssid = "OVENWIFI";
const char* password = "12345678";
String serverName = "http://martorenzo.click/project/oven/server/";  // include "/"
String deviceName = "1";                                           

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); 
long nextClear = 0;

// Current
Adafruit_ADS1115 ads;
const float FACTOR = 3.2; //20A/1V from teh CT
const float multiplier = 0.00005;
float sCurrent;

// TEMP / HUMI
#define DHTPIN 25            //what pin we're connected to
#define DHTTYPE DHT21       //DHT 21  (AM2301)
DHT dht(DHTPIN, DHTTYPE);   //Initialize DHT sensor for normal 16mhz Arduino
float sHumi;  //Stores humidity value
float sTemp; //Stores temperature value

// Control - Out
int pinLock = 14;
int pinFan = 32;
int pinHeater = 33;
int pinLedGreen = 25;
int pinLedRed = 26;
int pinLedOrange = 27;

// Data
String dStatus = "0";
String dLock = "0";
String dCurrent = "0";
String dHumi = "0";
String dTemp = "0";
String dTimer = "00:00:00";
String dTimerMain = "00:00:00";

// debounce
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;


// ============================
// Start
// ============================
void setup() 
{
  // Serial
  Serial.begin(9600);

  // init wifi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // init lcd
  lcd.init(); //initialize the lcd
  lcd.backlight(); //open the backlight 
  lcd.clear();

  // init current
  ads.setGain(GAIN_FOUR);      // +/- 1.024V 1bit = 0.5mV
  ads.begin();

  // init temp
  dht.begin();

  // init Control - Out
  pinMode(pinLock, OUTPUT);
  pinMode(pinFan, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  pinMode(pinLedGreen, OUTPUT);
  pinMode(pinLedRed, OUTPUT);
  pinMode(pinLedOrange, OUTPUT);
  digitalWrite(pinLock, LOW);
  digitalWrite(pinFan, LOW);
  digitalWrite(pinHeater, LOW);
  digitalWrite(pinLedGreen, LOW);
  digitalWrite(pinLedRed, LOW);
  digitalWrite(pinLedOrange, LOW);
}



// ============================
// Loop
// ============================
void loop() 
{
  if ((millis() - lastTime) > timerDelay) 
  {
    if(WiFi.status()== WL_CONNECTED)
    {
      // Get Current
      float voltage = ads.readADC_Differential_0_1() * multiplier; // ads.readADC_Differential_0_1 readADC_Differential_2_3
      sCurrent = voltage * FACTOR;

      // Get Temp
      sHumi = dht.readHumidity();
      sTemp= dht.readTemperature();

      // Set Data
      RequestSetCurrent();
      RequestSetHumi();
      RequestSetTemp();

      // Get Data
      RequestGetLock();
      RequestGetCurrent();
      RequestGetHumi();
      RequestGetTemp();
      RequestGetTimer();
      RequestGetTimerMain();

      // Display - Idle
      if (dStatus == "0")
      {
        lcd.setCursor(0, 0);        
        lcd.print("                    "); 
        lcd.setCursor(0, 1);        
        lcd.print("       STATUS       "); 
        lcd.setCursor(0, 2);        
        lcd.print("        IDLE        "); 
        lcd.setCursor(0, 3);        
        lcd.print("                    "); 

        // Out
        digitalWrite(pinFan, LOW);
        digitalWrite(pinHeater, LOW);
        digitalWrite(pinLedGreen, LOW);
        digitalWrite(pinLedRed, LOW);
        digitalWrite(pinLedOrange, HIGH);
      }

      // Display - In use
      if (dStatus == "1")
      {
        lcd.setCursor(0, 0);        
        lcd.print("     " + dTemp + "c    " + dHumi + "%     "); 
        lcd.setCursor(0, 1);        
        lcd.print(" " + dTimer + "--" + dTimerMain + " "); 
        lcd.setCursor(0, 2);        
        lcd.print("       STATUS       "); 
        lcd.setCursor(0, 3);        
        lcd.print("      RUNNING!      "); 

        // Out
        digitalWrite(pinFan, HIGH);
        digitalWrite(pinHeater, HIGH);
        digitalWrite(pinLedGreen, HIGH);
        digitalWrite(pinLedRed, LOW);
        digitalWrite(pinLedOrange, LOW);
      }

      // Display - Complete
      if (dStatus == "2")
      {
        lcd.setCursor(0, 0);        
        lcd.print("                    "); 
        lcd.setCursor(0, 1);        
        lcd.print("       STATUS       "); 
        lcd.setCursor(0, 2);        
        lcd.print("      COMPLETE      "); 
        lcd.setCursor(0, 3);        
        lcd.print("                    "); 

        // Out
        digitalWrite(pinFan, LOW);
        digitalWrite(pinHeater, LOW);
        digitalWrite(pinLedGreen, LOW);
        digitalWrite(pinLedRed, HIGH);
        digitalWrite(pinLedOrange, LOW);
      }

      // Lock?
      if (dLock == "0")
      {
        digitalWrite(pinLock, LOW);
      }

      if (dLock == "1")
      {
        digitalWrite(pinLock, HIGH);
      }
    }
    else 
    {
        lcd.setCursor(0, 0);        
        lcd.print("                    "); 
        lcd.setCursor(0, 1);        
        lcd.print("        WIFI        "); 
        lcd.setCursor(0, 2);        
        lcd.print("     CONNECTING     "); 
        lcd.setCursor(0, 3);        
        lcd.print("                    "); 
    }

    lastTime = millis();
  }
}

// Get Status
void RequestGetStatus()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=getstatus&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dStatus = payload;
  }

  http.end();
}

// Get Lock
void RequestGetLock()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=getlock&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dLock = payload;
  }

  http.end();
}

// Get Current
void RequestGetCurrent()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=getcurrent&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dCurrent = payload;
  }

  http.end();
}

// Get Humi
void RequestGetHumi()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=gethumi&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dHumi = payload;
  }

  http.end();
}

// Get Temp
void RequestGetTemp()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=gettemp&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dTemp = payload;
  }

  http.end();
}

// Get Timer
void RequestGetTimer()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=gettimer&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dTimer = payload;
  }

  http.end();
}

// Get Timer Main
void RequestGetTimerMain()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=gettimermain&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dTimerMain = payload;
  }

  http.end();
}

// Set Current
void RequestSetCurrent()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=setcurrent&id=" + deviceName + "&val=" + String(sCurrent);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}

// Set Humi
void RequestSetHumi()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=sethumi&id=" + deviceName + "&val=" + String(sHumi);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}

// Set Temp
void RequestSetTemp()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=settemp&id=" + deviceName + "&val=" + String(sTemp);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}