/*
 *  This sketch is for the ESP_Toy with scratch mesh wifiAP Mode With AP
 *  By www.chum-up.com T.kasahara 14/April/2017 Ver1.0
 *  　OTA機能あり。
 *  
 *  scratchホストになるPC(タブレット)をこの装置のSSIDに接続してください。
 *  パスワードはいりません。
 *    SSIDは　”CHUM**" **は、この装置のMACアドレスの一部
 *  
 *  ESP_Toy のハンダジャンパーにより、２モーター制御か、１モーター１サーボの
 *  ２つのモードが選べます。
 *  １モータ１サーボ　モードの時は #define SERVO をコメントでなく有効にしてください。
 *  
 *  使い方
 *   
 *  この装置を制御する変数名は下記のとおりです
 *  MA, MB モーター制御　数値がスピード、符号が回転方向になります(値は-1000～+1000)
 *  　１モータ１サーボ　モードでは　MA　は無効です
 *  DR サーボの角度　－90～＋90０がセンター
 *    ２モータ　モードでは　DR　は無効です
 *    
 *  LB, LG, LR 数値がそれぞれの色の明るさになります(値は0～255)
 *  
 *  オプションでこの装置から超音波センサーとADコンバータの２つのセンサーデータが取り込めます。
 *  UD(option), 超音波センサーの値です　US_inter　の値(ms)間隔で　送信されます
 *  　接続しない時は #define USSD をコメントにしてください。
 *  AD(option), ADの値です　US_inter　の値(ms)間隔で　送信されます
 *  　接続しない時は #define ADC をコメントにしてください。
 */

#define USSD // Ultra sound sensor
#define ADC //AD Converter
//#define SERVO //1mortor 1servo for 4ku

//************** wifi config ************************************************
//* Set these to your desired credentials. */
char ap_ssid[6] = "CHUM";
char* password = "";
char ssid[6];
byte mac_addr[6];
const char* host = "192.168.4.2";  // Scratch Host PC の IP address

//************** Adusting Constant
#define US_inter 100  //US_sensor interval (xx ms)
#define threshold 8000 //US_sensor MAX(mm)
#define max_val 1000 //Maximum Mortor value
#define servo_speed 15 //servo step delay
#define center 88 // center angle of steer normaly 90

#define DEBUG //Debug mode
//#define I2C //I2C module (Reserved)
#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_FPRINT(x,y)  Serial.print (x,y)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
 #define DEBUG_FPRINT(x,y)
#endif

#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h> 

#ifdef SERVO
  #include <Servo.h>
  Servo myservo;  // create servo object to control a servo
#endif

#define baudrate 115200
#define led 13

//**************
const int meshPort = 42001;
WiFiClient client;

//*************** Color_LED config
#include <Adafruit_NeoPixel.h>
#define C_led_pin 5
#define pix_no 1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(pix_no, C_led_pin, NEO_GRB + NEO_KHZ800);

//************** Mortor config
  #define MA1_pin 16
  #define MA2_pin 0
  #define MB1_pin 15
  #define MB2_pin 12
  #define MA_pwm_pin 13
#ifdef SERVO
  #define MB_pwm_pin 13 //same as MA
  #define servo_pin 4
#else
  #define MB_pwm_pin 4
#endif

//************** US_sensor　HC-SR04
#define trigPin 2
#define echoPin 14
//**************I2C
#define SDA 2
#define SCL 14

//************** scratch val
//char myNo[4] = {0};
int MA_V = 0 ;
int MB_V = 0 ;
int MA_D = 0 ;
int MB_D = 0 ;
int ledR = 0 ;
int ledG = 0 ;
int ledB = 0 ;
int val = 0 ;
// int US = 0 ;
// int US_span = 0;
//int myNo_ln = 0;

