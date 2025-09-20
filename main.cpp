#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#include <NewPing.h> 
#include <Stepper.h> // เพิ่มไลบรารีสำหรับ Stepper

// กำหนดขาสำหรับเซ็นเซอร์อัลตราโซนิก
#define TRIGGER_PIN  23 
#define ECHO_PIN     25 

// กำหนดขาสำหรับไฟ LED
#define LED_PIN      18 

// กำหนดขาสำหรับปุ่มกด
#define BUTTON1_PIN  4 
#define BUTTON2_PIN  5 

// กำหนดจำนวนสเต็ปต่อการหมุนหนึ่งรอบของมอเตอร์ (ส่วนใหญ่คือ 2048 หรือ 4096 สำหรับมอเตอร์ 28BYJ-48)
#define STEPS_PER_REV 2048 

// กำหนดขาควบคุมสเต็ปปิ้งมอเตอร์ (ตามการเชื่อมต่อกับ driver เช่น ULN2003)
#define MOTOR_PIN1  32 // เปลี่ยนขาให้เหมาะสม
#define MOTOR_PIN2  33
#define MOTOR_PIN3  27
#define MOTOR_PIN4  14

// กำหนดขนาดหน้าจอ OLED
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); 

// กำหนดระยะสูงสุดที่เซ็นเซอร์สามารถวัดได้ (cm)
#define MAX_DISTANCE 400 

// สร้างอ็อบเจกต์ NewPing สำหรับเซ็นเซอร์
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

// สร้างอ็อบเจกต์ Stepper
Stepper myStepper(STEPS_PER_REV, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);

// ตัวแปรสถานะสำหรับควบคุมการแสดงผล
enum DisplayState {
  WAITING,
  READY,
  PRODUCT1_2,
  RESETTING,
  BUTTON_PRODUCT1,
  BUTTON_PRODUCT2
};

DisplayState currentState = WAITING;
unsigned long lastStateChangeTime = 0;
const unsigned long readyDisplayDuration = 2000; // 2 วินาที
const unsigned long productDisplayDuration = 10000; // 10 วินาที
const unsigned long buttonDisplayDuration = 5000; // 5 วินาที

void setup() { 
  Serial.begin(115200); 

  pinMode(LED_PIN, OUTPUT); 
  digitalWrite(LED_PIN, LOW); 

  // กำหนดโหมดของขาปุ่มกดแบบ Pull-up
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  
  // กำหนดความเร็วของ Stepper Motor
  myStepper.setSpeed(60); // หมุนด้วยความเร็ว 60 RPM

  // เริ่มต้นการทำงานของหน้าจอ OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed")); 
    for(;;); 
  } 
  
  display.clearDisplay(); 
  display.setTextSize(2); 
  display.setTextColor(WHITE); 
  display.setCursor(0,0); 
  display.println("Waiting..."); 
  display.display(); 
} 

void loop() { 
  unsigned int distance = sonar.ping_cm(); 
  
  // ตรวจสอบการกดปุ่มก่อนการทำงานของเซ็นเซอร์
  if (digitalRead(BUTTON1_PIN) == LOW) {
    if (currentState != BUTTON_PRODUCT1) {
      currentState = BUTTON_PRODUCT1;
      lastStateChangeTime = millis();
      Serial.println("Button 1 pressed. Changing to BUTTON_PRODUCT1 state.");
    }
  } else if (digitalRead(BUTTON2_PIN) == LOW) {
    if (currentState != BUTTON_PRODUCT2) {
      currentState = BUTTON_PRODUCT2;
      lastStateChangeTime = millis();
      Serial.println("Button 2 pressed. Changing to BUTTON_PRODUCT2 state.");
    }
  } else {
    // ถ้าไม่มีการกดปุ่ม ให้ทำงานตามปกติ
    if (currentState == WAITING && distance > 0 && distance <= 10) {
      currentState = READY;
      lastStateChangeTime = millis();
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Distance is <= 10 cm. Changing to READY state."); 
    }
  }

  // จัดการการแสดงผลและเงื่อนไขการทำงาน
  switch (currentState) {
    case WAITING:
      display.clearDisplay(); 
      display.setTextSize(2); 
      display.setCursor(0,0); 
      display.println("Waiting..."); 
      display.display(); 
      digitalWrite(LED_PIN, LOW);
      break;

    case READY:
      display.clearDisplay(); 
      display.setTextSize(2); 
      display.setCursor(15, 20); 
      display.println("READY"); 
      display.display(); 
      if (millis() - lastStateChangeTime >= readyDisplayDuration) {
        currentState = PRODUCT1_2;
        lastStateChangeTime = millis();
        Serial.println("READY duration complete. Changing to PRODUCT1 & PRODUCT2 state.");
      }
      break;
    
    case PRODUCT1_2:
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 5);
      display.println("PRODUCT1");
      display.setCursor(0, 35);
      display.println("PRODUCT2");
      display.display();
      
      // เพิ่มเงื่อนไขเพื่อเช็คเซ็นเซอร์อัลตราโซนิกและควบคุม Stepper
      if (distance > 0 && distance <= 10) {
        myStepper.step(STEPS_PER_REV / 2); // หมุน 180 องศา
        Serial.println("Object detected. Stepper is working.");
      } 
      
      if (millis() - lastStateChangeTime >= productDisplayDuration) {
        currentState = RESETTING;
        Serial.println("PRODUCT display duration complete. Resetting.");
      }
      break;
      
    case RESETTING:
      if (distance == 0 || distance > 10) {
        currentState = WAITING;
        digitalWrite(LED_PIN, LOW);
        Serial.println("Object is gone. Back to WAITING state.");
      }
      break;
    
    case BUTTON_PRODUCT1:
      // แสดง "product1" เป็นเวลา 5 วินาที
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.println("product1");
      display.println("10 bath");
      display.display();
      if (millis() - lastStateChangeTime >= buttonDisplayDuration) {
        currentState = WAITING;
        Serial.println("BUTTON_PRODUCT1 duration complete. Back to WAITING state.");
      }
      break;

    case BUTTON_PRODUCT2:
      // แสดง "product2" เป็นเวลา 5 วินาที
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 20);
      display.println("product2");
      display.println("10 bath");
      display.display();
      if (millis() - lastStateChangeTime >= buttonDisplayDuration) {
        currentState = WAITING;
        Serial.println("BUTTON_PRODUCT2 duration complete. Back to WAITING state.");
      }
      break;
  }
  delay(50); 
}
