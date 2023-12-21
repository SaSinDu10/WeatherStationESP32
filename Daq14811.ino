//////////////Weather Station//////////////////
///////////// s14811 //////////////////////////


#include "WiFi.h"         //Library add for WiFi connection
#include <HTTPClient.h>
#include "time.h"
#include "DHT.h"          //Library add for DHT11 sensor
#include "ThingSpeak.h"   //Library add for ThingSpeak service
#include "SPIFFS.h"     
#include <vector>        

#include <Wire.h>              //Library add for BMP180 Sensor
#include <Adafruit_BMP085.h>  

#define PIN_DHT 4
#define PIN_MODE_DHT DHT11 
#define PIN_LDR 33             // define LDR pin is GPIO33

//Deep Sleep ESP32 for 60 seconds
#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  60 
       
RTC_DATA_ATTR int bootCount = 0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

//WIFI details ssid & password
const char * ssid = "Dialog 4G 640";
const char * password = "1d90cB6c";

//IFTTT server connection
String server = "http://maker.ifttt.com";
String eventName = "s14811_DAQ_WeatherStation";
String IFTTT_Key = "H-RtTi-Zyflg7UL-Nqp4f";
String IFTTTUrl="https://maker.ifttt.com/trigger/{sensor_data}/json/with/key/H-RtTi-Zyflg7UL-Nqp4f";

//ThingSpeak channel ID and API key
unsigned long chanelId = 1874165;
const char *rightAPIkey = "9YS1LMH6JYV6NYGC";
WiFiClient client;

// Google script ID and required 
String GOOGLE_SCRIPT_ID = "AKfycbwNEDXXE8xbOujzaZEj9nJjcAPhKUj3ywCQswzN7QvMzBRRNNQ5p1VLlUyNxKo99UtA";    // change Gscript ID
using namespace std;  //"SPIFFS.h library
int count = 0;

DHT dht11(PIN_DHT,PIN_MODE_DHT);

Adafruit_BMP085 bmp; 

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused -->  RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused --> RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused --> timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused --> touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused --> ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void listAllFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}


void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin(true);
  File tempfile;
  File humidityfile;


  char firstVariable = 0;
  char secondVariable = 0;
  char thirdVariable = 0;
  char timeVariable=0;
  char StartupEmptypoint;
  char number = 80; 
  char WifiConnectWaitingTime=20;
  String temparray[number];
  String humidarray[number];

  delay(1000);
  dht11.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if(!bmp.begin()){
    while(1){}
    }

  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);

    float dhtHumidity = dht11.readHumidity();
    float dhtTemperature = dht11.readTemperature();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
   thirdVariable++;
      if (thirdVariable > WifiConnectWaitingTime) { 
        Serial.println("\ndata recording Offline...");
        if(!(SPIFFS.exists("/tempc.txt")) && !(SPIFFS.exists("/humidity.txt"))){
          File tempfile = SPIFFS.open("/tempc.txt", FILE_WRITE);
          File humidityfile = SPIFFS.open("/humidity.txt", FILE_WRITE);
  
          Serial.println("file write in write mode");
          tempfile.println(String(dhtTemperature));
          humidityfile.println(String(dhtHumidity));

          tempfile.close();
          humidityfile.close();  
          }else{
            File tempfilea = SPIFFS.open("/tempc.txt","a" );
            File humidityfilea = SPIFFS.open("/humidity.txt", "a");
            tempfilea.println(String(dhtTemperature));// log temperature to tempfile
            humidityfilea.println(String(dhtHumidity));//log humidity to humidityfile
            Serial.println("file write in append mode");
            tempfilea.close();
            humidityfilea.close();
          }
      thirdVariable = 0;
      break;
      }
    }
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //ESP32 wakeup reason
  print_wakeup_reason();

  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  ThingSpeak.begin(client);

}

