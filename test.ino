#include <Arduino.h>
#include <Ticker.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Max72xxPanel.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <FirebaseESP8266.h>
#define DHTPIN  2
#define DHTTYPE DHT22
#define BELL  4 //Output bật chuông
#define OFF   5 //Input tắt chuông
#define PIN_LED 16
#define PIN_BUTTON 0
#define LED_ON() digitalWrite(PIN_LED, HIGH)
#define LED_OFF() digitalWrite(PIN_LED, LOW)
#define LED_TOGGLE() digitalWrite(PIN_LED, digitalRead(PIN_LED) ^ 0x01)
Ticker ticker;
WiFiUDP u;
NTPClient n(u, "2.asia.pool.ntp.org", 7 * 3600);
DHT dht(DHTPIN, DHTTYPE);
int pinCS = 15; // Attach CS to this pin, DIN to MOSI and CLK to SCK
int numberOfHorizontalDisplays = 1;  //số trong Hiển thị theo chiều ngang
int numberOfVerticalDisplays = 8;   // Số trong hiển thị theo chiều dọc.
int length;
int Hours, Minutes, Day, Month, Year;
int count = 0;
int Off = 0, Bell = 0;
int posHour, posMinute, posAlarm, posRepeat, posEnd;
float t, h;
unsigned long previousMillis = 0;
bool in_smartconfig = false;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays); // cấu hình matrix
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String str = "";  //Chuỗi hiển thị
String scroll = ""; //Chuỗi chạy chữ
String Data, AlarmFirebase, RepeatFirebase, HoursFirebase, MinutesFirebase;
int wait = 50; // thời gian chạy chữ.
int spacer = 1; // khoảng cách cách chữ
int width = 5 + spacer; // độ rộng của font là 5 fixel
FirebaseData TempData;
FirebaseData HumiData;
FirebaseData SetData;
FirebaseData GetData;
FirebaseData StrData;
void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(BELL, OUTPUT);
  pinMode(OFF, INPUT);
  digitalWrite(BELL, 0);
  ticker.attach(1, tick);
  Serial.println("Setup done");
  //  WiFi.disconnect();
  //  WiFi.begin("minhtrung", "minhtrung080397");
  //  Serial.println("");
  //  Serial.println("Dang ket noi");
  //  while ((!(WiFi.status() == WL_CONNECTED)))
  //  {
  //    Serial.print(".");
  //    delay(500);
  //  }
  //  Serial.println();
  //  Serial.print("Dia chi IP: ");
  //  Serial.println(WiFi.localIP());
  n.begin();
  dht.begin();
  Firebase.begin("smartclock-84809.Firebaseio.com", "fnj2eQGCwEAIri5F0HSOngKOV1DLW8M3phH5FXN1");
  Firebase.reconnectWiFi(true);
  matrix.setIntensity(5); // cài đặt giá trị độ tương phản từ 0 đến 15.
  matrix.setRotation(3);
  matrix.setCursor((numberOfVerticalDisplays * 8 - 0) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print("");
  matrix.write();
}
void loop()
{
  if (longPress())
  {
    enter_smartconfig();
    Serial.println("Enter smartconfig");
  }
  if (WiFi.status() == WL_CONNECTED && in_smartconfig && WiFi.smartConfigDone())
  {
    exit_smart();
    Serial.println("Connected, Exit smartconfig");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    unsigned long currentMillis = millis();
    Alarm();
    t = dht.readTemperature();
    h = dht.readHumidity();
    if (currentMillis - previousMillis >= 1000)
    {
      previousMillis = currentMillis;
      count++;
    }
    if (count >= 0 && count < 5)
    {
      Time(); //thời gian
    }
    if (count >= 5 && count < 6)
    {
      today();  //Thứ trong tuần
    }
    if (count >= 6 && count < 7)
    {
      date(); //Ngày/tháng/năm
    }
    if (count >= 7 && count < 8)
    {
      temperature();  //Nhiệt độ
    }
    if (count >= 8 && count < 9)
    {
      humidity(); //Độ ẩm
    }
    if (count >= 9)
    {
      scrollstr();  //Chạy chữ
      count = 0;
    }
  }
}
void temperature()
{
  str = String(t) + "oC";
  String strFirebase = String(t) + "°C";
  Firebase.setString(TempData, "/Temp", strFirebase);
  length = str.length() * width;
  //  for (i = numberOfVerticalDisplays * 8 - length; i > 0; i--)
  //  {
  //    str += " ";
  //  }
  matrix.setCursor((numberOfVerticalDisplays * 8 - length) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print(str);
  matrix.write();
}
void humidity()
{
  str =  String(h) + "%";
  Firebase.setString(HumiData, "/Humi", str);
  length = str.length() * width;
  //  for (i = numberOfVerticalDisplays * 8 - length; i > 0; i--)
  //  {
  //    str += " ";
  //  }
  matrix.setCursor((numberOfVerticalDisplays * 8 - length) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print(str);
  matrix.write();
}
void Time()
{
  str = n.getFormattedTime();
  length = str.length() * width;
  //  for (i = numberOfVerticalDisplays * 8 - length; i > 0; i--)
  //  {
  //    str += " ";
  //  }
  matrix.setCursor((numberOfVerticalDisplays * 8 - length) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print(str);
  matrix.write();
}
void today()
{
  str = weekDays[n.getDay()];
  length = str.length() * width;
  //  for (i = numberOfVerticalDisplays * 8 - length; i > 0; i--)
  //  {
  //    str += " ";
  //  }
  matrix.setCursor((numberOfVerticalDisplays * 8 - length) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print(str);
  matrix.write();
}
void date()
{
  unsigned long epochTime = n.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  Day = ptm->tm_mday;
  Month = ptm->tm_mon + 1;
  Year = ptm->tm_year + 1900;
  if (Day < 10)
  {
    str = "0" + String(Day) + "/";
  }
  else
  {
    str = String(Day) + "/";
  }
  if (Month < 10)
  {
    str += "0" + String(Month) + "/";
  }
  else
  {
    str += String(Month) + "/";
  }
  str += String(Year);
  length = str.length() * width;
  //  for (i = numberOfVerticalDisplays * 8 - length; i > 0; i--)
  //  {
  //    str += " ";
  //  }
  matrix.setCursor((numberOfVerticalDisplays * 8 - length) / 2, 0); // Center text
  matrix.fillScreen(LOW);
  matrix.print(str);
  matrix.write();
}
void scrollstr()
{
  Firebase.getString(StrData, "str");
  scroll = StrData.stringData();
  delay(500);
  for ( int i = 0 ; i < width * scroll.length() + matrix.width() - 1 - spacer; i++ )
  {
    matrix.fillScreen(LOW);
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
    while ( x + width - spacer >= 0 && letter >= 0 )
    {
      if ( letter < scroll.length() )
      {
        matrix.drawChar(x, y, scroll[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(wait);
  }
}
void Alarm()
{
  n.update();
  Hours = n.getHours();
  Minutes = n.getMinutes();
  if (digitalRead(OFF))
  {
    Off = 1;
  }
  getdata();
  findpos();
  findstr();
  if (AlarmFirebase == "ON")
  {
    if ((String(Hours) == HoursFirebase) && (String(Minutes) == MinutesFirebase) && (Off == 0))
    {
      Bell = 1;
    }
    if ((String(Hours) != HoursFirebase) || (String(Minutes) != MinutesFirebase))
    {
      Off = 0;
    }
  }
  if (Off == 1)
  {
    Bell = 0;
  }
  if (Bell == 1)
  {
    digitalWrite(BELL, HIGH);
  }
  if (Bell == 0)
  {
    digitalWrite(BELL, LOW);
    if (RepeatFirebase == "OFF")
    {
      AlarmFirebase = "OFF";
      String STR = "@" + HoursFirebase + "$" + MinutesFirebase + "%OFF&" + RepeatFirebase + "#";
      Firebase.setString(SetData, "Data", STR);
    }
  }
}
void findpos()
{
  posHour = posChar(Data, '@');
  posMinute = posChar(Data, '$');
  posAlarm = posChar(Data, '%');
  posRepeat = posChar(Data, '&');
  posEnd = posChar(Data, '#');
}
void findstr()
{
  HoursFirebase = Mid(Data, posHour + 1, posMinute - posHour - 1);
  MinutesFirebase = Mid(Data, posMinute + 1, posAlarm - posMinute - 1);
  AlarmFirebase = Mid(Data, posAlarm + 1, posRepeat - posAlarm - 1);
  RepeatFirebase = Mid(Data, posRepeat + 1, posEnd - posRepeat - 1);
}
void getdata()
{
  Firebase.getString(GetData, "Data");
  Data = GetData.stringData();
  delay(500);
}
int posChar(String data, char ch)
{
  int vitri;
  int j;
  for (j = 0; j < data.length(); j++)
  {
    if (data.charAt(j) == ch)
    {
      vitri = j;
      break;
    }
  }
  return vitri;
}
String Mid(String data, int BD, int num)
{
  int i;
  String KQ = "";
  for (i = BD; i < BD + num; i++)
  {
    KQ += data.charAt(i);
  }
  return KQ;
}
bool longPress()
{
  static int lastPress = 0;
  if (millis() - lastPress > 3000 && digitalRead(PIN_BUTTON) == 0)
  {
    return true;
  } else if (digitalRead(PIN_BUTTON) == 1)
  {
    lastPress = millis();
  }
  return false;
}
void tick()
{
  //toggle state
  int state = digitalRead(PIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(PIN_LED, !state);     // set pin to the opposite state
}
void enter_smartconfig()
{
  if (in_smartconfig == false)
  {
    in_smartconfig = true;
    ticker.attach(0.1, tick);
    WiFi.beginSmartConfig();
  }
}
void exit_smart()
{
  ticker.detach();
  LED_ON();
  in_smartconfig = false;
}
