
#include <ESP8266WiFi.h>    
#include <ESP8266WebServer.h> 
#include <BlynkSimpleEsp8266.h> 
#include <SimpleTimer.h>   //그래프 시간 헤더
#include <DHT.h> //온습도센서 라이브러리
#include <Wire.h>  //LCD I2C 지원 라이브러리
#include <LiquidCrystal_I2C.h>   // LCD I2C인터페이스 라이브러리
#include <ArduinoJson.h>  // 날씨데이터 전송 라이브러리

//====================온습도센서=========================
#define DHTPIN 2    //온습도센서 2번핀 으로 정의
#define DHTPIN_plants 13   //작물온습도센서 15번핀 으로 정의
#define DHTTYPE DHT11    // 온습도센서 11종류로 정의
DHT dht(DHTPIN, DHTTYPE); //DHT온습도센서 
DHT dht2(DHTPIN_plants, DHTTYPE); //작물 온습도 센서
SimpleTimer timer; //온습도센서 업로드 시간
SimpleTimer timer2; //온습도센서 업로드 시간
//====================온습도센서=========================

//=====================연결설정=========================
char auth [] = ""; 
// APP과 보드간에 연결 토큰
//====================================================
const char* ssid = ""; //home wifi
const char* pass = ""; //home wifi pw
//----------------------------------------------------
//char ssid [] = ""; //스마트폰 hotspot 
//char pass [] = ""; //스마트폰 hotspot pw
//----------------------------------------------------
//char ssid [] = ""; //sub wifi
//char pass [] = ""; //sub wifi pw
=======
//char pass [] = ""; //스마트폰 hotspot pw
//----------------------------------------------------
//char ssid [] = ""; //sub wifi
//char pass [] = ""; //sub wifi pw

//====================================================
String APIKEY = ""; 
//기상데이터와 아두이노 보드간 연결 코드
//=====================연결설정=========================

//=====================날씨 데이터======================
//String CityID = "1835848"; //서울
String CityID = "1845604"; // 청주
//기상데이터 원하는 곳 설정
char servername[]="api.openweathermap.org"; //날씨데이터 받는 URL위치
String result; // 날씨 문자열 선언(맑음, 흐림...)
//=====================날씨 데이터======================

//=====================미세먼지 측정======================
// 미세 먼지 없을 때 초기 V 값 0.3
#define no_dust 0.3
#define dust_balance 80

int dustout=A0;
int dust_LED=14; //미세먼지 LED핀 조절
float vo_value=0; // 센서로 읽은 값 변수 선언
float sensor_voltage = 0; // 센서로 읽은 값을 전압으로 측정 변수

float dust_density = 0; // 측정한 미세 먼지 밀도변수
float dust_density_array[20];
float dust_density_sum =0;
float dust_density_average=0;
int dust_index=0;
//=====================미세먼지 측정======================

//=====================날씨 및 LED=======================
ESP8266WebServer server(80);
WiFiClient client;

LiquidCrystal_I2C lcd(0x26,16,2); // LCD주소값, 크기 정의 , 날씨정보 출력
LiquidCrystal_I2C lcd2(0x27,16,4); //LCD, 20x4 온습도 미세먼지 출력
int lcd_counter =1; //lcd 루프카운터

int new_weatherDATA = 180; // 날씨정보 업데이트 주기
String weatherDescription =""; //날씨 상태
String weatherLocation = "";  //날씨 장소
String Country; //국가
float LCD_t; // 기상데이터 온도
float LCD_h;  //기상데이터 습도
//=====================날씨 및 LED=======================

//=====================온습도 제어========================
float House_t;
float House_h;
float plants_LCD_t; 
float plants_LCD_h;
float temp_max = 35;
float temp_min = 10;
float temp_plants_max = 35;
float temp_plants_min = 10;
//=====================온습도 제어========================

//=====================침입자 감지센서=====================
#define door_sensor_pin 16
int door_sensor_value;
int door_sensor_onoff;
int door_sensor_state = LOW;
//=====================침입자 감지센서=====================

