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


#define TEAM_ID "T18"    
constexpr uint16_t MQTT_PORT = 8883;  
constexpr uint32_t WIFI_RETRY_MS = 20000;
constexpr uint32_t BACKOFF_START_MS = 1000;
constexpr uint32_t BACKOFF_MAX_MS = 30000;
constexpr uint32_t mqttkeepalive = 15; // seconds
constexpr char NVS_NAMESPACE[] = "pill-dispenser";


struct Credentials{
    String wifiSsid, wifiPass;
    String mqttHost;
    String mqttUser; String mqttPass;
}
Credentials creds;

Preferences prefs;
constexpr const char* NVS_NAMESPACE = "pill-dispenser";

WiFiClientSecure net;
PubSubClient mqtt(net);

String clientId;
const String TOPIC_TELEMETRY = String("iot/") + TEAM_ID + "/telemetry";
const String TOPIC_STATUS = String("iot/") + TEAM_ID + "/status";

uint32_t backoffMs   = BACKOFF_START_MS;
uint32_t lastMqttTry = 0;
uint32_t seqNo       = 0;      // message counter: gaps in seq = lost messages
uint32_t pubOk = 0, pubFail = 0, reconnects = 0;

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

bool loadCredentials(){
    prefs.begin(NVS_NAMESPACE, true);
    creds.wifiSsid = prefs.getString("wifi_ssid", "");
    creds.wifiPass = prefs.getString("wifi_pass", "");
    creds.mqttHost = prefs.getString("mqtt_host", "");
    creds.mqttUser = prefs.getString("mqtt_user", "");
    creds.mqttPass = prefs.getString("mqtt_pass", "");
    prefs.end();
    if(creds.mqttHost == "" || creds.mqttUser == "" || creds.mqttPass == ""){
        Serial.println("No credentials found in NVS");
        return false;
    }else{
        Serial.println("Credentials loaded from NVS");
        return true;
    }
}
void saveCredentials(){
    prefs.begin(NVS_NAMESPACE, false);    
    prefs.putString("wifi_ssid", creds.wifiSsid);
    prefs.putString("wifi_pass", creds.wifiPass);
    prefs.putString("mqtt_host", creds.mqttHost);
    prefs.putString("mqtt_user", creds.mqttUser);
    prefs.putString("mqtt_pass", creds.mqttPass);
    prefs.end();
    Serial.println("Credentials saved to NVS");
}

void clearCredentials(){
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    creds = Credentials();
    Serial.println("Credentials cleared from NVS");
}

void printCredentials(){
    Serial.printf("WiFi SSID: %s\n", creds.wifiSsid.c_str());
    Serial.printf("WiFi Pass: %s\n", creds.wifiPass.c_str());
    Serial.printf("MQTT Host: %s\n", creds.mqttHost.c_str());
    Serial.printf("MQTT User: %s\n", creds.mqttUser.c_str());
    Serial.printf("MQTT Pass: %s\n", creds.mqttPass.c_str());
}

void runProvisioning(){
    Serial.println("Running provisioning...");
    do {
        creds.mqttHost = iotAsk("Enter MQTT Host: ");
    } while(creds.mqttHost == "");
    do {
        creds.mqttUser = iotAsk("Enter MQTT User: ");
    } while(creds.mqttUser == "");
    do {
        creds.mqttPass = iotAsk("Enter MQTT Pass: ");
    } while(creds.mqttPass == "");
    saveCredentials();
    serial.println("Provisioning complete.");
    printCredentials();

}

void configureMqtt(){
    net.setinsecure(); // disable certificate verification for testing
    mqtt.setServer(creds.mqttHost.c_str(), MQTT_PORT);
    mqtt.setkeepAlive(mqttkeepalive);
    mqtt.setsocketTimeout(5);
    mqtt.setBufferSize(512); // default is 128, we need more for our payload

}

const char* mqttStateText(int s) {
  switch (s) {
    case MQTT_CONNECTION_TIMEOUT:      return "-4 CONNECTION_TIMEOUT (no answer: host/port/firewall?)";
    case MQTT_CONNECTION_LOST:         return "-3 CONNECTION_LOST (network dropped)";
    case MQTT_CONNECT_FAILED:          return "-2 CONNECT_FAILED (TCP/TLS failed: host name? Wi-Fi blocks 8883?)";
    case MQTT_DISCONNECTED:            return "-1 DISCONNECTED";
    case MQTT_CONNECTED:               return "0 CONNECTED";
    case MQTT_CONNECT_BAD_PROTOCOL:    return "1 BAD_PROTOCOL";
    case MQTT_CONNECT_BAD_CLIENT_ID:   return "2 BAD_CLIENT_ID";
    case MQTT_CONNECT_UNAVAILABLE:     return "3 UNAVAILABLE (broker busy)";
    case MQTT_CONNECT_BAD_CREDENTIALS: return "4 BAD_CREDENTIALS (username/password)";
    case MQTT_CONNECT_UNAUTHORIZED:    return "5 UNAUTHORIZED (credential lacks permission)";
    default:                           return "unknown";
  }
}

