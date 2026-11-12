#include <Arduino.h>
#include <AccelStepper.h>

const int TRIG_PIN = 12;
const int ECHO_PIN = 14;

const int IN1_PIN = 27;
const int IN2_PIN = 26;
const int IN3_PIN = 25;
const int IN4_PIN = 33;
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN,IN3_PIN,IN2_PIN,IN4_PIN);



// Define sound speed in cm/microsecond
#define SOUND_SPEED 0.034


long duration;
float distanceCm;

unsigned long lastPing = 0;


void setup() {
  Serial.begin(9600); 
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT); 
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(200);
}

void loop() {
  if(millis() - lastPing >= 500){
    lastPing = millis();
    // Clear the TRIG_PIN by setting it LOW for 2 microseconds
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    // Trigger the sensor by setting the TRIG_PIN HIGH for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    // Read the ECHO_PIN, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    // Calculate the distance
    distanceCm = duration * SOUND_SPEED / 2; // 0 means no echo
    
    // Print the distance to the Serial Monitor
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
  }
    
  if (stepper.distanceToGo() == 0){
    stepper.moveTo(stepper.currentPosition() == 0 ? 2048: 0); // 2048 steps is ~ 1 turn
  }
  stepper.run();
}