int melody = 12;
int melody_tone = 370;
int speaker_onoff;

//=====================blynk 입출력핀 설정================
BLYNK_READ(V0){ // 가상핀V0에 LCD 첫번째줄 읽기
  Blynk.virtualWrite(V0, millis()/1000);
}
BLYNK_READ(V1){ //가상핀 V1에 LCD 두번째줄 읽기
  Blynk.virtualWrite(V1, millis());
}
BLYNK_WRITE(V2){
  temp_max = param.asInt();
}
BLYNK_WRITE(V3){
  temp_min = param.asInt();
}
BLYNK_WRITE(V9){
  speaker_onoff = param.asInt(); // speaker_onoff tone(12, 523, 500);
}
BLYNK_WRITE(V10){
  door_sensor_onoff = param.asInt(); 
}
BLYNK_WRITE(V11){
  temp_plants_max = param.asInt();
}
BLYNK_WRITE(V12){
  temp_plants_min = param.asInt();
}
//=====================blynk 입출력핀 설정================

void bell(){
  int tones[] = {493,523,493};
  for(int melody_tone = 0; melody_tone<3;melody_tone++);
      tone(12, tones[melody_tone], 500);
}
void door_detector(){
  BLYNK_WRITE(V10);
  if(door_sensor_onoff == LOW){
    Serial.print("door_sensor_onoff: ");
    Serial.println(door_sensor_onoff);
  }
  else{
    Serial.print("door_sensor_onoff: ");
    Serial.println(door_sensor_onoff);
    
    door_sensor_value = digitalRead(door_sensor_pin);
    if(door_sensor_value == HIGH){
      Blynk.notify("침입자 감지!");
         Serial.print("door_sensor_value: ");
         Serial.println(door_sensor_value);
         if(speaker_onoff ==1 ){
          bell();
         }
    }
      else{
        Serial.print("door_sensor_value_else: ");
        Serial.println(door_sensor_value);
        }
      }
}

void sendtoAPP_DHTDATA() { //온습도 데이터 앱으로 전송 및 알림 선언
  float House_t = dht.readTemperature();
  float House_h = dht.readHumidity();
  BLYNK_WRITE(V2);
  BLYNK_WRITE(V3);
  Serial.println("======loop=====");
  Serial.print("temp_max:");
  Serial.println(temp_max);
  Serial.print("temp_min:");
  Serial.println(temp_min);
  if(House_t > temp_max ){ //실내온도 35도이상
    //Blynk.email("dkxmpower999", "경고! ", "실내 온도가 35도 이상입니다!");
    Blynk.notify("실내 온도가 설정 온도 이상입니다!");
    if(speaker_onoff ==1 )
          bell();
  }
  else if(House_t < temp_min){ // 실내온도 10도 이하
   // Blynk.email("dkxmpower999", "경고! ", "실내 온도가 10도 이하입니다!");
    Blynk.notify("실내 온도가 설정 온도 이하입니다!");
    if(speaker_onoff ==1 )
          bell();
  }
  Blynk.virtualWrite(V5, House_h); //V5가상핀을 측정습도값 입력
  Blynk.virtualWrite(V6, House_t); //v6가상핀을 측정온도값 입력
}

void sendtoAPP_DHTDATA_plants() { //온습도 데이터 앱으로 전송 및 알림 선언
  float plants_t = dht2.readTemperature();
  float plants_h = dht2.readHumidity();
  BLYNK_WRITE(V11);
  BLYNK_WRITE(V12);
  Serial.print("temp_plants_max:");
  Serial.println(temp_plants_max);
  Serial.print("temp_plants_min:");
  Serial.println(temp_plants_min);
  if(plants_t > temp_plants_max){ //실내온도 35도이상
    //Blynk.email("dkxmpower999", "경고! ", "실내 온도가 35도 이상입니다!");
    Blynk.notify("식물 측정 온도가 높습니다!");
    if(speaker_onoff ==1 )
          bell();
  }
  else if(plants_h < temp_plants_min){ // 실내온도 10도 이하
   // Blynk.email("dkxmpower999", "경고! ", "실내 온도가 10도 이하입니다!");
    Blynk.notify("식물 측정 온도가 낮습니다!");
    if(speaker_onoff ==1 )
          bell();
  }
  Blynk.virtualWrite(V7, plants_h); //V7가상핀을 측정습도값 입력
  Blynk.virtualWrite(V8, plants_t); //v8가상핀을 측정온도값 입력
}

