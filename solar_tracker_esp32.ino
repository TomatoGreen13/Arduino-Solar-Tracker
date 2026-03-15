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

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPublish = 0;
unsigned long lastMove = 0;

int publishDelay = 600;
int moveDelay = 100;

// ===== MQTT RECEIVE =====
void callback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

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

  if (!autoMode && String(topic) == topic_horizontal) {

    servohori = message.toInt();
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);
    horizontal.write(servohori);
  }

  if (!autoMode && String(topic) == topic_vertical) {

    servovert = message.toInt();
    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
    vertical.write(servovert);
  }
}

// ===== MQTT RECONNECT =====
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

// ===== LDR FILTER =====
int readLDR(int pin) {

  long total = 0;

  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delay(2);
  }

  return total / 10;
}

void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  int tl = readLDR(ldrTopLeft);
  int tr = readLDR(ldrTopRight);
  int br = readLDR(ldrBottomRight);
  int bl = readLDR(ldrBottomLeft);

  Serial.println("TL:" + String(tl) + " TR:" + String(tr) + " BL:" + String(bl) + " BR:" + String(br));

  if (millis() - lastPublish > publishDelay) {

    client.publish(topic_tl, String(tl).c_str());
    client.publish(topic_tr, String(tr).c_str());
    client.publish(topic_br, String(br).c_str());
    client.publish(topic_bl, String(bl).c_str());

    lastPublish = millis();
  }

  if (autoMode && millis() - lastMove > moveDelay) {

    int tol = 50;

    int avt = (tl + tr) / 2;
    int avd = (bl + br) / 2;

    int avl = (tl + bl) / 2;
    int avr = (tr + br) / 2;

    int dvert = avt - avd;
    int dhoriz = avl - avr;

    // Dead Zone
    if (abs(dvert) < tol) dvert = 0;
    if (abs(dhoriz) < tol) dhoriz = 0;

    int speed = 2;

    if (dvert > 0) servovert += speed;
    else if (dvert < 0) servovert -= speed;

    if (dhoriz > 0) servohori += speed;
    else if (dhoriz < 0) servohori -= speed;

    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);

    vertical.write(servovert);
    horizontal.write(servohori);

    lastMove = millis();
  }
}