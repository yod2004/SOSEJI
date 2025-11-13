/*
つくるもの
・Wifiで通信する
・モーターを動かす
・エンコーダーを読む
・サーボモーターを動かす
・温度を調べる
・距離を読む

11/06 DCモーター，サーボモーターをうごかせるようになった．
*/

#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Servo.h>

//LED setup
ArduinoLEDMatrix matrix;
char message[30];

// Network setup
char SSID[] = "cafe_03_俺以外つなぐな";
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
// int enc1a = A1;
// int enc1b = A2;
// int enc2a = A3;
// int enc2b = A4;
int enc1 = A1;
int enc2 = A2;
int trig = 12;
int echo = 13;
int8_t torque1 = 0;//128~127
int8_t torque2 = 0;//128~127

//変数
volatile int count1 = 0;
volatile int count2 = 0;
bool isAutoMode = false;

// Communication setup
char c = '.';

enum STAGE{//自立制御時の段階を設定
  Push2Cups,//中央2つのコップを押し出す
  Back,//戻る
  RotateRight,//Cup1を見つけるまで右旋回
  CatchCup1,//Cup1をつかんで持ち上げる
  CarryCup1,//Cup1を左テーブルまで運ぶ
  PutCup1,//Cup1をテーブルに置く．
  RotateLeft,//Cup2を見つけるまで左旋回
  CatchCup2,//Cup2をつかんで持ち上げる
  CarryCup2,//Cup2を右テーブルまで運ぶ
  PutCup2,//Cup2を右テーブルに置く
  ReturnManual
};
STAGE stage = Push2Cups;