bool mqttConnect(){
    Serial.print("MQTT: connecting to ");
    Serial.print(creds.mqttHost);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print(" as ");
    Serial.println(clientId);
    if(mqtt.connected()) return true;
    Serial.printf("Connecting to MQTT broker %s:%u...\n", creds.mqttHost.c_str(), MQTT_PORT);
    clientId = String("ESP32-") + String(random(0xffff), HEX);
    if(mqtt.connect(clientId.c_str(), creds.mqttUser.c_str(), creds.mqttPass.c_str())){
        Serial.println("MQTT connected");
        reconnects++;
        return true;
    }else{
        Serial.printf("MQTT connect failed, rc=%d (%s)\n", mqtt.state(), mqttStateText(mqtt.state()));
        return false;
    }
}

void maintainMqtt(){
    if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
    if (millis() - lastMqttTry < backoffms) return; // don't spam
    lastMqttTry = millis();
    if(mqtt.connected()){
        mqtt.loop();
        backoffMs = BACKOFF_START_MS; // reset backoff on success
        reconnects++;
    }else{
        Serial.println(mqttStateText(mqtt.state()));
        backoffms = min(backoffMs * 2, BACKOFF_MAX_MS) + (millis() % 500); 
    }
}

void printHelp() {
  Serial.println("Commands: HELP | STATUS | PUB (publish now) | PROVISION | FORGET");
}

void printStatus() {
  Serial.print("Team "); Serial.print(TEAM_ID); Serial.print("  client id "); Serial.println(clientId);
  printCredentials();
  Serial.print("  Wi-Fi: "); Serial.println(WiFi.status() == WL_CONNECTED ? "connected" : "down");
  Serial.print("  MQTT : "); Serial.println(mqttStateText(mqtt.state()));
  Serial.print("  published ok/failed: "); Serial.print(pubOk); Serial.print("/"); Serial.print(pubFail);
  Serial.print("  reconnects: "); Serial.println(reconnects);
  Serial.print("  period: "); Serial.print(PERIOD_MS); Serial.println(" ms");
}

void handleCommand(String line) {
  line.trim();
  line.toUpperCase();
  if (line.length() == 0) return;
  if (line == "HELP")   { printHelp(); return; }
  if (line == "STATUS") { printStatus(); return; }
  if (line == "PUB") {
    if (!mqtt.connected()) { Serial.println("ERR not connected to the broker"); return; }
    if (publishTelemetry()) pubOk++; else pubFail++;
    return;
  }
  if (line == "FORGET") { goOfflineCleanly(); forgetCredentials(); Serial.println("Rebooting..."); ESP.restart(); return; }
  if (line == "PROVISION") { goOfflineCleanly(); runProvisioning(); Serial.println("Rebooting..."); ESP.restart(); return; }
  Serial.println("ERR unknown command. Type HELP");
}

void pollConsole() {
  static String buf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { handleCommand(buf); buf = ""; continue; }
    if (buf.length() < 60) buf += c;
  }
}


void goOfflineCleanly() {
  if (mqtt.connected()) {
    mqtt.publish(TOPIC_STATUS.c_str(), "offline", true);
    mqtt.disconnect();
  }
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
    iotInitI2C();
    iotInitSPI();
    iotInitDisplay(); 

    Serial.println("Type PROVISION (new credentials) or FORGET (erase) within 3 s...");
    iotDisplayLines("Lab 5 cloud", "PROVISION/FORGET?", "(3 s, Serial)");
    String cmd = iotReadLine(3000);

    cmd.toUpperCase();
    if (cmd == "FORGET") forgetCredentials();

    bool haveCreds = loadCredentials();
    if (cmd == "PROVISION" || !haveCreds) {
    if (!haveCreds) Serial.println("No credentials in NVS yet.");
    iotDisplayLines("Lab 5 cloud", "Provisioning:", "use Serial Monitor");
    runProvisioning();
    } else {
    Serial.println("Credentials loaded from NVS:");
    printCredentials();
    }

    WiFi.mode(WIFI_STA);
    configureMqtt();
    printHelp();
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
    pollconsole();
    maintainMqtt();
    maintainWifi();
    mqtt.loop();

    current = millis();
    handleUltraSonicSensor();
    handleWifi();

    static uint32_t lastPub = 0;
    if (iotEvery(PERIOD_MS, lastPub)) {
    if (mqtt.connected()) {
        if (publishTelemetry()) pubOk++; else pubFail++;
        }
    }

    static uint32_t lastDraw = 0;
    if (iotEvery(1000, lastDraw)) {
    const bool w = (WiFi.status() == WL_CONNECTED);
    String t = bmpOk ? String(bmp.readTemperature(), 1) + " C" : String("no BMP280");
    iotDisplayLines(String("Lab 5  ") + TEAM_ID,
                    w ? "WiFi " + String((int)WiFi.RSSI()) + " dBm" : String("WiFi: down"),
                    mqtt.connected() ? String("MQTT: online") : String("MQTT: retrying"),
                    t + "  sent " + String((unsigned long)pubOk));
    }
    // handleStepperMotor();
}

