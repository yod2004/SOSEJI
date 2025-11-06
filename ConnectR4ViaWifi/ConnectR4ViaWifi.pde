/* 
  Wifi client sample by Masato ISHIKAWA 2024
  Arduino counterpart: wifi_simple.ino
  手順：
  ・Arduino側のプログラムを書き込んで電源ON
  ・WiFiでSSID cafe_00が見えるようになるので接続
  　PASSは 123456789
  ・Processingのプログラム実行
  ・キーを押すと，ArduinoのLEDと，PC画面に表示される
  ・100msおきにアナログA0の値が表示される
  
　Reference:
  https://processing.org/reference/libraries/net/Client.html
*/

import processing.net.*;

int PORT = 80;
String IP = "192.48.56.1";
Client client;

byte[] byteBuffer = new byte[2]; //受信用
int t_prev; //タイマ用

void setup()
{ 
  size(200,200);
  background(0);
  textSize(40);
  textAlign(LEFT,TOP);
  
  println("connecting to "+IP+":"+PORT+"...");  
  client = new Client(this, IP, PORT); //サーバーIP:PORTに接続するクライアントを作成
  println("Client Started.");

  t_prev = millis();
}

void draw()
{   
  int high, low;
 
  //100msおきにセンサ値送信を要求
  if (millis()-t_prev > 100) {
    t_prev = millis();

    //2バイトの値を受信
    //注意: processingの変数はすべてsignedなので，
    //各データ下位8ビット以外をマスクしてやる必要がある
    client.write('U');
    while(client.available() < 2) {}//バッファが2つ溜まるまで待つ
    client.readBytes(byteBuffer);
    high = (byteBuffer[0] & 0xFF);
    low =  (byteBuffer[1] & 0xFF); 

    //下半分を消してセンサ値を表示
    fill(0); 
    rect(0,100,200,100);//画面下半分の領域
    fill(#ffffff);
    text(nfs(((high<<8)+low), 4), 10, 110);//整数部分は4桁で(10,110)に表示
  }
}

void keyPressed() {
    client.write(key);

    //上半分を消して押されたキーを表示
    fill(0); 
    rect(0,0,200,100);//画面の上半分の領域
    fill(#ffffff);
    text(key, 10, 10);      
}
