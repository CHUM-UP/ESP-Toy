/*
 *  This sketch is for the ESP_Toy_LED_Panel With Scratch
 *  By www.chum-up.com T.kasahara 14/April/2017 Ver0
 *  
 *  10X10 LED Panel をScratchから制御します
 *  
 *  使い方
 *  Scratch Host PCを"CHUM****"のAPに接続し、Scratchを起動する。
 *   
 *  この装置を制御する変数名は下記のとおりです
 *  LX=LED X Address,LY=LED Y Address,LB=LED Blue Data,LG=LED Green Data,LR=LED Red Dtata
 *　LX,LYのアドレス（0-9）を設定し、そのLEDのData（0-255）をLB,LG,LRで設定する
 *　その後、BroadCast "LS" でLEDのデータをセットし、"LL" で表示が変わる。
 */

#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h> 

//************** wifi config ************************************************
//* Set these to your desired credentials. */
char ap_ssid[6] = "CHUM";
char* password = "";
char ssid[6];
byte mac_addr[6];
const char* host = "192.168.4.2";  // Scratch Host PC の IP address
const int meshPort = 42001;
WiFiClient client;

//************** Adusting Constant
//#define DEBUG //Debug mode
#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_FPRINT(x,y)  Serial.print (x,y)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
 #define DEBUG_FPRINT(x,y)
#endif

#define baudrate 115200
#define led 13

//*************** Color_LED config
#include <Adafruit_NeoPixel.h>
#define C_led_pin 5
#define pix_no 101
Adafruit_NeoPixel strip = Adafruit_NeoPixel(pix_no, C_led_pin, NEO_GRB + NEO_KHZ800);

//************** scratch val
int lx,ly,lb,lg,lr;

//************** setup
void setup() {
  Serial.begin(baudrate);
  delay(100);
  Serial.println("scratch_mesh_LED_Panel_wifiAP_OTA");
  WiFi.mode(WIFI_OFF);

  //*************** color led setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

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

  WiFi.softAP(ssid);

  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());
delay(1000);
    
//***************** OTA setup
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

//******************************************************/
void loop() {
  ArduinoOTA.handle(); //********** OTA Handling
  
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
  int val;
  
  client.flush();

  //****************** DO LOOP
  for(uint32_t n = 0; n <100000; n++) {
    delay(10);

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
            if(buff[i]=='L'){
              scmd[0] = buff[i]; scmd[1] = buff[i+1]; i = i+4; //  "LX" xxx
              val = 0;
              if (buff[i] != ' '){
                val = val*10 + atoi( &buff[i]);
                i= i+1;
              }
              DEBUG_PRINT(scmd);
              DEBUG_PRINT(" = ");
              DEBUG_PRINTLN(val);

              if(scmd[1] == 'X') lx = val;
              else if(scmd[1] == 'Y') ly = val;
              else if(scmd[1] == 'G') lg = val;
              else if(scmd[1] == 'R') lr = val;
              else if(scmd[1] == 'B') lb = val;
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
          if(buff[i] == '\"'){
            i = i+1 ;
            if((buff[i]=='L') && (buff[i+1]=='L')){
               strip.show();
            }else if((buff[i]=='L') && (buff[i+1]=='S')){
              int lno=ly * 10 + lx +1;
              strip.setPixelColor(lno, strip.Color(lr,lg,lb));                            
            }
          }
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

