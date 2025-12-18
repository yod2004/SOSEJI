/*
つくるもの
・Wifiで通信する
・モーターを動かす
・エンコーダーを読む
・サーボモーターを動かす
・温度を調べる
・距離を読む

11/06 DCモーター，サーボモーターをうごかせるようになった．
12/12 ほぼできてる．あとは自律動作を調整していくだけ．
*/

#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Servo.h>

//LEDマトリクスを使うためにインスタンス化
ArduinoLEDMatrix matrix;
char message[30];

// ネットワークの準備
char SSID[] = "cafe_03_とっとこ菊太郎";
char PASS[] = "123456789";
byte IP[] = { 192, 48, 56, 1 };
int PORT = 80;
int status = WL_IDLE_STATUS;
WiFiServer server(PORT);  //Arduino上のサーバの設定

//サーボの準備
Servo servo1;//上げ下げ
Servo servo2;//掴む離す

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
int enc1 = A1;
int enc2 = A2;
int trig = 12;
int echo = 13;
int8_t torque1 = 0;//128~127
int8_t torque2 = 0;//128~127

//変数
volatile int count1 = 0;//タイヤ1のエンコーダーのカウント
volatile int count2 = 0;//タイヤ2のエンコーダーのカウント
bool isAutoMode = false;//オートモードかどうか
bool isRotateRight = false;//stage:RotateRightで使う変数

float c1 = 0;
float c2 = 0;
float a = 24 / 94.2;
float b = 1 / 4.5;
float push_c1;
float push_c2;
float back_c1;
float back_c2;
float find1_c1;
float find1_c2;
float carry1_c1;
float carry1_c2;
float before1_c1;
float before1_c2;
float after1_c1;
float after1_c2;
float RotateCenter1_c1;
float RotateCenter1_c2;
float find2_c1;
float find2_c2;
float carry2_c1;
float carry2_c2;
float before2_c1;
float before2_c2;
float after2_c1;
float after2_c2;
float RotateCenter2_c1;
float RotateCenter2_c2;
float return_c1;
float return_c2;

// Communication setup
char c = '.';

enum STAGE{//自立制御時の段階を設定
  Push2Cups,//中央2つのコップを押し出す
  Back,//戻る
  RotateRight,//Cup1を見つけるまで右旋回
  CatchCup1,//Cup1をつかんで持ち上げる
  CarryCup1,//Cup1を左テーブルまで運ぶ
  BeforePut1,//置く前に前に進む
  PutCup1,//Cup1をテーブルに置く．
  AfterPut1,//置いた後に後ろに戻る
  RotateCenter1,
  RotateLeft,//Cup2を見つけるまで左旋回
  CatchCup2,//Cup2をつかんで持ち上げる
  CarryCup2,//Cup2を右テーブルまで運ぶ
  BeforePut2,//置く前に前に進む
  PutCup2,//Cup2を右テーブルに置く
  AfterPut2,//置いた後に後ろに戻る
  RotateCenter2,//まっすぐを向く
  Return,//初期位置に戻る
  ReturnManual
};
STAGE stage = Push2Cups;

// enum STAGE_MID{//自律制御時の段階を設定(中間競技用)
//   rotL,
//   back15,
//   rotR,
//   back60,
//   go60,
//   rotR2,
//   back30,
//   rotL2,
//   back60_2,
//   go10,
//   returnManual
// };
// STAGE_MID stageMid = rotL;

//モーターを動かす関数 idとトルクを指定できる
void MoveMotor(int id,int8_t torque){//torqueは-127~127の範囲で送られてくる.
  if(id == 1){//idが1なら1つ目のモーターを動かす
    torque1 = torque;//エンコーダー1がタイヤ1の回転方向を知れるように，torque1にtorqueを代入
    if(torque >= 0){//トルクが正のとき
      digitalWrite(ain1,HIGH);
      digitalWrite(ain2,LOW);
      analogWrite(pwma,torque*2);//analogWriteは0~255の範囲を受け付けるのでtorqueを2倍する
    }
    if(torque < 0){//トルクが負のとき
      digitalWrite(ain1,LOW);
      digitalWrite(ain2,HIGH);
      analogWrite(pwma,-1 * torque*2);//analogWriteは0~255の範囲を受け付けるのでtorqueを2倍する
    }
  }
  if(id == 2){//idが2なら2つ目のモーターを動かす
    torque2 = torque;//エンコーダー2がタイヤ2の回転方向を知れるように，torque2にtorqueを代入
    if(torque >= 0){//トルクが正のとき
      digitalWrite(bin1,HIGH);
      digitalWrite(bin2,LOW);
      analogWrite(pwmb,torque*2);//analogWriteは0~255の範囲を受け付けるのでtorqueを2倍する
    }
    if(torque < 0){//トルクが負のとき
      digitalWrite(bin1,LOW);
      digitalWrite(bin2,HIGH);
      analogWrite(pwmb,-1 * torque*2);//analogWriteは0~255の範囲を受け付けるのでtorqueを2倍する
    }
  }
}

