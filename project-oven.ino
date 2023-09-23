// Board:             DOIT ESP32 (Node ESP?)
// Documentation URL: asdasd
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>;

// WIFI
const char* ssid = "OVENWIFI";
const char* password = "12345678";
String serverName = "http://martorenzo.click/project/oven/";  // include "/"
String deviceName = "1";                                           

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); 
long nextClear = 0;

// TEMP
#define DHTPIN 2            //what pin we're connected to
#define DHTTYPE DHT21       //DHT 21  (AM2301)
DHT dht(DHTPIN, DHTTYPE);   //Initialize DHT sensor for normal 16mhz Arduino
float sHumi;  //Stores humidity value
float sTemp; //Stores temperature value

// Control - Out
int pinFan = 0;
int pinHeater = 0;

// Data
String dStatus = "0";
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

  // init temp
  dht.begin();

  // init Control - Out
  pinMode(pinFan, OUTPUT);
  pinMode(pinHeater, OUTPUT);
  digitalWrite(pinFan, LOW);
  digitalWrite(pinHeater, LOW);
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
      // Get Temp
      sHumi = dht.readHumidity();
      sTemp= dht.readTemperature();

      // Set Data
      RequestSetHumi();
      RequestSetTemp();

      // Get Data
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
        lcd.print("       IN-USE       "); 

        // Out
        digitalWrite(pinFan, HIGH);
        digitalWrite(pinHeater, HIGH);
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
  String serverPath = serverName + "status.php?id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dStatus = payload;
  }

  http.end();
}

// Get Humi
void RequestGetHumi()
{
  HTTPClient http;
  String serverPath = serverName + "humi.php?id=" + deviceName;
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
  String serverPath = serverName + "temp.php?id=" + deviceName;
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
  String serverPath = serverName + "timer.php?id=" + deviceName;
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
  String serverPath = serverName + "timermain.php?id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
    dTimerMain = payload;
  }

  http.end();
}

// Set Humi
void RequestSetHumi()
{
  HTTPClient http;
  String serverPath = serverName + "humiset.php?id=" + deviceName + "&val=" + String(sHumi);
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
  String serverPath = serverName + "tempset.php?id=" + deviceName + "&val=" + String(sTemp);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}