void setup(void)
{
  Serial.begin (115200);  //시리얼통신 115200 속도값으로 정함
  Blynk.begin (auth, ssid, pass); //blynk랑 통신하기위한 값들
  WiFi.begin (ssid, pass);
  dht.begin(); //온습도센서 시작
  dht2.begin(); //온습도센서 시작
  pinMode(door_sensor_pin, INPUT);
  lcd.init(); 
  lcd2.init();
  lcd.backlight();
  lcd2.backlight();
  lcd.clear();
  lcd2.clear();
  lcd.setCursor(0,0);
  lcd2.setCursor(0,0);
  lcd.print("   Connected!");
  lcd2.print("     Connected!");
  Serial.print("LCD Connected");
  pinMode(dust_LED,OUTPUT);
  timer.setInterval(1000L, sendtoAPP_DHTDATA);
  delay(1000);
  timer2.setInterval(1000L, sendtoAPP_DHTDATA_plants);
}

void loop() {
  Blynk.run();
  timer.run();
  timer2.run();
  Serial.print("Local IP add:");
  Serial.println(WiFi.localIP());
  
  for(lcd_counter =1; lcd_counter <=2; lcd_counter++) {
  // 루프 함수에서 LCD만 실행되어 다른기능을 쓸 수 없으므로 추가
     //if문 아래에 날씨빼고 넣지 말기 오류생김...
     if(new_weatherDATA == 180){ //10분마다 서버로부터 날씨정보 받음
      new_weatherDATA = 0;
      LCD_loadingData();
      delay(1000);
      Request_WeatherData();
    }
      else{
      new_weatherDATA++;
      LCD_temphumidust();
      door_detector();
      LCD_displayWeather(weatherLocation,weatherDescription); //LCD에 날씨데이터 전송
      Blynk.virtualWrite(V0, weatherLocation);
      Blynk.virtualWrite(V1, weatherDescription);
      delay(5000); //5초마다
      
      LCD_displayConditions(LCD_t, LCD_h); //LCD에 날씨데이터 전송
      Blynk.virtualWrite(V0, "Tempeature");
      Blynk.virtualWrite(V1, LCD_t);
      delay(2500); //2.5초마다
      Blynk.virtualWrite(V0, "Humidity"); //앱에 LCD로 데이터 전송
      Blynk.virtualWrite(V1, LCD_h);
      delay(2500); //2.5초마다
  }
  lcd_counter = lcd_counter + 1;
  if(lcd_counter == 2);{
    break;
  }
  }
}

