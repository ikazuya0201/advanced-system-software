#include <M5Stack.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <Preferences.h>
#include <Wire.h>

const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "M5STACK_SETUP";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;
// wifi config store
Preferences preferences;
WebServer webServer(80);
int po;
// the setup routine runs once when M5Stack starts up
 
  
float pos_x, pos_y;
float vel_x, vel_y;
  

#define MPU9250_ADDR 0x68
#define MPU9250_AX  0x3B
#define MPU9250_AY  0x3D
#define MPU9250_AZ  0x3F
#define MPU9250_TP  0x41    //  data not used
#define MPU9250_GX  0x43
#define MPU9250_GY  0x45
#define MPU9250_GZ  0x47

struct PalletPosition {
  float position_x;
  float position_y;
  float position_z;
};
struct PalletInfo {
  uint8_t color;
  boolean reset_flag;
};
xQueueHandle PalletInfoQueue = xQueueCreate(1, sizeof(PalletInfo));
void task0(void* arg) {
  int cnt = 0;
  TickType_t xLastWakeTime;
  const uint32_t calledFrequency = 100;
  const TickType_t xFrequency = configTICK_RATE_HZ / calledFrequency;
  while (1) {
    po = (po + 1) % 1000;
    //printf("task2 thread_cnt=%ld\n", cnt++);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
void buttonTask(void* arg) {
  uint8_t buttonA_GPIO = 37;
  uint8_t buttonB_GPIO = 38;
  uint8_t buttonC_GPIO = 39;
  uint8_t COLOR_TYPE = 5;
  const uint32_t calledFrequency = 100;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = configTICK_RATE_HZ / calledFrequency;
  pinMode(buttonA_GPIO, INPUT);
  pinMode(buttonB_GPIO, INPUT);
  pinMode(buttonC_GPIO, INPUT);
  PalletInfo pallet_info;
  bool last_buttonA = false;
  while (1) {
    if (digitalRead(buttonA_GPIO) == 0){
      if(last_buttonA == true) pallet_info.color = (pallet_info.color + 1) % COLOR_TYPE;
    }
    last_buttonA = digitalRead(buttonA_GPIO);
    pallet_info.reset_flag = (digitalRead(buttonC_GPIO) == 0) ? true : false;
    xQueueSendToBack(PalletInfoQueue, &pallet_info, 0);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void task1(void* arg) {
  int cnt = 0;
  TickType_t xLastWakeTime;
  const uint32_t calledFrequency = 500;
  const TickType_t xFrequency = configTICK_RATE_HZ / calledFrequency;
  int16_t AccX, AccY, AccZ;
  int16_t Temp;
  int16_t GyroX, GyroY, GyroZ;
  delay(300);
  Wire.begin();

  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x6B);
  Wire.write(0x80);
  Wire.endTransmission();
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission();
  //  range of gyro
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x1B);
  Wire.write(0x18);
  Wire.endTransmission();

	float AccX_offset, AccY_offset, AccZ_offset, Temp_offset;
	float GyroX_offset, GyroY_offset, GyroZ_offset;
	for(int i = 0; i < 1000; i++){
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(MPU9250_AX);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU9250_ADDR, 14);
    while (Wire.available() < 14);
    //  get 14bytes
    int16_t tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    AccX_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    AccY_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    AccZ_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    Temp_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    GyroX_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    GyroY_offset += tmp;
    tmp = Wire.read() << 8;  tmp |= Wire.read();
    GyroZ_offset += tmp;
	}
	AccX_offset /= 1000.0;
	AccY_offset /= 1000.0;
	AccZ_offset /= 1000.0;
	Temp_offset /= 1000.0;
	GyroX_offset /= 1000.0;
	GyroY_offset /= 1000.0;
	GyroZ_offset /= 1000.0;
  while (1) {
    PalletInfo pallet_ret;
    //  send start address
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(MPU9250_AX);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU9250_ADDR, 14);
    while (Wire.available() < 14);
    //  get 14bytes
    AccX = Wire.read() << 8;  AccX |= Wire.read();
    AccY = Wire.read() << 8;  AccY |= Wire.read();
    AccZ = Wire.read() << 8;  AccZ |= Wire.read();
    Temp = Wire.read() << 8;  Temp |= Wire.read();  //  (Temp-12421)/340.0 [degC]
    GyroX = Wire.read() << 8; GyroX |= Wire.read();
    GyroY = Wire.read() << 8; GyroY |= Wire.read();
    GyroZ = Wire.read() << 8; GyroZ |= Wire.read();
    //  debug monitor
    const float factor = 16384.0 / 9.8;
    vel_x += (AccX - AccX_offset) / factor / calledFrequency * 100.0;
    vel_y += (AccY - AccY_offset) / factor / calledFrequency * 100.0;
    float threshold = 100;
    if(vel_x > threshold) vel_x = threshold;
    else if(vel_x < -threshold) vel_x = -threshold;
    if(vel_y > threshold) vel_y = threshold;
    else if(vel_y < -threshold) vel_y = -threshold;
    pos_x += vel_x / calledFrequency;
    pos_y += vel_y / calledFrequency;
    xQueueReceive(PalletInfoQueue, &pallet_ret, 0);
    //printf("%d, %d, %d, %d, %d\n", AccX, AccY, AccZ, pallet_ret.color * 2000, pallet_ret.reset_flag * -1230);
    //printf("%f, %f, %f, %f, %f, %f\n", AccX - AccX_offset, AccY - AccY_offset, vel_x, vel_y, pos_x, pos_y);
    printf("%f, %f\n", vel_x, vel_y);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");
  Serial.print("WIFI-SSID: ");
  M5.Lcd.print("WIFI-SSID: ");
  Serial.println(wifi_ssid);
  M5.Lcd.println(wifi_ssid);
  Serial.print("WIFI-PASSWD: ");
  M5.Lcd.print("WIFI-PASSWD: ");
  Serial.println(wifi_password);
  M5.Lcd.println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if (wifi_ssid.length() > 0) {
    return true;
  } else {
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  M5.Lcd.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      M5.Lcd.println();
      Serial.println("Connected!");
      M5.Lcd.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    M5.Lcd.print(".");
    count++;
  }
  Serial.println("Timed out.");
  M5.Lcd.println("Timed out.");
  return false;
}
void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    M5.Lcd.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    M5.Lcd.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.sendHeader("Access-Control-Allow-Origin", "*", false);
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });    
    webServer.on("/position", []() {
      String s = "<h1>";
      s += String(po);
      s += "</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.sendHeader("Access-Control-Allow-Origin", "*", false);
      //webServer.send(200, "application/json", makePage("Wi-Fi Settings", s));
      String info = "{\"x\":";
      info += String(pos_x) + String(",");
      info += String("\"y\":");
      info += String(pos_y);
      info += String("}");
      webServer.send(200, "application/json", info);
    });
    webServer.on("/setap", []() {
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      M5.Lcd.print("SSID: ");
      Serial.println(ssid);
      M5.Lcd.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      M5.Lcd.print("Password: ");
      Serial.println(pass);
      M5.Lcd.println(pass);
      Serial.println("Writing SSID to EEPROM...");
      M5.Lcd.println("Writing SSID to EEPROM...");

      // Store wifi config
      Serial.println("Writing Password to nvr...");
      M5.Lcd.println("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);

      Serial.println("Write nvr done!");
      M5.Lcd.println("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    M5.Lcd.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    M5.Lcd.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  M5.Lcd.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);

  startWebServer();
  Serial.print("Starting Access Point at \"");
  M5.Lcd.print("Starting Access Point at \"");
  Serial.print(apSSID);
  M5.Lcd.print(apSSID);
  Serial.println("\"");
  M5.Lcd.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  //s += String(po);
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
void serverTask(void* arg) {

  const uint32_t calledFrequency = 100;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = configTICK_RATE_HZ / calledFrequency;
    if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();
  while (1) {
    webServer.handleClient();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
void setup() {

  // Initialize the M5Stack object
  M5.begin();
  // LCD display
  M5.Lcd.print("Hello World!");
  M5.Lcd.print("M5Stack is running successfully!");
  xTaskCreatePinnedToCore(task0, "Task0", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(task1, "Task1", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(buttonTask, "ButonTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(serverTask, "ServerTask", 4096, NULL, 4, NULL, 1);

}

// the loop routine runs over and over again forever
void loop() {

}