//モジュールに関する関数
void MoveMotor(int id,int8_t torque){//-128 < torwue < 127
  if(torque == -128){
    torque = -127;
  }
  if(id == 1){//1つ目のモーターを動かす
    torque1 = torque;
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
  if(id == 2){//1つ目のモーターを動かす
    torque2 = torque;
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

void _1_CHANGE(){
  if(torque1 > 0){
    count1 ++;
  }else{
    count1 --;
  }
}

void _2_CHANGE(){
  if(torque2 > 0){
    count2 ++;
  }else{
    count2 --;
  }
}

// void _1A_CHANGE() {
//   if (digitalRead(enc1a) == HIGH) { //RISING
//     if (digitalRead(enc1b) == HIGH) {
//       count1--;
//     } else {
//       count1++;
//     }
//   } else { //FALLING
//     if (digitalRead(enc1b) == HIGH) {
//       count1++;
//     } else {
//       count1--;
//     }
//   }
// }

// void _1B_CHANGE() {
//   if (digitalRead(enc1b) == HIGH) { //RISING
//     if (digitalRead(enc1a) == HIGH) {
//       count1--;
//     } else {
//       count1++;
//     }
//   } else { //FALLING
//     if (digitalRead(enc1a) == HIGH) {
//       count1++;
//     } else {
//       count1--;
//     }
//   }
// }
// void _2A_CHANGE() {
//   if (digitalRead(enc2a) == HIGH) { //RISING
//     if (digitalRead(enc2b) == HIGH) {
//       count2--;
//     } else {
//       count2++;
//     }
//   } else { //FALLING
//     if (digitalRead(enc2b) == HIGH) {
//       count2++;
//     } else {
//       count2--;
//     }
//   }
// }

// void _2B_CHANGE() {
//   if (digitalRead(enc2b) == HIGH) { //RISING
//     if (digitalRead(enc2a) == HIGH) {
//       count2--;
//     } else {
//       count2++;
//     }
//   } else { //FALLING
//     if (digitalRead(enc2a) == HIGH) {
//       count2++;
//     } else {
//       count2--;
//     }
//   }
// }

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
  pinMode(echo,INPUT);
  // attachInterrupt(digitalPinToInterrupt(enc1a), _1A_CHANGE, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(enc1b), _1B_CHANGE, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(enc2a), _2A_CHANGE, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(enc2b), _2B_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc1), _1_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2), _2_CHANGE, CHANGE);
  servo1.attach(servo1_pin, 500, 2400);
  servo2.attach(servo2_pin, 500, 2400);
  Serial.begin(9600);
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
  Serial.print(count1);Serial.print(" ");Serial.println(count2);
  if(isAutoMode == true){
// // 1. まずクライアントからの入力をチェック (最優先)
    if (client.available() > 0) {
      c = client.read();
      if (c == 'y') { // 'y' が押されたら
        isAutoMode = false;      // 手動モードに戻す
        MoveMotor(1, 0);         // 安全のためモーターを止める
        MoveMotor(2, 0);
        sprintf(message, "Manual"); // 手動モードになったことを表示
        LED_print(0, 1, message, NO_SCROLL);
        // return; // ★重要: このloopはここで終了。次のloopから手動モードに入る。
        stage = ReturnManual;
      }else if(c == 'U'){  
        value = analogRead(thermister);
        client.write(highByte(value)); //上位バイト
        client.write(lowByte(value));  //下位バイト
      }
      // 'y' 以外の文字（改行コードなど）は読み捨てられ、無視される
    }
//     if(isAutoMode == false){
//       return;
//     }else{
        // 2. 'y'が押されていなかった場合、自動制御(switch)を実行する
      // (注：client.available() <= 0 の場合も、こちらが実行されます)
      switch (stage) { //モーターの出力を決める
        case Push2Cups:
        { // 変数宣言のためにスコープ { } を追加
          // int torque1 = 450 - count1; // P制御の計算
          // int torque2 = 450 - count2; // P制御の計算
          
          // ★★★重要(A)★★★
          // MoveMotorに渡す値を -128〜127 の範囲に収める (オーバーフロー防止)
          MoveMotor(1, constrain(153 - count1, -128, 127)); 
          MoveMotor(2, constrain(153 - count2, -128, 127));
          
          if (count1 > 450 && count2 > 450) {
            stage = Back;
          }
          sprintf(message, "Push");
          LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

          break;
        }

        case Back:
        { // スコープ { } を追加
          // int torque1 = count1 - 200; // P制御の計算
          // int torque2 = count2 - 200; // P制御の計算

          // ★★★重要(A)★★★
          // MoveMotorに渡す値を -128〜127 の範囲に収める (オーバーフロー防止)
          MoveMotor(1, constrain(127 - count1, -128, 127));
          MoveMotor(2, constrain(127 - count2, -128, 127));
          
          if (count1 < 127 && count2 < 127) {
            stage = RotateRight;
          }
          sprintf(message, "Back");
          LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
          break;
        }
        
        case RotateRight:
          MoveMotor(1,constrain(147 - count1, -128, 127));
          MoveMotor(2,constrain(107 - count2, -128, 127));
          if(count1 > 147 && count2 < 107){
            stage = CatchCup1;
          }
          sprintf(message, "RoR");
          LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
          break;

        case CatchCup1:
          stage = CarryCup1; 
          break;
        
        case CarryCup1:
          stage = PutCup1;
          break;
        
        case PutCup1:
          stage = RotateLeft;
          break;
        
        case RotateLeft:
          MoveMotor(1,constrain(107 - count1, -128, 127));
          MoveMotor(2,constrain(147 - count2, -128, 127));
          if(count1 < 107 && count2 > 147){
            stage = CatchCup2;
          }
          sprintf(message, "RoL");
          LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
          break;
        
        case CatchCup2:
          stage = CarryCup2;
          break;
        
        case CarryCup2:
          stage = PutCup2;
          break;
        
        case PutCup2:
          stage = ReturnManual;
          break;
        
        case ReturnManual:
          isAutoMode = false;
          break;

        default:
          isAutoMode = false;
          break;
      }
    // }
  }else{//手動モード
    if (client.available() <= 0) {
      return;  // データが来なかったらなにもしない
    }

    c = client.read();
    // 以下はコマンドの解釈．'U'のときのみ，A0ピンの値をPCに送信する．
    // それ以外のときは，コマンドの文字をそのままLEDに表示する．
    switch (c) {
      case 'w'://前進
        MoveMotor(1,127);//torqueは-128~127
        MoveMotor(2,127);//torqueは-128~127
        sprintf(message, "^  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 's'://後退
        MoveMotor(1,-127);//torqueは-128~127
        MoveMotor(2,-127);//torqueは-128~127
        sprintf(message, "v  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'd'://右回転
        MoveMotor(1,127);//torqueは-128~127
        MoveMotor(2,-127);//torqueは-128~127
        sprintf(message, ">  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'a'://左回転
        MoveMotor(1,-127);//torqueは-128~127
        MoveMotor(2,127);//torqueは-128~127
        sprintf(message, "<  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'W'://ゆっくり前進
        MoveMotor(1,64);//torqueは-128~127
        MoveMotor(2,64);//torqueは-128~127
        sprintf(message, "^  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'S'://ゆっくり後退
        MoveMotor(1,-64);//torqueは-128~127
        MoveMotor(2,-64);//torqueは-128~127
        sprintf(message, "v  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'D'://ゆっくり右回転
        MoveMotor(1,64);//torqueは-128~127
        MoveMotor(2,-64);//torqueは-128~127
        sprintf(message, ">  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
      case 'A'://ゆっくり左回転
        MoveMotor(1,-64);//torqueは-128~127
        MoveMotor(2,64);//torqueは-128~127
        sprintf(message, "<  ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;



      case 'p'://アームを上に
        servo1.write(180);
        sprintf(message, "up ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'l'://アームを下に
        servo1.write(0);
        sprintf(message, "down");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'k'://アームを開く
        servo2.write(180);
        sprintf(message, "open");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'j'://アームを閉じる
        servo2.write(0);
        sprintf(message, "close");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'U'://温度センサ取得  
        value = analogRead(thermister);
        client.write(highByte(value)); //上位バイト
        client.write(lowByte(value));  //下位バイト
        break;

      case 'K'://距離センサ取得
        break;

      case 't':
        isAutoMode = true;
        break;

      default:
        MoveMotor(1,0);//モーターを止める
        MoveMotor(2,0);
        sprintf(message, "%c  ", c);
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;
    }    
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
