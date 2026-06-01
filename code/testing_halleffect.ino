// #include <Servo.h>

// const int hallPin = 2;
// const int ledPin = 13;
// const int servoPin = 9;

// Servo servoMotor;

// int angle = 0;

// // timing
// unsigned long previousMillis = 0;
// const int stepDelay = 20; // speed control (lower = faster)

// // states
// enum State {
//   MOVING_FORWARD,
//   MOVING_BACK
// };

// State currentState = MOVING_FORWARD;

// void setup() {
//   pinMode(hallPin, INPUT);
//   pinMode(ledPin, OUTPUT);

//   Serial.begin(9600);

//   servoMotor.attach(servoPin);
//   servoMotor.write(angle);
// }

// void loop() {
//   int sensorState = digitalRead(hallPin);

//   // LED feedback
//   digitalWrite(ledPin, sensorState == LOW ? HIGH : LOW);

//   unsigned long currentMillis = millis();

//   if (currentMillis - previousMillis >= stepDelay) {
//     previousMillis = currentMillis;

//     if (currentState == MOVING_FORWARD) {
//       if (angle < 90) {
//         angle++;
//         servoMotor.write(angle);
//       } else {
//         // reached end → switch direction
//         currentState = MOVING_BACK;
//         Serial.println("Reached 90, reversing...");
//       }
//     }

//     else if (currentState == MOVING_BACK) {
//       if (sensorState == HIGH) {
//         // STOP when hall sensor detects magnet
//         servoMotor.write(angle);
//         Serial.println("Magnet detected, stopping.");
//       } else {
//         // keep moving back slowly
//         if (angle > 0) {
//           angle--;
//           servoMotor.write(angle);
//         }
//       }
//     }
//   }
// }

#include <Servo.h>

const int hallPin = 2;
const int ledPin = 13;
const int servoPin = 9;

Servo servoMotor;

int angle = 0;

// timing
unsigned long previousMillis = 0;
const int stepDelay = 20;

// states
enum State {
  MOVING_FORWARD,
  MOVING_BACK,
  WAIT_FOR_RESET
};

State currentState = MOVING_FORWARD;

void setup() {
  pinMode(hallPin, INPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);

  servoMotor.attach(servoPin);
  servoMotor.write(angle);
}

void loop() {
  int sensorState = digitalRead(hallPin);

  // LED feedback
  digitalWrite(ledPin, sensorState == LOW ? HIGH : LOW);

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= stepDelay) {
    previousMillis = currentMillis;

    switch (currentState) {

      case MOVING_FORWARD:
        if (angle < 90) {
          angle++;
          servoMotor.write(angle);
          Serial.println("Currently moving forward");
        } else {
          currentState = MOVING_BACK;
          Serial.println("Reached 90 → reversing");
        }
        break;

      case MOVING_BACK:
        if (sensorState == LOW) {
          // magnet detected → restart cycle
          Serial.println("Magnet detected → restarting");
          // currentState = WAIT_FOR_RESET;
          currentState = MOVING_FORWARD;
        } else {
          if (angle > 0) {
            angle--;
            servoMotor.write(angle);
          }
        }
        break;
    }
  }
}
