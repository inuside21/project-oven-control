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
int wifiConnected = 0;                          
int wifiStatus = 2; // 0 - manual
                    // 1 - wifi
                    // 2 - choose

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); 
long nextClear = 0;

// Current
//Adafruit_ADS1115 ads;
const float FACTOR = 3.2; //20A/1V from teh CT
const float multiplier = 0.00005;
float sCurrent = 0;

// TEMP / HUMI
#define DHTPIN 13            //what pin we're connected to
#define DHTTYPE DHT21       //DHT 21  (AM2301)
DHT dht(DHTPIN, DHTTYPE);   //Initialize DHT sensor for normal 16mhz Arduino
float sHumi = 0;  //Stores humidity value
float sTemp = 0; //Stores temperature value

// Control - Out
int pinLock = 14;
int pinFan = 32;
int pinHeater = 33;
int pinLedGreen = 27;
int pinLedRed = 25;
int pinLedOrange = 26;

// Button
int isSetup = 0;  // 0 - main display
                  // 1 - timer setting
int btnUp = 15;
int btnDown = 2;
int btnOK = 4;
int btnLock = 16;

// Data
String dStatus = "0";
String dLock = "0";
String dCurrent = "0";
String dHumi = "0";
String dTemp = "0";
String dTimer = "00:00:00";
String dTimerMain = "00:00:00";
int manualTimer = -1; // -1   idle
                      // 0    complete
                      // >0   running

// debounce
unsigned long lastTime = 0;
unsigned long lastTimeBtn = 0;
unsigned long lastTimeManualRunning = 0;
unsigned long timerDelay = 1000;
unsigned long timerDelay2 = 500;


// ============================
// Start
// ============================
void setup() 
{
  // Serial
  Serial.begin(9600);

  // init lcd
  lcd.init(); //initialize the lcd
  lcd.backlight(); //open the backlight 
  lcd.clear();

  // init current
  //ads.setGain(GAIN_FOUR);      // +/- 1.024V 1bit = 0.5mV
  //ads.begin();

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
  
  // init Button
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnOK, INPUT_PULLUP);
  pinMode(btnLock, INPUT_PULLUP);
}



