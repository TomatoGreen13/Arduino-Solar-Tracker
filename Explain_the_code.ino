#include <WiFi.h>          // เรียกใช้ไลบรารี WiFi สำหรับ ESP32
#include <PubSubClient.h>  // ไลบรารีสำหรับใช้งาน MQTT
#include <ESP32Servo.h>    // ไลบรารีควบคุม Servo บน ESP32

Servo horizontal;  // สร้างอ็อบเจกต์ servo สำหรับหมุนแนวนอน (ซ้าย-ขวา)
Servo vertical;    // สร้างอ็อบเจกต์ servo สำหรับหมุนแนวตั้ง (ขึ้น-ลง)

int servohori = 90;  // กำหนดมุมเริ่มต้นของ servo แนวนอน = 90 องศา
int servovert = 90;  // กำหนดมุมเริ่มต้นของ servo แนวตั้ง = 90 องศา

int servohoriLimitHigh = 175;  // กำหนดมุมสูงสุดของ servo แนวนอน
int servohoriLimitLow = 5;     // กำหนดมุมต่ำสุดของ servo แนวนอน

int servovertLimitHigh = 175;  // กำหนดมุมสูงสุดของ servo แนวตั้ง
int servovertLimitLow = 5;     // กำหนดมุมต่ำสุดของ servo แนวตั้ง

// LDR pins
int ldrTopLeft = 34;      // กำหนดขา ESP32 ที่ต่อกับ LDR ด้านบนซ้าย
int ldrTopRight = 35;     // กำหนดขา ESP32 ที่ต่อกับ LDR ด้านบนขวา
int ldrBottomRight = 32;  // กำหนดขา ESP32 ที่ต่อกับ LDR ด้านล่างขวา
int ldrBottomLeft = 33;   // กำหนดขา ESP32 ที่ต่อกับ LDR ด้านล่างซ้าย

// LED indicator
int ledPin = 4;  // กำหนดขา LED สำหรับแสดงสถานะโหมด

bool autoMode = true;  // ตัวแปรกำหนดโหมดการทำงาน true = Auto / false = Manual

// WiFi
const char* ssid = "RedmiNote11";   // ชื่อ WiFi ที่จะเชื่อมต่อ
const char* password = "iydiyd13";  // รหัสผ่าน WiFi

// MQTT
const char* mqtt_server = "test.mosquitto.org";  // ที่อยู่ MQTT Server

const char* topic_horizontal = "/solartracker/s_horizontal";  // topic สำหรับรับคำสั่ง servo แนวนอน
const char* topic_vertical = "/solartracker/s_vertical";      // topic สำหรับรับคำสั่ง servo แนวตั้ง
const char* topic_auto = "/solartracker/auto_control";        // topic สำหรับสั่งเปิด/ปิด Auto mode

const char* topic_tl = "/solartracker/ldrtopleft";      // topic ส่งค่าความสว่าง LDR บนซ้าย
const char* topic_tr = "/solartracker/ldrtopright";     // topic ส่งค่าความสว่าง LDR บนขวา
const char* topic_br = "/solartracker/ldrbottomright";  // topic ส่งค่าความสว่าง LDR ล่างขวา
const char* topic_bl = "/solartracker/ldrbottomleft";   // topic ส่งค่าความสว่าง LDR ล่างซ้าย

WiFiClient espClient;            // สร้าง client สำหรับเชื่อม WiFi
PubSubClient client(espClient);  // สร้าง MQTT client โดยใช้ WiFi client

unsigned long lastPublish = 0;  // ตัวแปรเก็บเวลาล่าสุดที่ส่งข้อมูล MQTT
unsigned long lastMove = 0;     // ตัวแปรเก็บเวลาล่าสุดที่ servo ขยับ

int publishDelay = 600;  // ระยะเวลาในการส่งข้อมูล MQTT (600 ms)
int moveDelay = 100;     // ระยะเวลาในการขยับ servo (100 ms)

