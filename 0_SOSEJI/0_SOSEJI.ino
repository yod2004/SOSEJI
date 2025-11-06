/*
つくるもの
・Wifiで通信する
・モーターを動かす
・エンコーダーを読む
・サーボモーターを動かす
・温度を調べる
・距離を読む
*/

#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Servo.h>

//LED setup
ArduinoLEDMatrix matrix;
char message[30];

// Network setup
char SSID[] = "cafe_00_俺以外つなぐな";
char PASS[] = "123456789";
byte IP[] = { 192, 48, 56, 1 };
int PORT = 80;
int status = WL_IDLE_STATUS;
WiFiServer server(PORT);  //Arduino上のサーバの設定

//Servo setup
Servo servo1;
Servo servo2;

//ピン設定
int ain1 = 2;
int ain2 = 3;
int bin1 = 4;
int bin2 = 7;
int pwma = 5;
int pwmb = 6;
int servo1_pin = 9;
int servo2_pin = 10;
int thermister = A0;
int enca = A1;
int encb = A2;
int trig = A3;
int echo = A4;

//変数
int count = 0;

// Communication setup
char c = '.';

//モジュールに関する関数
void MoveMotor(int id,int8_t torque){//-128 < torwue < 127
  if(id == 0){//1つ目のモーターを動かす
    if(torque >= 0){//トルクが正のとき
      digitalWrite(ain1,HIGH);
      digitalWrite(ain2,LOW);
      analogWrite(pwma,torque*2);//analogWriteは0~255
    }
    if(torque < 0){//トルクが負のとき
      digitalWrite(ain1,LOW);
      digitalWrite(ain2,HIGH);
      analogWrite(pwma,-1 * torque*2);//analogWriteは0~255
    }
  }
  if(id == 1){//1つ目のモーターを動かす
    if(torque >= 0){//トルクが正のとき
      digitalWrite(bin1,HIGH);
      digitalWrite(bin2,LOW);
      analogWrite(pwmb,torque*2);
    }
    if(torque < 0){//トルクが負のとき
      digitalWrite(bin1,LOW);
      digitalWrite(bin2,HIGH);
      analogWrite(pwmb,-1 * torque*2);
    }
  }
}
void A_CHANGE() {
  if (digitalRead(enca) == HIGH) { //RISING
    if (digitalRead(encb) == HIGH) {
      count--;
    } else {
      count++;
    }
  } else { //FALLING
    if (digitalRead(encb) == HIGH) {
      count++;
    } else {
      count--;
    }
  }
}

void B_CHANGE() {
  if (digitalRead(encb) == HIGH) { //RISING
    if (digitalRead(enca) == HIGH) {
      count--;
    } else {
      count++;
    }
  } else { //FALLING
    if (digitalRead(enca) == HIGH) {
      count++;
    } else {
      count--;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  matrix.begin();      // start LED matrix
  wifi_setup();        // start wifi
  pinMode(A0, INPUT);  // set analog input
  pinMode(ain1,OUTPUT);
  pinMode(ain2,OUTPUT);
  pinMode(bin1,OUTPUT);
  pinMode(bin2,OUTPUT);
  pinMode(thermister,INPUT);
  pinMode(trig,OUTPUT);
  pinMode(echo,OUTPUT);
  // attachInterrupt(digitalPinToInterrupt(enca),A_CHANGE,CHANGE);
  attachInterrupt(digitalPinToInterrupt(enca), A_CHANGE, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(encb),B_CHANGE,CHANGE);
  attachInterrupt(digitalPinToInterrupt(encb), B_CHANGE, CHANGE);
  servo1.attach(servo1_pin, 500, 2400);
  servo2.attach(servo2_pin, 500, 2400);
}

void loop() {
  // put your main code here, to run repeatedly:
  int value;

  // アクセスポイントに他のデバイスがつながるのを待つ
  if (WiFi.status() != WL_AP_CONNECTED) {
    LED_print(0, 1, "NO DEVICE", SCROLL_LEFT);
    return;
  }

  //接続されているクライアントを確認
  WiFiClient client = server.available();
  if (!client) {
    LED_print(0, 1, "NO CLIENT", SCROLL_LEFT);
    return;
  }

  if (!client.connected()) {
    client.stop();  //接続が切れてたらクライアントを終了
    return;
  }

  if (client.available() <= 0) {
    return;  // データが来なかったらなにもしない
  }

  c = client.read();
  // 以下はコマンドの解釈．'U'のときのみ，A0ピンの値をPCに送信する．
  // それ以外のときは，コマンドの文字をそのままLEDに表示する．
  switch (c) {
    case 'w'://前進
      MoveMotor(0,127);//torqueは-128~127
      MoveMotor(1,-128);//torqueは-128~127
      // message = "go";
      sprintf(message, "^");
      LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
      break;
    case 's'://後退
      MoveMotor(0,-128);//torqueは-128~127
      MoveMotor(1,127);//torqueは-128~127
      // message = "back";
      sprintf(message, "v");
      LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
      break;
    case 'd'://右回転
      MoveMotor(0,-128);//torqueは-128~127
      MoveMotor(1,-128);//torqueは-128~127
      // message = "right";
      sprintf(message, ">");
      LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
      break;
    case 'a'://左回転
      MoveMotor(0,127);//torqueは-128~127
      MoveMotor(1,127);//torqueは-128~127
      // message = "left";
      sprintf(message, "<");
      LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
      break;

    case 'p'://アームを上に
      servo1.write(180);
      break;

    case 'l'://アームを下に
      servo1.write(0);
      break;

    case 'k'://アームを開く
      servo2.write(180);
      break;

    case 'j'://アームを閉じる
      servo2.write(0);
      break;

    case 'U'://温度センサ取得  
      value = analogRead(thermister);
      client.write(highByte(value)); //上位バイト
      client.write(lowByte(value));  //下位バイト
      break;

    case 'K'://距離センサ取得
      break;
    
    default:
      MoveMotor(0,0);//モーターを止める
      MoveMotor(1,0);
      sprintf(message, "%c  ", c);
      LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
      break;
  }
}

//LEDマトリクスに文字を表示
void LED_print(int x, int y, char message[], int scroll) {
  matrix.beginDraw();
  matrix.textScrollSpeed(80);
  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_4x6);
  matrix.beginText(x, y, 0xFFFFFF);
  matrix.println(message);
  matrix.endText(scroll);
  matrix.endDraw();
}

//アクセスポイントSSIDを開始し，自己アドレスをIPに設定
void wifi_setup() {
  WiFi.config(IPAddress(IP));
  status = WiFi.beginAP(SSID, PASS);
  if (status != WL_AP_LISTENING) {
    while (true) {
      LED_print(0, 1, "AP failed", SCROLL_LEFT);
    }
  }
  server.begin();  //サーバを起動
}
