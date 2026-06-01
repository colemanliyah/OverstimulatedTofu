#include <WiFiNINA.h>
#include <Servo.h>

const char* WIFI_SSID   = "";
const char* WIFI_PASS   = "";
const char* SERVER_IP   = "10.23.11.218";
const uint16_t SERVER_PORT = 80;
const uint8_t CLIENT_ID = 1; // Change for each arduino

WiFiClient client;

// -------- PINS --------
#define HALL_PIN  2
#define LED_PIN   13
#define SERVO_PIN 9

Servo servoMotor;

// -------- GLOBAL STATE --------
int currentAngle   = 0;
int breathDirection = 1;
int lastState      = -1;

// -------- TIMING --------
unsigned long previousServoMillis   = 0;
const int servoInterval             = 20;

unsigned long previousBreathMillis  = 0;
const int breathInterval            = 30;

unsigned long previousPopMillis     = 0;

unsigned long previousStretchMillis = 0;
const int stretchInterval           = 30;
bool stretchStarted                 = false;

// -------- HOMING --------
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

// -------- RUN STATE --------
void runState(int state) {
  int sensorState = digitalRead(HALL_PIN);

  if (state != lastState) {
    unsigned long now    = millis();
    previousServoMillis  = now;
    previousPopMillis    = now;
    previousStretchMillis = now;
    stretchStarted       = false;
    lastState            = state;
  }

  if (state == 0) {
    homeStep(sensorState);
  }
  else if (state == 1) {
    digitalWrite(LED_PIN, LOW);
    if (currentAngle < 15) currentAngle = 15;
    unsigned long now = millis();
    if (now - previousBreathMillis >= breathInterval) {
      previousBreathMillis = now;
      currentAngle += breathDirection;
      if (currentAngle >= 30 || currentAngle <= 15) breathDirection *= -1;
      servoMotor.write(currentAngle);
    }
  }
  else if (state == 2) {
    bool isHome = homeStep(sensorState);
    if (isHome) {
      unsigned long now = millis();
      if (now - previousPopMillis >= servoInterval) {
        previousPopMillis = now;
        currentAngle = 25;
        servoMotor.write(currentAngle);
      }
    }
  }
  else if (state == 3) {
    if (!stretchStarted) {
      bool isHome = homeStep(sensorState);
      if (isHome) stretchStarted = true;
    } else {
      unsigned long now = millis();
      if (now - previousStretchMillis >= stretchInterval) {
        previousStretchMillis = now;
        if (currentAngle < 26) {
          currentAngle++;
          servoMotor.write(currentAngle);
        }
      }
    }
  }
}

// -------- WIFI --------
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

void runClient() {
  // CHANGED: cachedState persists between calls so runState() runs every loop
  static int cachedState = -1;
  // CHANGED: track last poll time so we don't hammer the server every loop
  static unsigned long lastPollMillis = 0;
  const unsigned long pollInterval = 100;

  // CHANGED: run state machine every loop using cached state, not just when
  //          a new network response arrives
  if (cachedState >= 0) runState(cachedState);

  // CHANGED: only do network work every 200ms, not every loop
  unsigned long now = millis();
  if (now - lastPollMillis < pollInterval) return;
  lastPollMillis = now;

  // Reconnect if needed — same as before, just moved below the state machine
  if (!client.connected()) {
    client.stop();
    Serial.print("[CLIENT] Trying to connect to server: ");
    Serial.print(SERVER_IP); Serial.print(":"); Serial.println(SERVER_PORT);

    if (client.connect(SERVER_IP, SERVER_PORT)) {
      Serial.println("[CLIENT] Connected to server!");
      digitalWrite(LED_BUILTIN, HIGH); delay(500);
      digitalWrite(LED_BUILTIN, LOW);  delay(500);
      client.setTimeout(50);
    } else {
      Serial.print("Current Client IP address: "); Serial.println(WiFi.localIP());
      Serial.println("[CLIENT] Failed to connect, retrying in 5 seconds");
      delay(5000);
      return;
    }
  }

  // Send — unchanged
  if (client.connected()) {
    String cmd = "YOUR_MESSAGE_HERE";
    cmd += CLIENT_ID;
    client.println(cmd);
    client.flush();
  }

  // CHANGED: store response in cachedState instead of calling runState() here
  if (client.available()) {
    String resp = client.readStringUntil('\n');
    resp.trim();
    if (resp.length() > 0) {
      cachedState = resp.toInt(); // CHANGED: was runState(resp.toInt())
      Serial.print("[CLIENT] Received state: ");
      Serial.println(cachedState);
    }
  }
}

// -------- SETUP --------
void setup() {
  //Serial.begin(9600);
  pinMode(HALL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(currentAngle);

  digitalWrite(LED_BUILTIN, HIGH); delay(5000);
  digitalWrite(LED_BUILTIN, LOW);  delay(5000);

  connectWiFi();

  digitalWrite(LED_BUILTIN, HIGH); delay(5000);
  digitalWrite(LED_BUILTIN, LOW);  delay(5000);
}

// -------- LOOP --------
void loop() {
  runClient();
}
