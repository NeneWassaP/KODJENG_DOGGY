#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include <SoftwareSerial.h>
#include "stdbool.h"
//#include <WiFiClientSecure.h>
//#include <TridentTD_LineNotify.h>
//#include <ArtronShop_LineNotify.h> // นำเข้าไลบารี่ ArtronShop_LineNotify

#include "cJSON.h"

#define RXD1 (18) // กำหนดขา RX ของ Serial1 เป็นขา 25
#define TXD1 (19)
#define WIFI_SSID "NENE"
#define WIFI_PASSWORD "Nene12345"
//#define LINE_TOKEN "CHQssV6BqSlmomZcaqrOSvcRRQ3jATAVxGyMG4FTALe" 

//--------config firebase
#define FIREBASE_HOST "https://kodjengdoggy-default-rtdb.asia-southeast1.firebasedatabase.app"  // Your Firebase Realtime Database URL
#define FIREBASE_AUTH "TMG9jiPMHIKFxiy12I62VuEcUYZxvL91g0vmc1x6"                                // Your Firebase Database secret or auth token

FirebaseData firebaseData;
FirebaseJson json;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

const char* path = "/Log";
FirebaseJson default_alarm_obj;
FirebaseJsonData jsonData;


//---------config time
long timezone = 7; 

byte daysavetime = 1;
struct tm tmstruct;

String nowTime, nowDate;

time_t lastFunctionCallTime = 0, dlastFunctionCallTime = 0; 
const int pauseDuration = 60, dpauseDuration = 60;

//---------sensor
const int UTpingPin = 5;
int UTinPin = 21, UTcount=0;
long UTduration, UTcm, UTmincm = 2e9;

int IRdigitalPin = 13;
int IRval = 0;

//---------reading from another esp32
int start = 0, grams = 0, past_grams=0;
String s = "";
int shake = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  pinMode(UTpingPin, OUTPUT);
  pinMode(IRdigitalPin, INPUT);
  delay(1000);

  // Connect to Wi-Fi
  connectToWiFi();

  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;  // Use legacy token

  // Initialize Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  // Set Firebase credentials
  Firebase.reconnectWiFi(true);

  // // Wait for the database to initialize
  delay(2000);

  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  //LINE.setToken(LINE_TOKEN);


}

