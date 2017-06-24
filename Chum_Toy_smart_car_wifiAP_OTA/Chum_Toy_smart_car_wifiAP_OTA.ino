/*
 *  *  This sketch is for the ESP_TRACER & Rover with scratch mesh. 
 *  By www.chum-up.com T.kasahara 25/01/2017 Ver1.0
 *  
 *  使い方
mortor color_led (sevo なし）
をスマホのブラウザーで制御する、サンプルソフト
前後左右と斜め、８方向に移動するように制御します。
スマホを　この装置のSSIDに接続し、ブラウザで　１９２．１６８.４．１　にアクセスします
画面は青くなるだけですが８方向に巣ワイプするとそちらに動き、LEDの色が変わります
US_sensor　を接続することも可能です

OTAは、この装置が接続したSSIDのポートから行います。(softAPではありません）
 */

/* Create a WiFi access point and provide a web server on it. */
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

/* Set these to your desired credentials. */
const char *ap_ssid = "chummy_robo";
const char *ap_password = "";
//connect to
const char* ssid = "chum-up-second";
const char* password = "Kasaharaterua";

ESP8266WebServer server(80);
ESP8266WebServer server_8080(8080);

//*************** Color_LED config
#include <Adafruit_NeoPixel.h>
#define C_led_pin 5
#define pix_no 1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(pix_no, C_led_pin, NEO_GRB + NEO_KHZ800);

//#define servo_pin 4
#define max_val 1000
#define baudrate 115200
#define led 13
#define US_inter 100 //US_sensor interval (xx ms)
#define threshold 8000//US_sensor MAX(mm)
#define Mortor_speed 1000 //Mortor speed
#define Mortor_L_speed 800 //Mortor speed
#define turn_diff 150 // Mortor speed differential
#define servo_speed 15 //servo step delay
#define center 88 // center angle of steer normaly 90
#define steer 30 // max steering angle

//************** Mortor config
#define MA1_pin 16
#define MA2_pin 0
#define MB1_pin 15
#define MB2_pin 12
#define MA_pwm_pin 13
#define MB_pwm_pin 4


//************** US_sensor　HC-SR04
#define trigPin 2
#define echoPin 14
//**************I2C
#define SDA 2
#define SCL 14

#define m_start  0
#define m_stop   1
#define m_back  2

char state_a = m_stop;
char state_b = m_stop;
int MM_V = 0;
int MA_speed = 0;
int MB_speed = 0;

String form ="<html>"
"<head>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1\">"
"<style>"
"* { padding: 0; margin: 0; }"
"body { background-color: #0097C1; }"
"</style>"
"</head>"
"<body>"
"<div style=\"position: fixed; bottom: 0;\" id=\"value\"></div>"
"<form action=\"\" target=\"tif\" id=\"form\">"
"<iframe src=\"javascript: false;\" style=\"display: none;\" name=\"tif\" id=\"tif\"></iframe>"
"</form>"
"<script>"
"var offset = 50;"
"document.body.style.height = document.body.clientHeight + offset + \'px\';"
"document.body.style.width = document.body.clientWidth + offset + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.height = document.body.style.height + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.width = document.body.style.width + \'px\';"
"var moveHomePosition = function() {"
    "document.body.scrollTop = offset / 2;"
    "document.body.scrollLeft = offset / 2;"
"};"
"setTimeout(moveHomePosition, 500);"
"var startX = 0;var startY = 0;var command =\'/stop\';"
"var threshold = 40;"
"var esp_port = \'http://192.168.4.1:8080\';"
"var el_form = document.getElementById(\'form\');"
"document.body.ontouchstart = function(event) {"
    "startX = event.touches[0].clientX;"
    "startY = event.touches[0].clientY;"
