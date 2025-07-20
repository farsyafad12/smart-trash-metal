#include <Servo.h>

const int trigPin = 2;
const int echoPin = 3;
const int buzzerPin = 4;
const int servoPin = 5;
const int redPin = 6;
const int yellowPin = 7;
const int greenPin = 8;
const int metalPin = 9;

const int servoDefaultWrite = 85;

long duration;
int distance;
int metalDetected = 0;

Servo servo;

unsigned long detectedAt = 0;
bool isWaitingConfirmation = false;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(buzzerPin, OUTPUT);
  servo.attach(servoPin);

  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  pinMode(metalPin, INPUT);

  Serial.begin(9600);

  servo.write(servoDefaultWrite);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, HIGH);
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Jarak : ");
  Serial.print(distance);
  Serial.println(" cm");

  metalDetected = digitalRead(metalPin);
  Serial.print("Metal Detected: ");
  Serial.println(metalDetected);

  if (distance < 15) {
    if (!isWaitingConfirmation) {
      detectedAt = millis();
      isWaitingConfirmation = true;

      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, HIGH);
      digitalWrite(greenPin, LOW);
    } else {
      if (millis() - detectedAt >= 3000) {
        digitalWrite(redPin, HIGH);
        digitalWrite(yellowPin, LOW);
        digitalWrite(greenPin, LOW);

        if (metalDetected == HIGH) {
          servo.write(45);  // Arah logam
        } else {
          servo.write(145);  // Arah non-logam
        }

        digitalWrite(buzzerPin, HIGH);
        delay(3500);

        isWaitingConfirmation = false;
        servo.write(servoDefaultWrite);
        digitalWrite(buzzerPin, LOW);
      }
    }
  } else {
    isWaitingConfirmation = false;
    servo.write(servoDefaultWrite);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(redPin, LOW);
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, HIGH);
  }

  delay(250);
}