void loop() {
  
  // if (Serial.available()) {
  //   char c = Serial.read();
  //   Serial.print(c);
  //   Serial1.write(c); // ส่งข้อมูลไป ESP32 B
  //   delay(100);
  // }
  
  willDispenseDef();
  willDispenseCus();


  while (Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c); // ส่งข้อมูลไปยัง Serial Monitor ของ ESP32 A
    
    if(c == '&'){
      if(start){
        //Serial.println(s.toInt());
        grams = s.toInt();
        s = "";

        // Serial.print("grams =");
        // Serial.println(grams);
        // Serial.print("shake =");
        // Serial.println(shake);

        // Serial.print("past grams =");
        // Serial.println(past_grams);

        if(grams != past_grams){
          Serial.println("add data");
          addData();
        }

        past_grams = grams;
        
      }
      start = !start;
    }
    else if(c == '@'){
      shake = s.toInt();  
      s = "";
    }
    else if(c == '/'){
      if(start){
        //Serial.println(s.toInt());
        grams = s.toInt();
        s = "";

        // Serial.print("grams =");
        // Serial.println(grams);
        shake = 0;
        addData();


      }
      start = !start;

    }
    else if(start){
      s += c;
      continue;
    }
   
  }

  delay(10000);

  // String test = "&50&";
  // Serial.println(test);
  // for(int i=0; i<4; i++){
  //   Serial1.write(test[i]);
  // }
  
 // -------------------ultra

  digitalWrite(UTpingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(UTpingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(UTpingPin, LOW);
  pinMode(UTinPin, INPUT);
  UTduration = pulseIn(UTinPin, HIGH);
  
  UTcm = microsecondsToCentimeters(UTduration);
  //Serial.println(UTcm);

  if(UTcount < 100){
    if(UTcm != 0){
      UTmincm = min(UTmincm, UTcm);
    }
    UTcount++;
  }
  else{
    UTcount = 0;
    UTmincm = 2e9;
  }

  //-------------------IR

  IRval = digitalRead(IRdigitalPin);
  Serial1.write(IRval);

}

void willDispenseDef() {
  time_t currentTime = time(NULL);
  if(difftime(currentTime, lastFunctionCallTime) < pauseDuration){ return; }
  // Define the Firebase path where your data is stored

  // Read data from Firebase
  if (Firebase.getJSON(firebaseData, "/Alarm_Default")) {
    FirebaseJson *json = firebaseData.jsonObjectPtr();  // Get JSON object from Firebase response
    size_t len = json->iteratorBegin();  // Get the number of keys in the JSON object
    String key, value;
    int type = 0;

    // Loop through each item in the JSON object
    for (size_t index = 0; index < len; index++) {
      yield();  // Yield to prevent watchdog timer reset

      // Get key-value pair
      json->iteratorGet(index, type, key, value);
      
      if (type == FirebaseJson::JSON_OBJECT && key.length() > 1) {
        // Process objects (index % 4 logic)
        if (index % 4 == 0) {
          // Parse the nested JSON object (value)
          cJSON *innerJson = cJSON_Parse(value.c_str());
          
          if (innerJson != NULL) {
            // Get "Time" and "Gram" from the nested JSON object
            cJSON *time = cJSON_GetObjectItemCaseSensitive(innerJson, "Time");
            cJSON *gram = cJSON_GetObjectItemCaseSensitive(innerJson, "Gram");
            cJSON *status = cJSON_GetObjectItemCaseSensitive(innerJson, "status");

            // Get the string values from the parsed cJSON
            const char *time_str = time ? time->valuestring : "";
            const char *gram_str = gram ? gram->valuestring : "";
           
            const bool def_status = status ? status->valueint : false;
            // Copy to local buffers
            char def_time[100];
            char def_gram[100];

            strncpy(def_time, time_str, sizeof(def_time) - 1);
            strncpy(def_gram, gram_str, sizeof(def_gram) - 1);



            def_time[sizeof(def_time) - 1] = '\0'; // Ensure null termination
            def_gram[sizeof(def_gram) - 1] = '\0';

            if ( def_time == localTime().substring(12, 17) && def_status) {
              String food_g = "&" + String(def_gram) + "&";
                for(int i=0; i<food_g.length(); i++){
                Serial1.write(food_g[i]); 
                Serial.print(food_g[i]);
                
              }
              Serial.println("");
              lastFunctionCallTime = currentTime;
            }
            cJSON_Delete(innerJson);
          } else {
            Serial.println("Error parsing nested JSON.");
          }
        }
      }
    }

    // End iteration and clear JSON object
    json->iteratorEnd();
    json->clear();
  } else {
    // If fetching failed, print the error reason
    Serial.print("Error fetching data: ");
    Serial.println(firebaseData.errorReason());
  }
}

void willDispenseCus() {
  time_t currentTime = time(NULL);
  if(difftime(currentTime, lastFunctionCallTime) < pauseDuration){ return; }
  // Define the Firebase path where your data is stored

  // Read data from Firebase
  if (Firebase.getJSON(firebaseData, "/Alarm_Customize")) {
    FirebaseJson *json = firebaseData.jsonObjectPtr();  // Get JSON object from Firebase response
    size_t len = json->iteratorBegin();  // Get the number of keys in the JSON object
    String key, value;
    int type = 0;

    // Loop through each item in the JSON object
    for (size_t index = 0; index < len; index++) {
      yield();  // Yield to prevent watchdog timer reset

      // Get key-value pair
      json->iteratorGet(index, type, key, value);
      
      if (type == FirebaseJson::JSON_OBJECT && key.length() > 1) {
        // Process objects (index % 4 logic)
        if (index % 5 == 0) {
          // Parse the nested JSON object (value)
          cJSON *innerJson = cJSON_Parse(value.c_str());
          
          if (innerJson != NULL) {
            // Get "Time" and "Gram" from the nested JSON object
            cJSON *time = cJSON_GetObjectItemCaseSensitive(innerJson, "Time");
            cJSON *gram = cJSON_GetObjectItemCaseSensitive(innerJson, "Gram");
            cJSON *status = cJSON_GetObjectItemCaseSensitive(innerJson, "status");
            cJSON *date = cJSON_GetObjectItemCaseSensitive(innerJson, "Date");

            // Get the string values from the parsed cJSON
            const char *time_str = time ? time->valuestring : "";
            const char *gram_str = gram ? gram->valuestring : "";
            const char *date_str = date ? date->valuestring : "";
            const bool def_status = status ? status->valueint : false;
            
            // Copy to local buffers
            char def_time[100];
            char def_gram[100];
            char def_date[100];
            strncpy(def_time, time_str, sizeof(def_time) - 1);
            strncpy(def_gram, gram_str, sizeof(def_gram) - 1);
            strncpy(def_date, date_str, sizeof(def_date) - 1);


            def_time[sizeof(def_time) - 1] = '\0'; // Ensure null termination
            def_gram[sizeof(def_gram) - 1] = '\0';
            def_date[sizeof(def_date) - 1] = '\0';

            // Compare time and print gram
            if ( def_time == localTime().substring(12, 17) && def_date == localTime().substring(0,10) && def_status) {
              String food_g = "&" + String(def_gram) + "&";
                for(int i=0; i<food_g.length(); i++){
                Serial1.write(food_g[i]);
              }
              lastFunctionCallTime = currentTime;
              // delay(20000); 
            }

            // Clean up the cJSON object
            cJSON_Delete(innerJson);
          } else {
            Serial.println("Error parsing nested JSON.");
          }
        } 
      }
    }

    // End iteration and clear JSON object
    json->iteratorEnd();
    json->clear();
  } else {
    // If fetching failed, print the error reason
    Serial.print("Error fetching data: ");
    Serial.println(firebaseData.errorReason());
  }
}

long microsecondsToCentimeters(long microseconds){ return microseconds / 29 / 2; }

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi!");
}

