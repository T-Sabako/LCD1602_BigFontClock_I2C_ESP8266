#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <time.h>
#define B 0x20
#define F 0xFF
#define C 0xA5

LiquidCrystal_I2C lcd(0x27, 16, 2);
// NodeMCU Dev Kit => D1 = SCL, D2 = SDA

const char *Mon[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
const char *Day[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

//Wi-Fi
#define ssid1 "hogeID1"
#define password1 "hogePW1"
#define ssid2 "hogeID2"
#define password2 "hogePW2"
String newHostname = "Clock1602LCD";

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
ESP8266WiFiMulti wifiMulti;

//NTP client
int timezone = 9;
char ntp_server1[20] = "ntp.nict.jp";
char ntp_server2[20] = "ntp.jst.mfeed.ad.jp";
char ntp_server3[20] = "time.aws.com";
int dst = 0;

int msecond, second, minute, hour, dayOfWeek, dayOfMonth, month, year, dots;

unsigned long lastSecond = millis();
unsigned long currentMillis = 0;
unsigned long lastUpdateTime = 0;
unsigned long lastUpdateData = 0;
String upperSec = "0";
String underSec = "0";

//custom charactor
byte cc1[8] = {0x07,0x0F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};//binarycode
byte cc2[8] = {0x1F,0x1F,0x1F,0x00,0x00,0x00,0x00,0x00};
byte cc3[8] = {0x1C,0x1E,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
byte cc4[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x0F,0x07};
byte cc5[8] = {0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F};
byte cc6[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1E,0x1C};
byte cc7[8] = {0x1F,0x1F,0x1F,0x00,0x00,0x00,0x1F,0x1F};
byte cc8[8] = {0x1F,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F};

// send custom characters to the display

void initWiFi() {
  WiFi.hostname(newHostname.c_str());
  WiFi.mode(WIFI_STA);
//  WiFi.begin(ssid1, password1);
  wifiMulti.addAP(ssid1,password1);
  wifiMulti.addAP(ssid2,password2);
  Serial.print("Connecting to WiFi ..");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi sucessfully.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  Serial.printf("hostname: %s\n", WiFi.hostname().c_str());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi, trying to connect...");
  WiFi.disconnect();
  WiFi.hostname(newHostname.c_str());
  while (wifiMulti.run() != WL_CONNECTED) {
  Serial.print('.');
  delay(1000);
  }
}


void DefineLargeChar() {
    lcd.createChar(1, cc1);
    lcd.createChar(2, cc2);
    lcd.createChar(3, cc3);
    lcd.createChar(4, cc4);
    lcd.createChar(5, cc5);
    lcd.createChar(6, cc6);
    lcd.createChar(7, cc7);
    lcd.createChar(8, cc8);
}

// 0 1 2 3 4 5 6 7 8 9
char bn1[] = {
  1, 2, 3,  2, 3, B,  2, 7, 3,  2, 7, 3,  4, 5, F,  F, 7, 7,  1, 7, 7,  2, 2, 6,  1, 7, 3,  1, 7, 3
};
char bn2[] = {
  4, 5, 6,  B, F, B,  1, 8, 8,  5, 8, 6,  B, B, F,  8, 8, 6,  4, 8, 6,  B, 1, B,  4, 8, 6,  5, 5, 6
};


void printTwoNumber(uint8_t number, uint8_t position)//13
{
  // Print position is hardcoded
  int digit0; // To represent the ones
  int digit1; // To represent the tens
  digit0 = number % 10;
  digit1 = number / 10;

  // Line 1 of the two-digit number
  lcd.setCursor(position, 0);
  lcd.write(bn1[digit1 * 3]);
  lcd.write(bn1[digit1 * 3 + 1]);
  lcd.write(bn1[digit1 * 3 + 2]);
  //lcd.write(B); // Blank
  lcd.write(bn1[digit0 * 3]);
  lcd.write(bn1[digit0 * 3 + 1]);
  lcd.write(bn1[digit0 * 3 + 2]);

  // Line 2 of the two-digit number
  lcd.setCursor(position, 1);
  lcd.write(bn2[digit1 * 3]);
  lcd.write(bn2[digit1 * 3 + 1]);
  lcd.write(bn2[digit1 * 3 + 2]);
  //lcd.write(B); // Blank
  lcd.write(bn2[digit0 * 3]);
  lcd.write(bn2[digit0 * 3 + 1]);
  lcd.write(bn2[digit0 * 3 + 2]);
}

void printColons(uint8_t position)
{
  lcd.setCursor(position, 0);
  lcd.write (C);
  lcd.setCursor(position, 1);
  lcd.write (C);
}

void printNoColons(uint8_t position)
{
  lcd.setCursor(position, 0);
  lcd.write (B);
  lcd.setCursor(position, 1);
  lcd.write (B);
}

void setup() {
  Serial.begin(115200);

  //Register event handlers
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
   
  initWiFi();

  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }

  lcd.init();
  lcd.backlight();
  DefineLargeChar(); // Create the custom characters
  lcd.clear();

  updateTime();
  updateData();
}

void loop() {
  currentMillis = millis();

  if ((currentMillis - lastUpdateTime) > 300000) updateTime();
  if ((currentMillis - lastUpdateData) > 600000) updateData();
  if (year < 2000) updateTime();

  dots = (currentMillis % 1000) < 500;
  if (dots) printColons(8); else printNoColons(8);

  if ((millis() - lastSecond) > 1000) {
    lastSecond = millis();
    second++;
    if (second > 59) {
      second = 0;
      minute++;
      if (minute > 59) {
        minute = 0;
        hour++;
        if (hour > 23) hour = 0;
      }
    }

    if (second < 10) {
      upperSec = "0";
      underSec = (String(second)).substring(0, 1);
    } else {
      upperSec = (String(second)).substring(0, 1);
      underSec = (String(second)).substring(1, 2);
    }

    printTwoNumber(hour, 2);
    printTwoNumber(minute, 9);
    lcd.setCursor(15, 0); lcd.print(upperSec);
    lcd.setCursor(15, 1); lcd.print(underSec);
    lcd.setCursor(0,0); lcd.print(month);
    lcd.setCursor(0,1); lcd.print(dayOfMonth);
  }
}

void updateData() {
  lastUpdateData = millis();
}

void updateTime() {
  lastUpdateTime = millis();
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);

  year = String(newtime->tm_year + 1900).toInt();
  month = String(newtime->tm_mon + 1).toInt();
  dayOfMonth = String(newtime->tm_mday).toInt();
  dayOfWeek = String(newtime->tm_wday).toInt();
  hour = String(newtime->tm_hour).toInt();
  minute = String(newtime->tm_min).toInt();
  second = String(newtime->tm_sec).toInt();
}
