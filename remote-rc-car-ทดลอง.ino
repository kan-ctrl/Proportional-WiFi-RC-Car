#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// ----------------- Wi-Fi Setup -------------------
const char* ssid = "Car_Controller_RC"; 
const char* password = "password123"; 

IPAddress remoteIP(192, 168, 4, 1); 
unsigned int remotePort = 2390;
// -------------------------------------------------------------

// ----------------- Pin Setup -------------------------
#define STEERING_PIN A0 
#define FWD_PIN 5  // เปลี่ยนจาก D1 เป็น GPIO5
#define REV_PIN 4  // เปลี่ยนจาก D2 เป็น GPIO4

// Throttle Values (0-1023)
#define THROTTLE_STOP 512
#define THROTTLE_SPEED 800
#define THROTTLE_REVERSE 200
// -------------------------------------------------------------

WiFiUDP Udp;

struct CarData {
    int throttle; 
    int steering; 
};

CarData dataToSend;

void setup() {
    Serial.begin(115200);
    
    // Use INPUT_PULLUP (Push button = LOW)
    pinMode(FWD_PIN, INPUT_PULLUP); 
    pinMode(REV_PIN, INPUT_PULLUP); 

    // Connect to AP
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Car AP...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Udp.begin(remotePort);
}

void loop() {
    // 1. Read Steering (Analog)
    dataToSend.steering = analogRead(STEERING_PIN); 

    // 2. Set Throttle (Digital Buttons)
    int fwd_state = digitalRead(FWD_PIN);
    int rev_state = digitalRead(REV_PIN);

    dataToSend.throttle = THROTTLE_STOP;

    if (fwd_state == LOW && rev_state == HIGH) {
        dataToSend.throttle = THROTTLE_SPEED;
    } else if (fwd_state == HIGH && rev_state == LOW) {
        dataToSend.throttle = THROTTLE_REVERSE;
    } 

    // 3. Send UDP data
    Udp.beginPacket(remoteIP, remotePort);
    Udp.write((uint8_t*)&dataToSend, sizeof(dataToSend));
    Udp.endPacket();

    Serial.print("Sent: Thr=");
    Serial.print(dataToSend.throttle);
    Serial.print(", Steer=");
    Serial.println(dataToSend.steering);
    
    delay(20); 
}