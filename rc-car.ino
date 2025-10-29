#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>

// ----------------- ตั้งค่าการเชื่อมต่อ Wi-Fi -------------------
const char* ssid = "Car_Controller_RC"; 
const char* password = "password123"; 
// -------------------------------------------------------------

// ----------------- ตั้งค่าพิน Motor & Servo (ใช้หมายเลข GPIO) ---------------------
// L298N Mini (4 สายควบคุม)
// D5 -> GPIO14
#define PWMA_L 14 // PWM Motor ซ้าย (ความเร็ว)
// D6 -> GPIO12
#define DIR_L 12 // Direction Motor ซ้าย (ทิศทาง)
// D7 -> GPIO13
#define PWMA_R 13 // PWM Motor ขวา (ความเร็ว)
// D8 -> GPIO15 (หมายเหตุ: GPIO15 ต้องเป็น LOW เมื่อบูต)
#define DIR_R 15 // Direction Motor ขวา (ทิศทาง)

// Servo 
// D3 -> GPIO0
#define SERVO_PIN 0

Servo steeringServo;
// ... ส่วนที่เหลือของโค้ดเหมือนเดิม ...

WiFiUDP Udp;
unsigned int localUdpPort = 2390; 

// โครงสร้างข้อมูลที่รับจากรีโมท (ต้องตรงกันทั้ง 2 ฝั่ง)
struct CarData {
    int throttle; // ค่าคันเร่ง (0-1023)
    int steering; // ค่าเลี้ยว (0-1023)
};

CarData receivedData;

void setup() {
    Serial.begin(115200);
    
    // ตั้งค่า Motor Pins
    pinMode(PWMA_L, OUTPUT);
    pinMode(DIR_L, OUTPUT);
    pinMode(PWMA_R, OUTPUT);
    pinMode(DIR_R, OUTPUT);
    
    // ตั้งค่า Servo
    steeringServo.attach(SERVO_PIN);
    steeringServo.write(70); // ตั้งค่าเริ่มต้นให้อยู่ตรงกลางที่ 70 องศา

    // 1. สร้าง Access Point
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    // 2. เริ่มต้น UDP
    Udp.begin(localUdpPort);
    Serial.println("UDP listening...");
}

void loop() {
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        Udp.read((char*)&receivedData, sizeof(receivedData));

        // แสดงข้อมูลที่ได้รับ
        Serial.print("Rx: Thr=");
        Serial.print(receivedData.throttle);
        Serial.print(", Steer=");
        Serial.println(receivedData.steering);

        // 3. ประมวลผลและควบคุม
        controlMotor(receivedData.throttle);
        controlSteering(receivedData.steering);
    }
    delay(10);
}

// ----------------- ฟังก์ชันควบคุมมอเตอร์ -----------------

void controlMotor(int rawThrottle) {
    // Mapping ค่าคันเร่ง: 0-1023 -> -1023 ถึง 1023 (สำหรับ Forward/Backward)
    int throttle = map(rawThrottle, 0, 1023, -1023, 1023);

    int speed = abs(throttle);
    if (speed > 1023) speed = 1023; 
    
    if (abs(throttle) < 50) { // Dead zone ใกล้ศูนย์
        // หยุดมอเตอร์
        analogWrite(PWMA_L, 0);
        analogWrite(PWMA_R, 0);
        digitalWrite(DIR_L, LOW);
        digitalWrite(DIR_R, LOW);
    } else if (throttle > 0) {
        // เดินหน้า (Forward)
        digitalWrite(DIR_L, HIGH); 
        digitalWrite(DIR_R, HIGH);
        analogWrite(PWMA_L, speed);
        analogWrite(PWMA_R, speed);
    } else {
        // ถอยหลัง (Backward)
        digitalWrite(DIR_L, LOW); 
        digitalWrite(DIR_R, LOW);
        analogWrite(PWMA_L, speed);
        analogWrite(PWMA_R, speed);
    }
}

void controlSteering(int rawSteering) {
    // Mapping ค่า 0-1023 ให้เป็น 40 ถึง 100 องศา 
    int angle = map(rawSteering, 0, 1023, 40, 100);
    
    // จำกัดค่าให้อยู่ในช่วงที่กำหนดเพื่อความปลอดภัย
    angle = constrain(angle, 40, 100); 

    steeringServo.write(angle);
}