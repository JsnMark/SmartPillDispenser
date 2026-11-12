#include <Arduino.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include "secrets.h"

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
unsigned long current = 0;

void initUltrasonic(){
    pinMode(TRIG_PIN, OUTPUT); 
    pinMode(ECHO_PIN, INPUT); 
}

void initStepper(){
    stepper.setMaxSpeed(500);
    stepper.setAcceleration(200);
}

void initWifi(){
    delay(1000);
    WiFi.mode(WIFI_STA);
    Serial.print("ESP32 MAC Address: ");
    Serial.println(WiFi.macAddress());
    WiFi.begin(ssid, password);
    Serial.print("Connecting to mobile hotspot: ");
    Serial.println(ssid);
    // Wait until connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP()); // Prints the IP assigned by your phone
}

void setup() {
    Serial.begin(9600); 
    delay(1000);
    for(int i = 5; i > 0; i--){
        Serial.printf("Beginning init in %d\n", i);
        delay(1000);
    }
    initUltrasonic();
    initStepper();
    initWifi();


}

void handleUltraSonicSensor(){
    if(current - lastPing >= 500){
        lastPing = current;
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
}

void handleWifi(){
    if(current - lastPing < 500){
        return;
    }


}

void handleStepperMotor(){
    if (stepper.distanceToGo() == 0){
        stepper.moveTo(stepper.currentPosition() == 0 ? 2048: 0); // 2048 steps is ~ 1 turn
    }
    stepper.run();
}

void loop() {
    current = millis();
    handleUltraSonicSensor();
    // handleWifi();
    handleStepperMotor();
}