float getDistance(){
  // トリガーを初期化（LOWにする）
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  // 10マイクロ秒のパルスを出力して超音波を発射
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // echoピンがHIGHになっている時間を計測（単位：マイクロ秒）
  // 第3引数はタイムアウト設定（ここでは30ms = 約5mまで）
  unsigned long duration = pulseIn(echo, HIGH, 30000);

  // 計測できなかった場合（タイムアウト時）は -1 を返すなどの処理
  if (duration == 0) {
    // Serial.println("time out");
    return -1;
  }

  // 距離を計算
  // 音速 340m/s = 0.034cm/us
  // 往復なので2で割る: 0.034 / 2 = 0.017
  float distance = duration * 0.017;
  // Serial.println(distance);
  return distance;
}

float getTempC(){
  int tempRending = analogRead(thermister);
  double tempK = log(10000.0 * ((1024.0/tempRending-1)));
  tempK = 1/(0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK)) * tempK);
  return tempK - 273.15;
}

void _1_CHANGE(){//エンコーダー1が変わったときに実行される関数
  if(torque1 > 0){
    count1 ++;//現在のモーター1のトルクが正ならcount1を1増やす
  }else{
    count1 --;//現在のモーター1のトルクが負ならcount1を1減らす
  }
}

void _2_CHANGE(){//エンコーダー2が変わったときに実行される関数
  if(torque2 > 0){
    count2 ++;//現在のモーター2のトルクが正ならcount2を1増やす
  }else{
    count2 --;//現在のモーター2のトルクが負ならcount2を1減らす
  }
}

