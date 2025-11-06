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

// Communication setup
char c = '.';

void setup() {
  // put your setup code here, to run once:
  matrix.begin();      // start LED matrix
  wifi_setup();        // start wifi
  pinMode(A0, INPUT);  // set analog input
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
      
      break;
    case 's'://後退
      
      break;
    case 'd'://右回転
      
      break;
    case 'a'://左回転
      
      break;
    cae 'p'://アーム上下
      
      break;
    case 'l'://アーム開閉
      
      break;


    case 'o'://温度センサ取得  
      value = analogRead(A0);
      client.write(highByte(value)); //上位バイト
      client.write(lowByte(value));  //下位バイト
      break;

    case 'k'://距離センサ取得
      break;
    
    default:
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