void loop() {
   delay(2000);
    float dhtHumidity = dht11.readHumidity();
    float dhtTemperature = dht11.readTemperature();
    float bmpPressure = bmp.readPressure();
     char firstVariable = 0;
      char secondVariable = 0;
      char thirdVariable = 0;
      char timevariable=0;
      char StartupEmptypoint;
      char number = 100; 
      char WifiConnectWaitingTime=25;
      String temparray[number];
      String humidarray[number];

   //check the any reads fail to read, then try again
   if(isnan(dhtHumidity) || isnan(dhtTemperature)){
   Serial.println(F("Failed read DHT11 sensor!!!"));
  return;
    }

   Serial.print(F("Humidity: "));
   Serial.println(dhtHumidity);
   Serial.print(F("Temperature: "));
   Serial.print(dhtTemperature);
   Serial.println(F("°C "));

  
    Serial.print("Pressure = ");
    Serial.print(bmpPressure);
    Serial.println(" Pa");

    //LDR sensor data
    int analogValueLDR = analogRead(PIN_LDR);
    Serial.print("Analog value of LDR = ");
    Serial.println(analogValueLDR);

    //calibrated equation for analog to lux converter for LDR. EQ; y' = 0.0047x^2 - 6.4653x + 2238.8;
  float luxvalue = (0.0047*(analogValueLDR*analogValueLDR))-(6.4653*analogValueLDR)+2238.8;
    Serial.println(luxvalue);
    

    ThingSpeak.setField(1,dhtTemperature);
    ThingSpeak.setField(2,dhtHumidity);
    ThingSpeak.setField(3,bmpPressure);

    int respons = ThingSpeak.writeFields(chanelId,rightAPIkey);

     if (respons == 200) {
        Serial.println("Data upload sucessful");

     } else {
         Serial.println("Data upload unsucessful");

     }
   
      
      if (WiFi.status() == WL_CONNECTED) {
         
        if ((SPIFFS.exists("/tempc.txt")) && (SPIFFS.exists("/humidity.txt"))) {
         
          File tempfile1 = SPIFFS.open("/tempc.txt");
          File humidityfile1 = SPIFFS.open("/humidity.txt");
          vector<String> v1;
          vector<String> v2;
    
          while (tempfile1.available()) {
            v1.push_back(tempfile1.readStringUntil('\n'));
          }
    
          while (humidityfile1.available()) {
            v2.push_back(humidityfile1.readStringUntil('\n'));
          }
    
          tempfile1.close();
          humidityfile1.close();
    
          for (String s1 : v1) {
            
            temparray[firstVariable] = s1; 
            firstVariable++;
          }
    
          for (String s2 : v2) {
            
            humidarray[secondVariable] = s2;
            secondVariable++;
          }
          while (timeVariable <= number) {
            if(temparray[timeVariable]==0 && humidarray[timeVariable]==0){
              Serial.println("\nArrays are empty..");
                StartupEmptypoint=timeVariable;
                break;
              }else{
                if (WiFi.status() == WL_CONNECTED) {
                  HTTPClient http;
                  Serial.println("offline data uploading... ");
                  Serial.print("temp :");
                  Serial.print(temparray[timeVariable]);
                  Serial.print("\t");
                  Serial.print("humid :");
                  Serial.print(humidarray[timeVariable]);
                  Serial.print("\n");
                  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?"+"Temperature=" + String(temparray[t]) + "&Humidity=" + String(humidarray[t]);
                  
                  Serial.println("Making a request");
                  http.begin(url.c_str()); 
                  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                  int httpCode = http.GET();
                  String payload;
                  if (httpCode > 0) { 
                    payload = http.getString();
                    Serial.println(httpCode);
                    Serial.println(payload);
                  }
                  else {
                    Serial.println("Error on HTTP request");
                  }
                  http.end();
                }
              
              timeVariable++; 


              }
          }
          if (timeVariable == number || timeVariable==StartupEmptypoint) {
              
              listAllFiles();
              SPIFFS.remove("/tempc.txt");
              SPIFFS.remove("/humidity.txt");
           
              listAllFiles();
              firstVariable = 0;
              secondVariable = 0;
            }
        }
      else{        
        static bool flag = false;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
          return;
        }
        char timeStringBuff[50]; 
        strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
        String asString(timeStringBuff);
        asString.replace(" ", "-");
        Serial.print("Time:");
        Serial.println(asString);
        String urlFinal = "https://script.google.com/macros/s/"+ GOOGLE_SCRIPT_ID +"/exec?"+"Temperature=" + String(dhtTemperature) + "&Humidity=" + String(dhtHumidity) +"&LDR=" + String(luxvalue) + "&Pressure=" + String(bmpPressure);
        Serial.print("POST data to spreadsheet:");
        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        
        
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: "+payload);    
        }
        
          
          http.end();
    }
      }
      WiFi.disconnect();
      
      Serial.println("Disconnectiong the WiFi....");
      delay(1000);

     Serial.println("Sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
}