void getDateTime() {
  char time[30];

  char date[30];

  getLocalTime(&tmstruct, 5000);

  sprintf(time,"%02d:%02d:%02d",tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);

  sprintf(date,"%d-%02d-%02d",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday);
 
  nowTime = time;
  nowDate = date;
}


void addData(){
  getDateTime();
  
  delay(500);
  Serial.print(F("UTmincm = "));
  Serial.println(UTmincm);

  Serial.print(F("Date = "));
  Serial.println(nowDate);
  Serial.print(F("Gram = "));
  Serial.println(grams);
  Serial.print(F("Time = "));
  Serial.println(nowTime);
  Serial.print(F("isEat = "));
  Serial.println((shake && (UTmincm <= 15)));
  Serial.print(F("isEmpty = "));
  Serial.println(IRval);

  FirebaseJson data;
  data.set("Date", nowDate);
  data.set("Gram", grams);
  data.set("Time", nowTime);
  data.set("isEat" , (shake && (UTmincm <= 15))) ;
  data.set("isEmpty" , bool(IRval)) ;

  if(Firebase.pushJSON(firebaseData, path, data)) {
      Serial.println("Pushed : " + firebaseData.pushName()); 
  } else {
      Serial.println("Error : " + firebaseData.errorReason());
  }

}

String localTime(){
  char buffer[30];
  getLocalTime(&tmstruct, 5000);
  sprintf(buffer,"%d-%02d-%02d, %02d:%02d:%02d",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);

  time_t now;
  time(&now);

  return String(buffer);
}