//************** setup
void setup() {
  Serial.begin(baudrate);
  delay(100);
  Serial.println();  Serial.println();
  Serial.println("scratch_mesh_toy_wifiAP_OTA_v1.0");
  WiFi.mode(WIFI_OFF);

  WiFi.macAddress(mac_addr);
  sprintf(ssid,"%s%x%x",ap_ssid,mac_addr[4],mac_addr[5]);
  Serial.print("Mac address =");
  for (int i=0 ; i<6 ; i++){
    Serial.print(mac_addr[i],HEX);
    Serial.print(":");
  }
  Serial.println();
  Serial.print("SSID = ");
  Serial.println(ssid);
  Serial.println();  Serial.println();

  Serial.println("Begin SoftAP");
  WiFi.mode(WIFI_AP);
  ///* You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid);

  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());
delay(1000);
  
#ifdef SERVO
  Serial.println("#define SERVO");
  //*************** servo setup
  myservo.attach(servo_pin);  // attaches the servo on servo_pin to the servo object
  servo_control(0);
#endif

#ifdef ADC
  Serial.println("#define ADC");
#endif

  //*************** color led setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //*************** mortor setup
  pinMode(MA1_pin,OUTPUT);
  digitalWrite(MA1_pin,LOW);
  pinMode(MA2_pin,OUTPUT);
  digitalWrite(MA2_pin,LOW);
  pinMode(MB1_pin,OUTPUT);
  digitalWrite(MB1_pin,LOW);
  pinMode(MB2_pin,OUTPUT);
  digitalWrite(MB2_pin,LOW);
#ifndef SERVO
  pinMode(MA_pwm_pin,OUTPUT);
  analogWrite(MA_pwm_pin,MA_V);
#endif
  pinMode(MB_pwm_pin,OUTPUT);
  analogWrite(MB_pwm_pin,MB_V);

#ifdef USSD
  Serial.println("#define USSD");
  //*************** US-sensor setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
#endif

//***************** OTA setup
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN("Start");
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
  //*************** Waiting download OTA approx 10seconds (while led_d13 on)
  digitalWrite(led, HIGH);
  for(int i=0; i<100 ; i++){
    ArduinoOTA.handle();
    delay(100);
    }
  digitalWrite(led, LOW);
  
}

#ifdef USSD
//******************* US_sens control Local functions **************************************/
int US_sens(){
    long duration, distance;
  digitalWrite(trigPin, LOW);  
  delayMicroseconds(2); 
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH,50000);
  distance = (duration/2) / 2.91;

  if (distance >= threshold || distance <= 0){
    DEBUG_PRINTLN("Out of range");
    distance =-1;
  }
  else {
    DEBUG_PRINT(distance);
    DEBUG_PRINTLN(" mm");
  }
return (int)distance;
}
#endif
#ifdef ADC
//******************* AD converrt **************************************/
int AD_sens(){
  int ad_data =analogRead(A0);
  DEBUG_PRINT(ad_data);
  DEBUG_PRINTLN(" mV");

return (int)ad_data;
}
#endif

//**************** Mortor control Local functions **************************************/
void setMA_V(int val){
    if(val < (-1)*max_val) val=(-1)*max_val;
    if(val > max_val) val=max_val;
    if(val > 0 ){
      digitalWrite(MA2_pin,LOW);
      digitalWrite(MA1_pin,HIGH);
      analogWrite(MA_pwm_pin,val);
    }else{
      digitalWrite(MA1_pin,LOW);
      digitalWrite(MA2_pin,HIGH);
      analogWrite(MA_pwm_pin,(val*(-1)));
    }
}
void setMB_V(int val){
    if(val < (-1)*max_val) val=(-1)*max_val;
    if(val > max_val) val=max_val;
    if(val > 0 ){
      digitalWrite(MB2_pin,LOW);
      digitalWrite(MB1_pin,HIGH);
      analogWrite(MB_pwm_pin,val);
    }else{
      digitalWrite(MB1_pin,LOW);
      digitalWrite(MB2_pin,HIGH);
      analogWrite(MB_pwm_pin,(val*(-1)));
    }
}

//******************************************************/