// ===== MQTT RECEIVE =====
void callback(char* topic, byte* payload, unsigned int length) {  // ฟังก์ชันทำงานเมื่อมีข้อความจาก MQTT

  String message = "";  // สร้างตัวแปรเก็บข้อความที่รับมา

  for (int i = 0; i < length; i++) {  // วนลูปอ่านข้อมูลที่ส่งมา
    message += (char)payload[i];      // แปลงข้อมูล byte เป็นตัวอักษรแล้วรวมเป็น String
  }

  if (String(topic) == topic_auto) {  // ถ้า topic ที่รับมาตรงกับ auto control

    if (message == "1") {          // ถ้าข้อความคือ 1
      autoMode = true;             // เปิดโหมด Auto
      digitalWrite(ledPin, HIGH);  // เปิด LED แสดงสถานะ
    }

    if (message == "0") {         // ถ้าข้อความคือ 0
      autoMode = false;           // ปิด Auto mode
      digitalWrite(ledPin, LOW);  // ปิด LED
    }
  }

  if (!autoMode && String(topic) == topic_horizontal) {  // ถ้าอยู่ Manual mode และเป็นคำสั่งแนวนอน

    servohori = message.toInt();                                              // แปลงข้อความเป็นตัวเลขมุม servo
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);  // จำกัดมุมให้อยู่ในช่วงที่กำหนด
    horizontal.write(servohori);                                              // สั่ง servo หมุนตามมุม
  }

  if (!autoMode && String(topic) == topic_vertical) {  // ถ้าอยู่ Manual mode และเป็นคำสั่งแนวตั้ง

    servovert = message.toInt();                                              // แปลงข้อความเป็นตัวเลขมุม
    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);  // จำกัดมุม
    vertical.write(servovert);                                                // สั่ง servo หมุน
  }
}

// ===== MQTT RECONNECT =====
void reconnect() {  // ฟังก์ชันเชื่อมต่อ MQTT ใหม่เมื่อหลุด

  while (!client.connected()) {  // ถ้ายังไม่เชื่อมต่อ

    if (client.connect("ESP32SolarTracker")) {  // พยายามเชื่อมต่อ MQTT ด้วยชื่อ client

      client.subscribe(topic_auto);        // subscribe topic auto mode
      client.subscribe(topic_horizontal);  // subscribe topic servo แนวนอน
      client.subscribe(topic_vertical);    // subscribe topic servo แนวตั้ง

    } else {

      delay(2000);  // ถ้าเชื่อมไม่สำเร็จให้รอ 2 วินาทีแล้วลองใหม่
    }
  }
}

void setup() {  // ฟังก์ชัน setup ทำงานครั้งเดียวตอนเปิดเครื่อง

  Serial.begin(115200);  // เปิด Serial monitor ความเร็ว 115200

  horizontal.attach(18);  // ต่อ servo แนวนอนที่ขา GPIO18
  vertical.attach(19);    // ต่อ servo แนวตั้งที่ขา GPIO19

  horizontal.write(servohori);  // ตั้งตำแหน่งเริ่มต้น servo แนวนอน
  vertical.write(servovert);    // ตั้งตำแหน่งเริ่มต้น servo แนวตั้ง

  pinMode(ledPin, OUTPUT);     // กำหนดขา LED เป็น OUTPUT
  digitalWrite(ledPin, HIGH);  // เปิด LED (เริ่มต้น Auto mode)

  WiFi.begin(ssid, password);  // เริ่มเชื่อมต่อ WiFi

  while (WiFi.status() != WL_CONNECTED) {  // ถ้ายังไม่เชื่อม WiFi
    delay(500);                            // รอ 0.5 วินาที
    Serial.print(".");                     // แสดงจุดใน serial เพื่อบอกว่ากำลังเชื่อม
  }

  Serial.println("WiFi connected");  // แจ้งว่าเชื่อม WiFi สำเร็จ

  client.setServer(mqtt_server, 1883);  // กำหนด MQTT server และ port
  client.setCallback(callback);         // กำหนดฟังก์ชัน callback เมื่อมีข้อความเข้ามา
}