"};"
"document.body.ontouchmove = function(event) {"
    "var x = parseInt(event.touches[0].clientX - startX);"
    "var y = parseInt(event.touches[0].clientY - startY);"
    "var url = null;"
    "if (x < (threshold * -1)) {"
       "if (y < (threshold * -1)){"
          "url = \'/f_left\';"
       "} else if (y > threshold) {"
          "url = \'/r_left\';"
       "}else {"
        "url = \'/left\';"
       "}"
    "} else if (x > threshold) {"
       "if (y < (threshold * -1)) {"
          "url = \'/f_right\';"
      "} else if (y > threshold) {"
          "url = \'/r_right\';"
      "}else{"
          "url = \'/right\';"
      "}"
     "} else {"
        "if (y < (threshold * -1)) {"
            "url = \'/drive\';"
        " } else if (y > threshold) {"
            "url = \'/back';"
        "}"
     "}"
    "if (command != url) {"
      "if (url) {"
          "el_form.action = esp_port + url;"
          "el_form.submit();"
          "document.getElementById(\'value\').innerHTML = url;"
      "}"
    "}"
    "command = url;"
"};"
"document.body.ontouchend = function(event) {"
    "el_form.action = esp_port + \'/stop\';"
    "el_form.submit();"
    "setTimeout(moveHomePosition, 50);"
   "document.getElementById('value').innerHTML = \'/stop\';"
"};"
"</script>"
"</body>"
"</html>";

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Chum_Toy_smart_car_wifiAP_OTA v1.0");
  
//
  Serial.println("Begin SoftAP");
  WiFi.mode(WIFI_AP_STA);
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());

  //*************** wifi connecting
  Serial.print("Wifi Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println("Connected");
  Serial.print("my IP address: ");
  Serial.println(WiFi.localIP());

//****** OTA COnfig

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
//*

	server.on("/", handleRoot);

  // And as regular external functions:
  server_8080.on("/stop", handle_stop);
  server_8080.on("/drive", handle_drive);
  server_8080.on("/back", handle_back);
  server_8080.on("/left", handle_left);
  server_8080.on("/right", handle_right);
  server_8080.on("/f_left", handle_f_left);
  server_8080.on("/f_right", handle_f_right);
  server_8080.on("/r_left", handle_r_left);
  server_8080.on("/r_right", handle_r_right);
 
	server.begin();
  server_8080.begin();
	Serial.println("HTTP server started");
  //*************** color led setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  //*************** servo setup
//  myservo.attach(servo_pin);  // attaches the servo on servo_pin to the servo object
//  servo_control(center);
  //*************** mortor setup
  pinMode(MA1_pin,OUTPUT);
  digitalWrite(MA1_pin,LOW);
  pinMode(MA2_pin,OUTPUT);
  digitalWrite(MA2_pin,LOW);
  pinMode(MB1_pin,OUTPUT);
  digitalWrite(MB1_pin,LOW);
  pinMode(MB2_pin,OUTPUT);
  digitalWrite(MB2_pin,LOW);
  pinMode(MA_pwm_pin,OUTPUT);
  analogWrite(MA_pwm_pin,MM_V);
  pinMode(MB_pwm_pin,OUTPUT);
  analogWrite(MB_pwm_pin,MM_V);

  delay(100);
}


void loop() {
  ArduinoOTA.handle();
	server.handleClient();
  server_8080.handleClient();
}


void handleRoot() {
  server.send(200, "text/html", form);
}


void handle_stop() {
  Serial.print("stop\r\n");
  stop_ma();
  stop_mb();
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.show();
  server.send(200, "text/html", "");
}

void handle_drive() {
 Serial.print("drive\r\n");
  strip.setPixelColor(0, strip.Color(30,25,35));
  strip.show();
  
  MA_speed = Mortor_speed;
  MB_speed = Mortor_speed;
  start_ma();
  start_mb();

  server.send(200, "text/html", "");
}

void handle_back() {
  Serial.print("back\r\n");
  strip.setPixelColor(0, strip.Color(100,0,0));
  strip.show();

  MA_speed = Mortor_speed;
  MB_speed = Mortor_speed;
  reverse_ma();
  reverse_mb();

}

void handle_left(){
  Serial.print("left\r\n");
  strip.setPixelColor(0, strip.Color(0,80,0));
  strip.show();

  MA_speed = Mortor_speed;
  MB_speed = Mortor_speed;
  start_ma();
  reverse_mb();

  server.send(200, "text/html", "");
}

