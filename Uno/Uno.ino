#define BLYNK_PRINT Serial
// Your ESP8266 baud rate:
#define ESP8266_BAUD 9600
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <SoftwareSerial.h>

SoftwareSerial EspSerial(2, 3); // RX, TX
char auth[] = ""; //tokenID
char ssid[] = ""; //wifi ID
char pass[] = ""; //wifi password
ESP8266 wifi(&EspSerial);

//=====================가스감지====================
int gas_detector = A1;
float gas_density;
int gas_notify_value = 450;
//=====================가스감지====================

//=====================부저=======================
int melody = 12;
int speaker_uno_onoff;
//=====================부저=======================

//=====================빗물감지====================
int rain_drop_sensor = A2;
int rain_drop_value;

BLYNK_WRITE(V9){
  speaker_uno_onoff = param.asInt(); // speaker_onoff tone(12, 523, 500);
}

void bell_fun(){
  int tones = 493;
  tone(melody, tones, 500);
  delay(100);
}

int gas_detector_function(){
  gas_density = analogRead(gas_detector);
  Serial.print("Gas density: ");
  Serial.println(gas_density);
  BLYNK_READ(V15);
  Blynk.virtualWrite(V15, gas_density);
  Serial.print("speaker_uno_onoff: ");
  Serial.println(speaker_uno_onoff);
  if (gas_density >= gas_notify_value){
    Blynk.notify("유해가스가 감지되었습니다!");
    if(speaker_uno_onoff ==1)
         bell_fun();
  }
}

void rain_drop_sensor_function(){
  rain_drop_value = analogRead(rain_drop_sensor);
  Serial.print("rain_drop: ");
  Serial.println(rain_drop_value);
  BLYNK_READ(V16);
  Blynk.virtualWrite(V16, rain_drop_value);
}

void setup()
{
  Serial.begin(9600);
  delay(10);
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  Blynk.begin(auth, wifi, ssid, pass);

  pinMode(gas_detector, INPUT);
  pinMode(melody ,OUTPUT);
}

void loop()
{
  Blynk.run();
  Serial.println("========loop======");
  gas_detector_function();
  if(gas_density >= gas_notify_value){
    if(speaker_uno_onoff ==1)
         bell_fun();
  }
     rain_drop_sensor_function();
     delay(500);
}