bool autoMove(int targetCount1, int targetCount2, float kp){//カウント1の目標，カウント2の目標，pゲイン
  int8_t dir1 = (targetCount1 > count1) ? 1 : -1;//タイヤ1が前に進むべきなら1，後ろに進むべきなら-1にする
  int8_t dir2 = (targetCount2 > count2) ? 1 : -1;//タイヤ2が前に進むべきなら1，後ろに進むべきなら-1にする
  MoveMotor(1, constrain(dir1 * 40 + kp*(targetCount1 - count1),-127,127));//差の2倍に20の下駄を履かせて出力する constrain関数でトルクを -127~127 の間に収める
  MoveMotor(2, constrain(dir2 * 40 + kp*(targetCount2 - count2),-127,127));//差の2倍に20の下駄を履かせて出力する constrain関数でトルクを -127~127 の間に収める
  
  if (count1 == targetCount1 && count2 == targetCount2) {//現在のエンコーダーのカウントが目標値と一致したときのみtrueを返す.
    return true;
  }else{
    return false;
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
  pinMode(echo,INPUT);
  attachInterrupt(digitalPinToInterrupt(enc1), _1_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2), _2_CHANGE, CHANGE);
  servo1.attach(servo1_pin, 500, 2400);
  servo2.attach(servo2_pin, 500, 2400);
  // Serial.begin(9600);
  servo1.write(90);
  servo2.write(0);

  c1 -= 660*a;
  c2 -= 660*a;
  push_c1 = c1;
  push_c2 = c2;
  c1 += 45*b;
  c2 -= 45*b;
  find1_c1 = c1;
  find1_c2 = c2;
  c1 += 70*b;
  c2 -= 70*b;
  carry1_c1 = c1;
  carry1_c2 = c2;
  c1 += 30*a;
  c2 += 30*a;
  before1_c1 = c1;
  before1_c2 = c2;
  c1 -= 30*a;
  c2 -= 30*a;
  after1_c1 = c1;
  after1_c2 = c2;
  c1 -= 115*b;
  c2 += 115*b;
  RotateCenter1_c1 = c1;
  RotateCenter1_c2 = c2;
  c1 -= 45*b;
  c2 += 45*b;
  find2_c1 = c1;
  find2_c2 = c2;
  c1 -= 70*b;
  c2 += 70*b;
  carry2_c1 = c1;
  carry2_c2 = c2;
  c1 += 30*a;
  c2 += 30*a;
  before2_c1 = c1;
  before2_c2 = c2;
  c1 -= 30*a;
  c2 -= 30*a;
  after2_c1 = c1;
  after2_c2 = c2;
  c1 += 115*b;
  c2 -= 115*b;
  RotateCenter2_c1 = c1;
  RotateCenter2_c2 = c2;
  c1 += 660*a;
  c2 += 660*a;
  return_c1 = c1;
  return_c2 = c2;
  // Serial.print(push_c1);  Serial.println(push_c2);
  // Serial.print(after2_c1);  Serial.println(after2_c2);
}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t value;

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
  // Serial.print(count1);Serial.print(" ");Serial.println(count2);
  if(isAutoMode == true){
    if (client.available() > 0) {//まずクライアントからの入力をチェック
      c = client.read();
      if (c == 'y') { // 'y' が押されたら
        isAutoMode = false;      // 手動モードに戻す
        MoveMotor(1, 0);         // 安全のためモーターを止める
        MoveMotor(2, 0);
        sprintf(message, "Manual"); // 手動モードになったことを表示
        LED_print(0, 1, message, NO_SCROLL);
        stage = ReturnManual;
        // stageMid = returnManual;
      }else if(c == 'U'){
        value = uint16_t(getTempC());
        client.write(highByte(value)); //上位バイト
        client.write(lowByte(value));  //下位バイト
      }
    }
    switch (stage) {
      case Push2Cups:
      {
        if(autoMove(int(push_c1),int(push_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
          isRotateRight = true;//次の段階の最初で右回転するように設定
          stage = RotateRight;
        }
        sprintf(message, "PUS");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

        break;
      }

      case Back:
      {
        if(autoMove(int(back_c1),int(back_c2),2)){
          isRotateRight = true;//次の段階の最初で右回転するように設定
          stage = RotateRight;
        }
        sprintf(message, "BAC");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;
      }
      
      case RotateRight:
      {
        // if(autoMove(-107,-147,2)){
        //   stage = CatchCup1;
        // }
        // while(getDistance > 10 && (count1 < -110 && count2 < -110)){//対物距離が10cm以上でかつ，回転しすぎていなければ実行する
        //   int count1Target = count1 -= 5;
        //   int count2Target = count2 -= 5;
        //   autoMove(count1Target, count2Target, 2);
        // }
        if(isRotateRight){
          if(autoMove(int(find1_c1)+10, int(find1_c2)-10, 2)){
            isRotateRight = false;
          }
        }else{
          if(autoMove(int(find1_c1) - 10, int(find1_c2)+10, 2)){
            isRotateRight = true;
          }
        }
        float distance = getDistance();
        if(3 < distance && distance < 10.0){//一定距離以内にものを確認したらそれを掴む段階に移る．
          stage = CatchCup1;
        }

        sprintf(message, "FI1");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;
      }

      case CatchCup1:
        sprintf(message, "CAT");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        MoveMotor(1,0);
        MoveMotor(2,0);
        servo2.write(0);//開く
        delay(1000);//1s待つ
        servo1.write(0);//下げる
        delay(1000);//1s待つ
        servo2.write(50);//掴む
        delay(1000);//1s待つ
        servo1.write(90);//上げる
        delay(1000);//1s待つ
        stage = CarryCup1;
        break;

      case CarryCup1://できれば回転のみで運ぶ
        if(autoMove(int(carry1_c1),int(carry1_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
          stage = BeforePut1;
        }
        sprintf(message, "CAR");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

        break;
      
      case BeforePut1://置く前に前に進む
        if(autoMove(int(before1_c1),int(before1_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
          stage = PutCup1;
        }
        sprintf(message, "Be1");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

        break;

      case PutCup1:
        sprintf(message, "PUT");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        MoveMotor(1,0);
        MoveMotor(2,0);
        servo1.write(0);//下げる
        delay(1000);//1s待つ
        servo2.write(0);//離す
        delay(1000);//1s待つ
        servo1.write(90);//上げる
        delay(1000);//1s待つ
        stage = AfterPut1;
        isRotateRight = false;
        break;
      
      case AfterPut1://置いた後に後ろに戻る
        if(autoMove(int(after1_c1),int(after1_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
          stage = RotateCenter1;
        }
        sprintf(message, "Af1");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

        break;

      case RotateCenter1:
      {
        if(autoMove(int(RotateCenter1_c1),int(RotateCenter1_c2),2)){
          stage = RotateLeft;
        }
        sprintf(message, "RoC");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;
      }

      case RotateLeft:{
        // if(autoMove(-163,-143,2)){
        //   stage = CatchCup2;
        // }
        if(!isRotateRight){//左に回るモード
          if(autoMove(int(find2_c1)-10, int(find2_c2)+10, 2)){
            isRotateRight = true;//右に回るモードにする
          }
        }else{//右に回るモード
          if(autoMove(int(find2_c1)+10, int(find2_c2)-10, 2)){
            isRotateRight = false;//左に回るモードにする
          }
        }
        float distance = getDistance();
        if(3 < distance && distance < 10.0){//一定距離以内にものを確認したらそれを掴む段階に移る．
          stage = CatchCup2;
        }
        sprintf(message, "FI2");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;
      }

      case CatchCup2:
        sprintf(message, "CAT");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        MoveMotor(1,0);
        MoveMotor(2,0);
        servo1.write(0);//下げる
        delay(1000);//1s待つ
        servo2.write(50);//掴む
        delay(1000);//1s待つ
        servo1.write(90);//上げる
        delay(1000);//1s待つ
        stage = CarryCup2;
        break;
      
      case CarryCup2:
        if(autoMove(int(carry2_c1),int(carry2_c2),2)){
          stage = BeforePut2;
        }
        sprintf(message, "CAR");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;

      case BeforePut2://置く前に前に進む
        if(autoMove(int(before2_c1),int(before2_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
          stage = PutCup2;
        }
        sprintf(message, "Be2");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

        break;

      case PutCup2:
        sprintf(message, "PUT");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        MoveMotor(1,0);
        MoveMotor(2,0);
        servo1.write(0);//下げる
        delay(1000);//1s待つ
        servo2.write(0);//離す
        delay(1000);//1s待つ
        servo1.write(90);//上げる
        delay(1000);//1s待つ
        stage = AfterPut2;
        break;

      case AfterPut2://置いた後に後ろに戻る
      if(autoMove(int(after2_c1),int(after2_c2),2)){//カウント1の目標，カウント2の目標，pゲイン
        stage = RotateCenter2;
      }
      sprintf(message, "Af2");
      LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示

      break;

      case RotateCenter2://まっすぐを向く
        if(autoMove(int(RotateCenter2_c1),int(RotateCenter2_c2),2)){
          stage = Return;
        }
        sprintf(message, "roC");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;
      
      case Return://初期位置に戻る
        if(autoMove(int(return_c1),int(return_c2),2)){
          stage = ReturnManual;
        }
        sprintf(message, "RET");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        break;

      case ReturnManual:
        isAutoMode = false;
        MoveMotor(1,0);
        MoveMotor(2,0);

        sprintf(message, "Man");
        LED_print(0, 1, message, NO_SCROLL); //受け取った文字をLEDに表示
        
        break;

      default:
        isAutoMode = false;
        break;
    }

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
        servo1.write(90);
        sprintf(message, "up ");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'l'://アームを下に
        servo1.write(0);
        sprintf(message, "down");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'j'://アームを開く
        servo2.write(0);
        sprintf(message, "open");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'k'://アームを閉じる
        servo2.write(50);
        sprintf(message, "close");
        LED_print(0, 1, message, NO_SCROLL);  //受け取った文字をLEDに表示
        break;

      case 'U'://温度センサ取得  
        value = uint16_t(getTempC());
        client.write(highByte(value)); //上位バイト
        client.write(lowByte(value));  //下位バイト
        break;

      case 'K'://距離センサ取得
        break;

      case 't':
        isAutoMode = true;
        stage = Push2Cups;
        // stageMid = rotL;
        torque1 = 0;
        torque2 = 0;
        count1 = 0;
        count2 = 0;
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
