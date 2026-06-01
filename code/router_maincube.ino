#include <Servo.h>
#include <WiFiNINA.h>

// -------- PINS --------
#define TRIG_PIN 3 // orange
#define ECHO_PIN 4 // white
#define HALL_PIN 2   
#define LED_PIN 13
#define SERVO_PIN A0

Servo servoMotor;

// -------- GLOBAL STATE --------
int currentAngle = 0;
int breathDirection = 1;
int lastState = -1;          // track previous state to detect transitions

// ultrasonic
float duration_us, distance_cm;

// -------- TIMING (millis) --------
unsigned long previousServoMillis = 0;
const int servoInterval = 20;

unsigned long previousBreathMillis = 0;
const int breathInterval = 30;

unsigned long previousPopMillis = 0;
unsigned long previousStretchMillis = 0;
const int stretchInterval = 30;          // tune for drama
bool stretchStarted = false;

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";
const uint16_t SERVER_PORT = 80;

WiFiServer server(SERVER_PORT);

// -------- SETUP --------
void setup() {
  //Serial.begin(9600);

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(currentAngle);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(HALL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Before Wifi
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(5000);

  // connectWiFi();
  // server.begin();
  // Serial.print("Server IP: ");
  // Serial.println(WiFi.localIP());

  // after wifi
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(5000);
}

// -------- HOMING FUNCTION --------
bool homeStep(int sensorState) {
  if (sensorState == HIGH) {
    digitalWrite(LED_PIN, LOW);
    unsigned long now = millis();
    if (now - previousServoMillis >= servoInterval) {
      previousServoMillis = now;
      if (currentAngle > 0) {
        currentAngle--;
        servoMotor.write(currentAngle);
      }
    }
    return false;
  } else {
    digitalWrite(LED_PIN, HIGH);
    return true;
  }
}

// -------- LOOP --------
void loop() {
  //runServer();

  // ---- ULTRASONIC READ ----
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration_us = pulseIn(ECHO_PIN, HIGH);
  distance_cm = 0.017 * duration_us;

  int sensorState = digitalRead(HALL_PIN);

  Serial.print("Distance: ");
  Serial.println(distance_cm);

  // ---- FIGURE OUT CURRENT STATE ----
  int state;
  if      (distance_cm > 60)                       state = 0;
  else if (distance_cm <= 60 && distance_cm > 40)  state = 1;
  else if (distance_cm <= 40 && distance_cm > 20)  state = 2;
  else                                              state = 3;

  // ---- ON STATE CHANGE: reset timers so new state starts fresh ----
  if (state != lastState) {
    unsigned long now = millis(); 
    previousServoMillis  = now;
    previousPopMillis    = now;
    previousStretchMillis = now;
    stretchStarted = false;
    lastState = state;
    Serial.print("→ entering state ");
    Serial.println(state);
  }

  // -------- STATES --------

  // ===== STATE 0: HOMING =====
  if (state == 0) {
    Serial.println("Homing...");
    homeStep(sensorState);
  }

  // ===== STATE 1: BREATHING =====
  else if (state == 1) {
    Serial.println("Breathing");
    digitalWrite(LED_PIN, LOW);
    if (currentAngle < 30) currentAngle = 30;
    unsigned long now = millis();
    if (now - previousBreathMillis >= breathInterval) {
      previousBreathMillis = now;
      currentAngle += breathDirection;
      if (currentAngle >= 45 || currentAngle <= 30) breathDirection *= -1;
      servoMotor.write(currentAngle);
    }
  }

  // ===== STATE 2: HOME → POP =====
  else if (state == 2) {
    Serial.println("State 2");
    bool isHome = homeStep(sensorState);
    if (isHome) {
      Serial.println("POP");
      unsigned long now = millis();
      if (now - previousPopMillis >= servoInterval) {
        previousPopMillis = now;
        currentAngle = 45;
        servoMotor.write(currentAngle);
      }
    }
  }

  // ===== STATE 3: HOME → SLOW STRETCH =====
  else if (state == 3) {
    Serial.println("State 3");

    if (!stretchStarted) {
      bool isHome = homeStep(sensorState);
      // Serial.print("isHome: "); Serial.println(isHome);
      if (isHome) {
        stretchStarted = true;  // home confirmed, never check again
      }
    } else {
      // home confirmed — just stretch, no homing interference
      Serial.print("Stretching... angle: "); Serial.println(currentAngle);
      unsigned long now = millis();
      if (now - previousStretchMillis >= stretchInterval) {
        previousStretchMillis = now;
        if (currentAngle < 46) {
          currentAngle++;
          servoMotor.write(currentAngle);
        }
      }
    }
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi... SSID: ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void runServer() {
  WiFiClient client = server.available();
  if (client) {
    Serial.print("[SERVER] Client connected from: ");
    Serial.println(client.remoteIP());
    processClient(client);
    client = server.available(); 
  }
}

void processClient(WiFiClient client) {
  if (!client || !client.connected()) return;
  if (!client.available()) return;

  client.setTimeout(50);

  String msg = client.readStringUntil('\n');
  msg.trim();
  if (msg.length() == 0) return;

  client.println(lastState);
}