void loop() {
  //****************** Use WiFiClient class to create TCP connections
  DEBUG_PRINT("TCP connecting to ");
  DEBUG_PRINTLN(host);  
  if (!client.connect(host, meshPort)) {
    DEBUG_PRINTLN("TCP connection failed");
    return;
  }
    DEBUG_PRINTLN("TCP connected to mesh");

  //******************* Scratch mesh commands
  char broadstr[] =  "broadcast ";  //10char
  char sens_up[] = "sensor-update "; //14char
  char buff[512] = "";
  char UD_buff[32] = "";
  char AD_buff[32] = "";
  char scmd[3] = "";
  uint8_t Nchar = 0;
  int i,j;
  
  client.flush();
  //**************** Timer set for US_sens
  long US_time = millis();

  //****************** DO LOOP
  for(uint32_t n = 0; n <100000; n++) {
    delay(10);

    //****************** US_sens update
    if((millis() - US_time) > US_inter ){
    US_time = millis();
    Nchar = 0;
#ifdef USSD
      Nchar = Nchar + sprintf(UD_buff,"\"UD\" %d ",US_sens());
#endif
#ifdef ADC
      Nchar = Nchar + sprintf(AD_buff,"\"AD\" %d ",AD_sens());
#endif
      if(Nchar > 0){
        Nchar = Nchar + 14;
        sprintf(buff,"%c%c%c%csensor-update %s%s",0,0,0,Nchar,UD_buff,AD_buff);        
        client.write(&*buff,Nchar+4);
      }
    }
    
    //****************** if there are message from Scratch, read them:
    int nos = 0;
    if(client.available()) {
      for( i =0; i<4; i++){
        nos = nos*10 + client.read();
      }
      DEBUG_FPRINT(nos,DEC);
      DEBUG_PRINTLN("char");
    for( i=0;i<nos;i++){
      buff[i]=client.read();
    }
    buff[i]='\0';
      DEBUG_PRINT(buff);
      DEBUG_PRINTLN("//string end");

      //**************** check comands
      for( i=0; i<14 ; i++){
        if(buff[i] != sens_up[i]) break; //**** sensor-update receaved
      }     
      if( i == 14 ){
        DEBUG_PRINTLN("Receaved sensor-update");
        while( i < nos-2 ){
          if(buff[i] == '\"'){
            i = i+1 ;
            if((buff[i]=='M') || (buff[i]=='L') || (buff[i]=='D')){
              scmd[0] = buff[i]; scmd[1] = buff[i+1]; i = i+4; //  "MA" xxx
              val = 0;
              if (buff[i] != ' '){
                val = val*10 + atoi( &buff[i]);
                i= i+1;
              }
              DEBUG_PRINT(scmd);
              DEBUG_PRINT(" = ");
              DEBUG_PRINTLN(val);
              if(scmd[0] == 'M'){
                if(scmd[1] == 'A') setMA_V(val);
                else if(scmd[1] == 'B') setMB_V(val);                
              }else if(scmd[0] == 'L'){
                if(scmd[1] == 'B') ledB = val;
                else if(scmd[1] == 'G') ledG = val;
                else if(scmd[1] == 'R') ledR = val;
                strip.setPixelColor(0, strip.Color(ledR,ledG,ledB));
                strip.show();
              }else if((scmd[0] == 'D') && (scmd[1] == 'R')){ //DR servo control command
#ifdef SERVO
                servo_control(val);
#endif
              }
            }
          }
          i += 1;
        }
      }else {
        for( i=0; i<10 ; i++){
          if(buff[i] != broadstr[i]) break; //**** Receave broadcast
        }
        if( i == 10 ){
          DEBUG_PRINTLN("Receave broadcast");  
        }else {
         DEBUG_PRINTLN("<=Receave Error");  
        }
      }
    }
  ArduinoOTA.handle(); //********** OTA Handling
  }
  //******************* if the server's disconnected, stop the client:
  if (!client.connected()) {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("disconnecting from server. error");
    client.stop();
    // do nothing forevermore:
    while(true);
  }
}
#ifdef SERVO
void servo_control(int angle){
  DEBUG_PRINTLN(angle);
  int i = center + angle;
    if(i<0)i=0;
    if(i>180)i=180;
      myservo.write(i);
      delay(servo_speed);
}
#endif