void handle_right(){
  Serial.print("right\r\n");
  strip.setPixelColor(0, strip.Color(0,0,100));
  strip.show();

  MA_speed = Mortor_speed;
  MB_speed = Mortor_speed;
  start_mb();
  reverse_ma();
  
  server.send(200, "text/html", "");
}


void handle_f_left(){
  Serial.print("f_left\r\n");
  strip.setPixelColor(0, strip.Color(20,50,30));
  strip.show();
  
  MA_speed = Mortor_speed;
  MB_speed = Mortor_L_speed;
  start_ma();
  start_mb();

  server.send(200, "text/html", "");
}

void handle_f_right(){
  Serial.print("f_right\r\n");
  strip.setPixelColor(0, strip.Color(0,40,60));
  strip.show();

  MA_speed = Mortor_L_speed;
  MB_speed = Mortor_speed;
  start_ma();
  start_mb();

 server.send(200, "text/html", "");
}

void handle_r_left(){
  Serial.print("r_left\r\n");
  strip.setPixelColor(0, strip.Color(50,40,0));
  strip.show();

  MA_speed = Mortor_speed;
  MB_speed = Mortor_L_speed;
  reverse_ma();
  reverse_mb();

  server.send(200, "text/html", "");
}

void handle_r_right(){
  Serial.print("r_right\r\n");
  strip.setPixelColor(0, strip.Color(40,20,50));
  strip.show();

  MA_speed = Mortor_L_speed;
  MB_speed = Mortor_speed;
  reverse_ma();
  reverse_mb();

  server.send(200, "text/html", "");
}

void start_ma(){
  if(state_a == m_back){
      digitalWrite(MA2_pin,LOW);
      digitalWrite(MA1_pin,HIGH);
      analogWrite(MA_pwm_pin,0);
      delay(10);
      analogWrite(MA_pwm_pin,MA_speed);
  }else if(state_a == m_stop){
      digitalWrite(MA2_pin,LOW);
      digitalWrite(MA1_pin,HIGH);
      analogWrite(MA_pwm_pin,MA_speed);
  }
  state_a = m_start;
}

void reverse_ma(){
  if(state_a == m_start){
      digitalWrite(MA1_pin,LOW);
      digitalWrite(MA2_pin,HIGH);
      analogWrite(MA_pwm_pin,0);
      delay(10);
      analogWrite(MA_pwm_pin,MA_speed);
  }else if(state_a == m_stop){
      digitalWrite(MA1_pin,LOW);
      digitalWrite(MA2_pin,HIGH);
      analogWrite(MA_pwm_pin,MA_speed);
  }
  state_a = m_back;
}

void stop_ma(){
      digitalWrite(MA2_pin,LOW);
      digitalWrite(MA1_pin,HIGH);
      analogWrite(MA_pwm_pin,0);
      state_a = m_stop;
}

void start_mb(){
  if(state_b == m_back){
      digitalWrite(MB2_pin,LOW);
      digitalWrite(MB1_pin,HIGH);
      analogWrite(MB_pwm_pin,0);
      delay(10);
      analogWrite(MB_pwm_pin,MB_speed);
  }else if(state_b == m_stop){
      digitalWrite(MA2_pin,LOW);
      digitalWrite(MA1_pin,HIGH);
      analogWrite(MB_pwm_pin,MB_speed);
  }
  state_b = m_start;
}

void reverse_mb(){
  if(state_b == m_start){
      digitalWrite(MB1_pin,LOW);
      digitalWrite(MB2_pin,HIGH);
      analogWrite(MB_pwm_pin,0);
      delay(10);
      analogWrite(MB_pwm_pin,MB_speed);
  }else if(state_b == m_stop){
      digitalWrite(MB1_pin,LOW);
      digitalWrite(MB2_pin,HIGH);
      analogWrite(MB_pwm_pin,MB_speed);
  }
  state_b = m_back;
}

void stop_mb(){
      digitalWrite(MB2_pin,LOW);
      digitalWrite(MB1_pin,HIGH);
      analogWrite(MB_pwm_pin,0);
      state_b = m_stop;
}