void LCD_temphumidust(){
  float House_t = dht.readTemperature();
  float House_h = dht.readHumidity();

  lcd2.clear();
  lcd2.setCursor(0,0);
  lcd2.print("Home Information");
  lcd2.setCursor(0,1);
  lcd2.print("Temp:");
  lcd2.print(House_t);
  lcd2.print(" C");
  lcd2.setCursor(0,2);
  lcd2.print("Humi:");
  lcd2.print(House_h);
  lcd2.print(" %");

  //================미세먼지 관련=================
  digitalWrite(dust_LED,LOW); // 적외선 LED ON
  delayMicroseconds(280); // 280us동안 딜레이
  vo_value=analogRead(dustout); // 데이터를 읽음
  delayMicroseconds(40); // 320us - 280us
  digitalWrite(dust_LED,HIGH); // 적외선 LED OFF
  delayMicroseconds(9680); // 10ms(주기) -320us(펄스 폭) 한 값
  
  sensor_voltage = get_voltage(vo_value);
  dust_density = get_dust_density(sensor_voltage);
  if(dust_density <0){  }
  else
  {
    dust_density_array[dust_index] = dust_density;
  dust_density_sum = dust_density_sum + dust_density_array[dust_index];
  dust_index++;
  int indexsize =30;
    if (dust_index == indexsize){
        dust_density_average = dust_density_sum/(indexsize);
        dust_density_sum=0;
        dust_index = 0;
  }
  }
  
  Serial.print("dust_index = ");    Serial.println(dust_index);
  Serial.print("value = ");         Serial.println(vo_value);
  Serial.print("Voltage = ");       Serial.print(sensor_voltage); Serial.println(" [V]");
  Serial.print("Dust Density = ");  Serial.print(dust_density);   Serial.println(" [ug/m^3]");
  Serial.print("dust_density_average = "); Serial.print(dust_density_average); Serial.println(" [ug/m^3]");

  lcd2.setCursor(0,3);
  lcd2.print("Dust:");
  lcd2.print(dust_density_average);
  lcd2.print(" ug/m^3");
  Blynk.virtualWrite(V4, dust_density_average);
}

float get_voltage(float value){
  float V= value * (5.0 / 1024.0);  // 아날로그 값을 전압 값으로 바꿈
  return V;
}

float get_dust_density(float voltage){
 float dust =(voltage - no_dust) / 0.005 + dust_balance; // 데이터 시트에 있는 미세 먼지 농도(ug) 공식 기준
 return dust;
}

//================================수정X 날시데이터================================
void Request_WeatherData(){ //보드로부터 요청한 데이터를 수신 송신 기능함수
  if (client.connect(servername, 80)) {  //보드랑 서버랑 연결 (80번포트)
    client.println("GET /data/2.5/weather?id="+CityID+"&units=metric&APPID="+APIKEY);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  } 
  else {
    Serial.println("connection Error!"); //연결 불가시 연결 불가능 메세지 출력
    Serial.println();
  }
  while(client.connected() && !client.available()) delay(1); //데이터 지연
  while (client.connected() || client.available()) { //연결 또는 사용가능할때
    char c = client.read(); //이더넷의 버퍼로부터 데이터 읽음
      result = result+c;
    }
 
  client.stop(); //클라이언트 정지
  result.replace('[', ' ');
  result.replace(']', ' ');
  Serial.println(result);
 
  char jsonArray [result.length()+1];
  result.toCharArray(jsonArray,sizeof(jsonArray));
  jsonArray[result.length() + 1] = '\0';
 
  StaticJsonBuffer<1024> json_buf;
  JsonObject &root = json_buf.parseObject(jsonArray);

  if (!root.success())
  {
    Serial.println("parseObject() failed");
  }
 
  String location = root["name"];
  String country = root["sys"]["country"];
  float temperature = root["main"]["temp"];
  float humidity = root["main"]["humidity"];
  String weather = root["weather"]["main"];
  String description = root["weather"]["description"];
 
  weatherDescription = description;
  weatherLocation = location;
  Country = country;
  LCD_t = temperature;
  LCD_h = humidity; 
}

void LCD_displayWeather(String location,String description)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(location);
  lcd.print(", ");
  lcd.print(Country);
  lcd.setCursor(0,1);
  lcd.print(description);
}
 
void LCD_displayConditions(float LCD_t,float LCD_h){
 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("Temp:"); 
 lcd.print(LCD_t,1);
 //lcd.print((char)223);
 lcd.print(" C");
 lcd.setCursor(0,1);
 lcd.print("Humi:");
 lcd.print(LCD_h,0);
 lcd.print(" %");
}
 
void LCD_loadingData(){
  lcd.clear();
  lcd.print("Loading data");
}
//================================수정X===========================================
