#include <Arduino.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include "secrets.h"
#include "ultrasonic.h"

WiFiClient client;
const char* SERVER_IP = "172.20.10.5"; // destination IP here
const uint16_t SERVER_PORT = 5050;     // destination Port here

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

#define NUM_PAYLOADS 2
UltrasonicPayload payloads[NUM_PAYLOADS];
UltrasonicPayload* current_payload = payloads; 
unsigned int current_payload_size = 0;
unsigned int current_payload_idx = 0; // ultrasonic puts in idx, wifi reads from !idx
bool payload_ready_flag = false;

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
        if(current_payload_size == 0){
            current_payload->m = current;
        }
        lastPing = current;
        // clear the TRIG_PIN by setting it LOW for 2 microseconds
        digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
        // Trigger the sensor by setting the TRIG_PIN HIGH for 10 microseconds
        digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        // Read the ECHO_PIN, returns the sound wave travel time in microseconds
        duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
        distanceCm = duration * SOUND_SPEED / 2; // 0 means no echo
        Serial.print("Distance (cm): ");
        Serial.println(distanceCm);

        // add data to current payload
        float* distances = current_payload->distances;
        distances += current_payload_size;
        *distances = distanceCm;
        ++current_payload_size;

        // switch to other payload if full
        if(current_payload_size >= ULTRASONIC_DISTANCE_LENGTH){
            UltrasonicPayload* old = current_payload;
            current_payload_idx = (current_payload_idx + 1) % NUM_PAYLOADS;
            current_payload = &payloads[current_payload_idx];
            current_payload->m = current;
            current_payload_size = 0;
            payload_ready_flag = true;

            Serial.print("Old m: ");
            Serial.println(old->m);
        }
    }
}

#define CONNECT_RETRY_MS 2000
#define CONNECT_TIMEOUT_MS 500
unsigned long lastConnectAttempt = 0;

void handleWifi(){
    if(!payload_ready_flag) return;
    if(!client.connected()){
        // don't block the sensor loop retrying every iteration
        if(lastConnectAttempt != 0 && current - lastConnectAttempt < CONNECT_RETRY_MS) return;
        lastConnectAttempt = current;
        Serial.printf("Connecting to %s:%u...\n", SERVER_IP, SERVER_PORT);
        if(!client.connect(SERVER_IP, SERVER_PORT, CONNECT_TIMEOUT_MS)){
            Serial.println("Connect failed, will retry");
            return;
        }
        Serial.println("Connected to server");
    }
    // the buffer that was just filled is the one NOT at current_payload_idx
    UltrasonicPayload* ready = &payloads[(current_payload_idx + 1) % NUM_PAYLOADS];
    size_t sent = client.write((const uint8_t*)ready, sizeof(UltrasonicPayload));
    if(sent != sizeof(UltrasonicPayload)){
        Serial.printf("Send failed (%u/%u bytes), reconnecting\n", sent, sizeof(UltrasonicPayload));
        client.stop();
        return; // flag stays set, retry this payload after reconnect
    }
    Serial.printf("Sent %u bytes\n", sent);
    payload_ready_flag = false;
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
    handleWifi();
    // handleStepperMotor();
}

