#include <Arduino.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "ultrasonic.h"

WiFiClient client;
/* 
const char* SERVER_IP = "172.20.10.5"; // destination IP here
const uint16_t SERVER_PORT = 5050;     // destination Port here
*/

const int TRIG_PIN = 12;
const int ECHO_PIN = 14;

const int IN1_PIN = 27;
const int IN2_PIN = 26;
const int IN3_PIN = 25;
const int IN4_PIN = 33;
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN,IN3_PIN,IN2_PIN,IN4_PIN);

#define TEAM_ID "T18"

constexpr uint16_t MQTT_PORT        = 8883; 
constexpr uint16_t MQTT_KEEPALIVE_S = 15;      
constexpr uint32_t WIFI_RETRY_MS    = 20000;   
constexpr uint32_t BACKOFF_START_MS = 1000;    
constexpr uint32_t BACKOFF_MAX_MS   = 30000;   

const char* MQTT_TOPIC = "iot/T18/telemetry";

unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_MS = 5000;

uint32_t seqNo = 0;
WiFiClientSecure net;
PubSubClient mqtt(net);

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

void initMqtt(){
    net.setInsecure(); // Disable certificate verification for testing purposes

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    mqtt.setSocketTimeout(5);
    mqtt.setBufferSize(512);
}

bool connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    String clientId = "ultrasonic-T18-" +
                      String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.println("Connecting to MQTT broker...");

    if (mqtt.connect(
        clientId.c_str(),
        MQTT_USER,
        MQTT_PASS
    )) {
        Serial.println("MQTT connected!");
        return true;
    }

    Serial.print("MQTT connection failed, state: ");
    Serial.println(mqtt.state());

    return false;
}

void handleMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqtt.connected()) {
        if (millis() - lastMqttAttempt >= MQTT_RETRY_MS) {
            lastMqttAttempt = millis();
            connectMqtt();
        }
        return;
    }

    mqtt.loop();
}

//bugfix code. comment out later
void testBrokerConnection() {
    Serial.println("\n--- MQTT Connection Test ---");

    IPAddress brokerIP;

    if (WiFi.hostByName(MQTT_HOST, brokerIP)) {
        Serial.print("Broker IP: ");
        Serial.println(brokerIP);
    } else {
        Serial.println("ERROR: DNS lookup failed");
        return;
    }

    WiFiClient testClient;

    if (testClient.connect(MQTT_HOST, MQTT_PORT, 3000)) {
        Serial.println("TCP connection SUCCESS");
    } else {
        Serial.println("TCP connection FAILED");
    }

    testClient.stop();

    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

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
    testBrokerConnection();
    initMqtt();
}

bool publishUltrasonicReading(float distanceCm, bool validEcho) {
    if (!mqtt.connected()) {
        return false;
    }

    char payload[128];

    if (validEcho) {
        snprintf(payload, sizeof(payload),
            "{\"seq\":%lu,\"distance_cm\":%.2f,\"uptime_ms\":%lu}",
            (unsigned long)++seqNo,
            distanceCm,
            millis()
        );
    } else {
        snprintf(payload, sizeof(payload),
            "{\"seq\":%lu,\"distance_cm\":null,\"uptime_ms\":%lu}",
            (unsigned long)++seqNo,
            millis()
        );
    }

    bool success = mqtt.publish(MQTT_TOPIC, payload, false);

    if (success) {
        Serial.print("MQTT published: ");
        Serial.println(payload);
    } else {
        Serial.println("MQTT publish failed");
    }

    return success;
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

        //should send reading to mqtt
        publishUltrasonicReading(distanceCm, duration != 0);

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

/*
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
*/

void handleStepperMotor(){
    if (stepper.distanceToGo() == 0){
        stepper.moveTo(stepper.currentPosition() == 0 ? 2048: 0); // 2048 steps is ~ 1 turn
    }
    stepper.run();
}

void loop() {
    current = millis();
    handleMqtt();
    handleUltraSonicSensor();
    //handleWifi(); deprecated, using MQTT instead
    // handleStepperMotor();
}