// ===== LDR FILTER =====
int readLDR(int pin) {  // ฟังก์ชันอ่านค่า LDR พร้อมกรองค่า

  long total = 0;  // ตัวแปรเก็บผลรวมค่าที่อ่านได้

  for (int i = 0; i < 10; i++) {  // อ่านค่า 10 ครั้ง
    total += analogRead(pin);     // อ่านค่าแสงจาก LDR แล้วรวมค่า
    delay(2);                     // หน่วงเวลาเล็กน้อย
  }

  return total / 10;  // คืนค่าเฉลี่ยของการอ่านทั้ง 10 ครั้ง
}

void loop() {  // ฟังก์ชัน loop ทำงานซ้ำตลอดเวลา

  if (!client.connected()) reconnect();  // ถ้า MQTT หลุดให้เชื่อมใหม่
  client.loop();                         // ให้ MQTT client ทำงานรับส่งข้อมูล

  int tl = readLDR(ldrTopLeft);      // อ่านค่า LDR บนซ้าย
  int tr = readLDR(ldrTopRight);     // อ่านค่า LDR บนขวา
  int br = readLDR(ldrBottomRight);  // อ่านค่า LDR ล่างขวา
  int bl = readLDR(ldrBottomLeft);   // อ่านค่า LDR ล่างซ้าย

  Serial.println("TL:" + String(tl) + " TR:" + String(tr) + " BL:" + String(bl) + " BR:" + String(br));  // แสดงค่าบน serial

  if (millis() - lastPublish > publishDelay) {  // ถ้าถึงเวลาส่งข้อมูล MQTT

    client.publish(topic_tl, String(tl).c_str());  // ส่งค่า TL ไป MQTT
    client.publish(topic_tr, String(tr).c_str());  // ส่งค่า TR
    client.publish(topic_br, String(br).c_str());  // ส่งค่า BR
    client.publish(topic_bl, String(bl).c_str());  // ส่งค่า BL

    lastPublish = millis();  // บันทึกเวลาที่ส่งข้อมูลล่าสุด
  }

  if (autoMode && millis() - lastMove > moveDelay) {  // ถ้าอยู่ Auto mode และถึงเวลาขยับ servo

    int tol = 50;  // กำหนดค่า dead zone เพื่อลดการสั่นของ servo

    int avt = (tl + tr) / 2;  // ค่าเฉลี่ยแสงด้านบน
    int avd = (bl + br) / 2;  // ค่าเฉลี่ยแสงด้านล่าง

    int avl = (tl + bl) / 2;  // ค่าเฉลี่ยแสงด้านซ้าย
    int avr = (tr + br) / 2;  // ค่าเฉลี่ยแสงด้านขวา

    int dvert = avt - avd;   // คำนวณความต่างแสงแนวตั้ง
    int dhoriz = avl - avr;  // คำนวณความต่างแสงแนวนอน

    if (abs(dvert) < tol) dvert = 0;    // ถ้าค่าต่างต่ำกว่า tolerance ให้ถือว่าเท่ากัน
    if (abs(dhoriz) < tol) dhoriz = 0;  // ป้องกัน servo สั่น

    int speed = 2;  // กำหนดความเร็วในการหมุน servo

    if (dvert > 0) servovert += speed;       // ถ้าแสงบนมากกว่าให้หมุนขึ้น
    else if (dvert < 0) servovert -= speed;  // ถ้าแสงล่างมากกว่าให้หมุนลง

    if (dhoriz > 0) servohori += speed;       // ถ้าแสงซ้ายมากกว่าให้หมุนซ้าย
    else if (dhoriz < 0) servohori -= speed;  // ถ้าแสงขวามากกว่าให้หมุนขวา

    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);  // จำกัดมุมแนวตั้ง
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);  // จำกัดมุมแนวนอน

    vertical.write(servovert);    // สั่ง servo แนวตั้งหมุน
    horizontal.write(servohori);  // สั่ง servo แนวนอนหมุน

    lastMove = millis();  // บันทึกเวลาที่ servo ขยับล่าสุด
  }
}