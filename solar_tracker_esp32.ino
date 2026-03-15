#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

Servo horizontal;
Servo vertical;

int servohori = 90;
int servovert = 90;

int servohoriLimitHigh = 175;
int servohoriLimitLow = 5;

int servovertLimitHigh = 175;
int servovertLimitLow = 5;

// LDR pins
int ldrTopLeft = 34;
int ldrTopRight = 35;
int ldrBottomRight = 32;
int ldrBottomLeft = 33;

// LED indicator
int ledPin = 4;

bool autoMode = true;

// WiFi
const char* ssid = "RedmiNote11";
const char* password = "iydiyd13";

// MQTT
const char* mqtt_server = "test.mosquitto.org";

const char* topic_horizontal = "/solartracker/s_horizontal";
const char* topic_vertical = "/solartracker/s_vertical";
const char* topic_auto = "/solartracker/auto_control";

const char* topic_tl = "/solartracker/ldrtopleft";
const char* topic_tr = "/solartracker/ldrtopright";
const char* topic_br = "/solartracker/ldrbottomright";
const char* topic_bl = "/solartracker/ldrbottomleft";

const char* topic_direction = "/solartracker/direction";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // AUTO / MANUAL
  if (String(topic) == topic_auto) {

    if (message == "1") {
      autoMode = true;
      digitalWrite(ledPin, HIGH);
    }

    if (message == "0") {
      autoMode = false;
      digitalWrite(ledPin, LOW);
    }
  }

  // MANUAL HORIZONTAL
  if (!autoMode && String(topic) == topic_horizontal) {

    servohori = message.toInt();
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);
    horizontal.write(servohori);
  }

  // MANUAL VERTICAL
  if (!autoMode && String(topic) == topic_vertical) {

    servovert = message.toInt();
    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
    vertical.write(servovert);
  }
}

// reconnect MQTT
void reconnect() {

  while (!client.connected()) {

    if (client.connect("ESP32SolarTracker")) {

      client.subscribe(topic_auto);
      client.subscribe(topic_horizontal);
      client.subscribe(topic_vertical);

    } else {

      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  horizontal.attach(18);
  vertical.attach(19);

  horizontal.write(servohori);
  vertical.write(servovert);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) reconnect();

  client.loop();

  int tl = analogRead(ldrTopLeft);
  int tr = analogRead(ldrTopRight);
  int br = analogRead(ldrBottomRight);
  int bl = analogRead(ldrBottomLeft);

  client.publish(topic_tl, String(tl).c_str());
  client.publish(topic_tr, String(tr).c_str());
  client.publish(topic_br, String(br).c_str());
  client.publish(topic_bl, String(bl).c_str());

  if (autoMode) {

    int tol = 90;

    int avt = (tl + tr) / 2;
    int avd = (bl + br) / 2;
    int avl = (tl + bl) / 2;
    int avr = (tr + br) / 2;

    int dvert = avt - avd;
    int dhoriz = avl - avr;

    String direction = "แสงอยู่ตรงกลาง";

    // ===== คำนวณ 8 ทิศ =====

    if (abs(dhoriz) < tol && abs(dvert) < tol) {
      direction = "แสงอยู่ตรงกลาง";
    }

    else if (abs(dhoriz) > tol && abs(dvert) <= tol) {

      if (dhoriz > 0)
        direction = "ตะวันออก";
      else
        direction = "ตะวันตก";
    }

    else if (abs(dvert) > tol && abs(dhoriz) <= tol) {

      if (dvert > 0)
        direction = "เหนือ";
      else
        direction = "ใต้";
    }

    else if (abs(dhoriz) > tol && abs(dvert) > tol) {

      if (dhoriz > 0 && dvert > 0)
        direction = "ตะวันออกเฉียงเหนือ";

      else if (dhoriz < 0 && dvert > 0)
        direction = "ตะวันตกเฉียงเหนือ";

      else if (dhoriz > 0 && dvert < 0)
        direction = "ตะวันออกเฉียงใต้";

      else if (dhoriz < 0 && dvert < 0)
        direction = "ตะวันตกเฉียงใต้";
    }

    // ===== ควบคุม Servo =====

    if (abs(dvert) > tol) {

      if (avt > avd)
        servovert--;
      else
        servovert++;

      servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
      vertical.write(servovert);
    }

    if (abs(dhoriz) > tol) {

      if (avl > avr)
        servohori++;
      else
        servohori--;

      servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);
      horizontal.write(servohori);
    }

    client.publish(topic_direction, direction.c_str());
  }

  delay(100);
}