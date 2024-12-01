#include <SoftwareSerial.h>
#include "HX711.h"
#include <ESP32Servo.h>
#include <WiFi.h>
#include <TridentTD_LineNotify.h>

float calibration_factor = 324241.00;
#define zero_factor 8120485
#define DOUT  23
#define CLK   22
#define RXD1 (18) // กำหนดขา RX ของ Serial1 เป็นขา 25
#define TXD1 (19)
#define WIFI_SSID "NENE"
#define WIFI_PASSWORD "Nene12345"
#define LINE_TOKEN  "CHQssV6BqSlmomZcaqrOSvcRRQ3jATAVxGyMG4FTALe"   // TOKEN

Servo myservo; //ประกาศตัวแปรแทน Servo
HX711 scale(DOUT, CLK);
float offset=0;
int get_units_kg();
int mode = 0, start = 0, grams = 0;
int data;
bool foodtank = false;
void addWeight();
void stopWeight();

int sen_DO = 13;
int valsen_Analog = 0;
int valsen_Digital = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  delay(1000);

  myservo.attach(26);
  
  scale.set_scale(calibration_factor); 
  scale.set_offset(zero_factor); 

  pinMode(sen_DO, INPUT);

  // Connect to Wi-Fi
  connectToWiFi();

  LINE.setToken(LINE_TOKEN);
  LINE.notify("Hello");

}

String s = "";
int state = 0;

void loop() {
  // put your main code here, to run repeatedly:

  if (Serial.available()) {
    Serial.println("Serial available");
    char c = Serial.read();
    Serial1.write(c); // ส่งข้อมูลไป ESP32 B
  }

  if(!Serial1.available()){
    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  }


  while(Serial1.available()) {
    //Serial.println("Serial1 available");
    char c = Serial1.read();
    Serial.println(c);
    if(c != '\n'){
      if(mode == 0){
      if(c == '&'){
        if(start){
          Serial.println(s.toInt());
          grams = s.toInt();
          Serial.print(grams);
          mode = 2;
          s = "";
        }
        start = !start;
      }
      else if(start){
        s += c;
        continue;
      }
      else{
        if(c == 1 && !foodtank){
          LINE.notify("The Food tank is low! Please fill the tank.");
          foodtank = true;
        }
        else if(c == 0){
          foodtank = false;
        }
        mode = 1;
      }
    }
    if(mode == 1){
      //-------------------------READING SENSOR MODE---------------------
      Serial.print("MODE 1   ");
      Serial.println(mode);
      int sum=0;

      for(int j=0; j<50; j++){

        valsen_Digital = digitalRead(sen_DO);
        sum += valsen_Digital;

        Serial.print("  ค่าสัญญาณ Digital = ");
        Serial.println(valsen_Digital);

        if(valsen_Digital != 0){
          Serial.println("              ตรวจพบแรงสั่นสะเทือน");
        }

        delay(100);
      }

      String sent = "&";
      int readWeight = 0;
      if(sum >= 5){
        Serial.println("EAT EAT EAT");
        sent += '1';
      }
      else{
        Serial.println("SLEEP SLEEP");
        sent += '0';
      }
      sent += '@';
      readWeight = max(0,get_units_kg());
      sent += String(readWeight);
      sent += '&';

      Serial.println(sent);

      //SEND DATA TO ANOTHER BOARD
        Serial1.flush();
        for(int i=0; i <sent.length(); i++){
          Serial1.write(sent[i]);
          delay(100);
        }

      //CLEAR
      mode = 0;
      delay(3000);

    }
    else if(mode == 2){
      //-------------------------FEEDING SENSOR MODE---------------------
      Serial.print("MODE 2   ");
      Serial.println(mode);

      while(mode==2){
        if(grams != 0){
        //delay(1000);
        data = max(0,get_units_kg());

        int state = 0;

        if((grams) > data){ state = 1; }
        else{ state = 2; }

        Serial.println(state);

        switch(state){
          case 0:
          break;
          case 1:
            addWeight();
          break;
          case 2:
            stopWeight();

            //SEND DATA TO ANOTHER BOARD
            String sentout = '/' + String(data) + '/';
            
            Serial1.flush();
            for(int i=0; i<sentout.length(); i++){
              Serial1.write(sentout[i]);  
            }
            
            //CLEAR
            mode = 0;
          break;
        }
        }

      }
    }
    }
  }

  delay(100);
}

int get_units_kg()
{
  delay(500);
  float kg = scale.get_units()*0.453592+offset;
  return ( kg*100 ) - 4;
}

void addWeight(){
  myservo.write(90);
  //Serial.print("< Reading: ");
  Serial.print(data);
  Serial.println(" g");
  
}

void stopWeight(){
  myservo.write(0);
  Serial.print(">= Reading: ");
  Serial.print(data);
  Serial.println(" g Correct!");
  grams = 0;
  
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi!");
}