// ============================
// Loop
// ============================
void loop() 
{
  // Get Current
  //float voltage = ads.readADC_Differential_0_1() * multiplier; // ads.readADC_Differential_0_1 readADC_Differential_2_3
  sCurrent = 0; //voltage * FACTOR;

  // Get Temp
  sHumi = dht.readHumidity();
  sTemp= dht.readTemperature();

  // Button
  int btnUpVal = digitalRead(btnUp);
  int btnDownVal = digitalRead(btnDown);
  int btnOKVal = digitalRead(btnOK);
  int btnLockVal = digitalRead(btnLock);

  // AUTO
  if (wifiStatus == 1)
  {
    //
    if (wifiConnected == 0)
    {
      ConnectWifi();
    }

    //
    if ((millis() - lastTime) > timerDelay) 
    {
      // Connected
      if(WiFi.status()== WL_CONNECTED)
      {
        // Set Data
        RequestSetData();
        RequestSetTimer();

        // Get Data
        RequestGetStatus();
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
          lcd.print("       STATUS       "); 
          lcd.setCursor(0, 1);        
          lcd.print("        IDLE        "); 
          lcd.setCursor(0, 2);        
          lcd.print("                    "); 
          lcd.setCursor(0, 3);        
          lcd.print("     Via  Phone     "); 

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
          lcd.print("   " + dTemp + "c  " + dHumi + "%   "); // 00.00
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
      }

      // Disconnected
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

      // Next
      lastTime = millis();
    }
  }

  // MANUAL
  if (wifiStatus == 0)
  {
    // Main Display
    if (isSetup == 0)
    {
      // Btn?
      if ((millis() - lastTimeBtn) > timerDelay2) 
      {
        if (!btnOKVal)
        {
          isSetup = 1;

          // Next
          lastTimeBtn = millis();
          return;
        }

        if (!btnLockVal)
        {
          if (manualTimer > 0)
          {
            return;
          }

          if (dLock == "0")
          {
            dLock = "1";
            lastTimeBtn = millis();
            return;
          }

          if (dLock == "1")
          {
            dLock = "0";
            lastTimeBtn = millis();
            return;
          }
        }
      }

      // running
      if (manualTimer > 0)
      {
        if ((millis() - lastTimeManualRunning) > timerDelay) 
        {
          ConvertTime(manualTimer);

          lcd.setCursor(0, 0);      
          lcd.print("   " + String(sTemp) + "c  " + String(sHumi) + "%   "); 
          lcd.setCursor(0, 1);        
          lcd.print("      " + dTimer + "      "); 
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

          //
          manualTimer = manualTimer - 1;
          lastTimeManualRunning = millis();
        }
      }

      // complete
      else if (manualTimer == 0)
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

      // idle
      else 
      {
        lcd.setCursor(0, 0);        
        lcd.print("       STATUS       "); 
        lcd.setCursor(0, 1);        
        lcd.print("        IDLE        "); 
        lcd.setCursor(0, 2);        
        lcd.print("                    "); 
        lcd.setCursor(0, 3);        
        lcd.print("       Manual       "); 

        // Out
        digitalWrite(pinFan, LOW);
        digitalWrite(pinHeater, LOW);
        digitalWrite(pinLedGreen, LOW);
        digitalWrite(pinLedRed, LOW);
        digitalWrite(pinLedOrange, HIGH);
      }
    }

    // Timer Display
    else
    {
      // Btn?
      if ((millis() - lastTimeBtn) > timerDelay2) 
      {
        if (!btnUpVal)
        {
          manualTimer = manualTimer + 60;

          // Next
          lastTimeBtn = millis();
          return;
        }

        if (!btnDownVal)
        {
          manualTimer = manualTimer - 60;
          if (manualTimer <= 0)
          {
            manualTimer = 0;
          }

          // Next
          lastTimeBtn = millis();
          return;
        }

        if (!btnOKVal)
        {
          isSetup = 0;

          // Next
          lastTimeBtn = millis();
          return;
        }
      }

      ConvertTime(manualTimer);

      lcd.setCursor(0, 0);        
      lcd.print("     SET  TIMER     "); 
      lcd.setCursor(0, 1);        
      lcd.print("      " + dTimer + "      "); 
      lcd.setCursor(0, 2);        
      lcd.print("   UP - Increase    "); 
      lcd.setCursor(0, 3);        
      lcd.print("  Down - Decrease   "); 
    }
  }

  // CHOOSE
  if (wifiStatus == 2)
  {
    lcd.setCursor(0, 0);        
    lcd.print("       WELCOME      "); 
    lcd.setCursor(0, 1);        
    lcd.print("    CHOOSE  MODE    "); 
    lcd.setCursor(0, 2);        
    lcd.print("    UP - Manual     "); 
    lcd.setCursor(0, 3);        
    lcd.print("    Down - Phone    "); 

    // Btn?
    if ((millis() - lastTimeBtn) > timerDelay) 
    {
      if (!btnUpVal)
      {
        wifiStatus = 0;

        // Next
        lastTimeBtn = millis();
        return;
      }

      if (!btnDownVal)
      {
        wifiStatus = 1;

        // Next
        lastTimeBtn = millis();
        return;
      }
    }
  }

  // Lock?
  {
    if (dLock == "0")
    {
      digitalWrite(pinLock, LOW);
    }

    if (dLock == "1")
    {
      digitalWrite(pinLock, HIGH);
    }
  }
}

void ConnectWifi()
{
  lcd.setCursor(0, 0);        
  lcd.print("                    "); 
  lcd.setCursor(0, 1);        
  lcd.print("        WIFI        "); 
  lcd.setCursor(0, 2);        
  lcd.print("     CONNECTING     "); 
  lcd.setCursor(0, 3);        
  lcd.print("                    "); 

  Serial.println("init wifi");
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

  wifiConnected = 1;
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

// Set Data
void RequestSetData()
{
  /*
    current
    humi
    temp
  */
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=setdata&id=" + deviceName + "&val1=" + String(sCurrent) + "&val2=" + String(sHumi) + "&val3=" + String(sTemp);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}

// Set Timer
void RequestSetTimer()
{
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=settimer&id=" + deviceName;
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
}

//
void ConvertTime(int timeRemaining)
{
  if (timeRemaining <= 0)
  {
    timeRemaining = 0;
  }

  int hours = timeRemaining / 3600;
  int minutes = (timeRemaining % 3600) / 60;
  int seconds = timeRemaining % 60;

  String sHrs = String(hours, DEC);
  String sMin = String(minutes, DEC);
  String sSec = String(seconds, DEC);

  if (hours < 10) sHrs = "0" + sHrs;
  if (minutes < 10) sMin = "0" + sMin;
  if (seconds < 10) sSec = "0" + sSec;

  dTimer = sHrs + ":" + sMin + ":" + sSec;
}