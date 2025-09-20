#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <NewPing.h> 

// กำหนดขาสำหรับเซ็นเซอร์อัลตราโซนิก
#define TRIGGER_PIN  23 
#define ECHO_PIN     25 

// === เพิ่มขาสำหรับ LED สองดวงใหม่ ===
#define LED1_PIN     18 // LED สำหรับปุ่ม 1 (ใช้ขาเดิมเพื่อความง่าย)
#define LED2_PIN     19 // LED สำหรับปุ่ม 2 (ใช้ขาใหม่)
// ===================================

// กำหนดขาสำหรับปุ่มกด
#define BUTTON1_PIN  4 
#define BUTTON2_PIN  5 

// === เพิ่มขาสำหรับเซ็นเซอร์ IR และมอเตอร์ ===
#define IR_SENSOR_PIN 26
#define MOTOR_PIN     27
// ===================================

// กำหนดที่อยู่ I2C และขนาดของหน้าจอ LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// กำหนดระยะสูงสุดที่เซ็นเซอร์สามารถวัดได้ (cm)
#define MAX_DISTANCE 400 

// สร้างอ็อบเจกต์ NewPing สำหรับเซ็นเซอร์
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

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
  Serial.begin(9600); 

  // === ตั้งค่าขา LED ทั้งสองดวงเป็น OUTPUT ===
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW); // ตั้งค่าเริ่มต้นให้ LED ดับ
  digitalWrite(LED2_PIN, LOW);
  // ======================================
  
  // === ตั้งค่าขาเซ็นเซอร์ IR เป็น INPUT และมอเตอร์เป็น OUTPUT ===
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW); // ตั้งค่าเริ่มต้นให้มอเตอร์หยุดหมุน
  // ====================================================

  // กำหนดโหมดของขาปุ่มกดแบบ Pull-up
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // เริ่มต้นการทำงานของหน้าจอ LCD
  lcd.init(); 
  lcd.backlight();
  
  // แสดงข้อความเริ่มต้น
  lcd.clear(); 
  lcd.setCursor(0,0); 
  lcd.print("Waiting..."); 
} 

void loop() { 
  unsigned int distance = sonar.ping_cm(); 
  
  // ตรวจสอบการกดปุ่มก่อนการทำงานของเซ็นเซอร์
  if (digitalRead(BUTTON1_PIN) == LOW) {
    if (currentState != BUTTON_PRODUCT1) {
      currentState = BUTTON_PRODUCT1;
      lastStateChangeTime = millis();
      Serial.println("Button 1 pressed. Changing to BUTTON_PRODUCT1 state.");
      // === เปิด LED1 และปิด LED2 เมื่อกดปุ่ม 1 ===
      digitalWrite(LED1_PIN, HIGH);
      digitalWrite(LED2_PIN, LOW);
      // =======================================
    }
  } else if (digitalRead(BUTTON2_PIN) == LOW) {
    if (currentState != BUTTON_PRODUCT2) {
      currentState = BUTTON_PRODUCT2;
      lastStateChangeTime = millis();
      Serial.println("Button 2 pressed. Changing to BUTTON_PRODUCT2 state.");
      // === เปิด LED2 และปิด LED1 เมื่อกดปุ่ม 2 ===
      digitalWrite(LED2_PIN, HIGH);
      digitalWrite(LED1_PIN, LOW);
      // =======================================
    }
  } else {
    if (currentState == WAITING && distance > 0 && distance <= 10) {
      currentState = READY;
      lastStateChangeTime = millis();
      Serial.println("Distance is <= 10 cm. Changing to READY state."); 
    }
  }

  // จัดการการแสดงผลตามสถานะปัจจุบัน
  switch (currentState) {
    case WAITING:
      lcd.clear(); 
      lcd.setCursor(0,0); 
      lcd.print("Waiting..."); 
      // === ปิด LED ทั้งสองดวงเมื่อกลับสู่สถานะ WAITING ===
      digitalWrite(LED1_PIN, LOW);
      digitalWrite(LED2_PIN, LOW);
      // ===============================================
      break;

    case READY:
      lcd.clear(); 
      lcd.setCursor(1, 0); 
      lcd.print("READY"); 
      if (millis() - lastStateChangeTime >= readyDisplayDuration) {
        currentState = PRODUCT1_2;
        lastStateChangeTime = millis();
        Serial.println("READY duration complete. Changing to PRODUCT1 & PRODUCT2 state.");
      }
      break;
    
    case PRODUCT1_2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PRODUCT1");
      lcd.setCursor(0, 1);
      lcd.print("PRODUCT2");
      if (digitalRead(IR_SENSOR_PIN) == LOW) {
          digitalWrite(MOTOR_PIN, HIGH); // มอเตอร์หมุนเมื่อมีวัตถุผ่าน
      } else {
          digitalWrite(MOTOR_PIN, LOW); // มอเตอร์หยุดหมุนเมื่อไม่มีวัตถุ
      }
      if (millis() - lastStateChangeTime >= productDisplayDuration) {
        currentState = RESETTING;
        Serial.println("PRODUCT display duration complete. Resetting.");
      }
      break;
      
    case RESETTING:
      digitalWrite(MOTOR_PIN, LOW); // มอเตอร์หยุดทำงานเมื่อออกจากสถานะนี้
      if (distance == 0 || distance > 10) {
        currentState = WAITING;
        Serial.println("Object is gone. Back to WAITING state.");
      }
      break;
    
    case BUTTON_PRODUCT1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("product1");
      lcd.setCursor(0, 1);
      lcd.print("10 bath");
      digitalWrite(MOTOR_PIN, LOW); // มอเตอร์หยุดทำงานเมื่อเข้าสู่สถานะนี้
      if (millis() - lastStateChangeTime >= buttonDisplayDuration) {
        currentState = WAITING;
        Serial.println("BUTTON_PRODUCT1 duration complete. Back to WAITING state.");
      }
      break;

    case BUTTON_PRODUCT2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("product2");
      lcd.setCursor(0, 1);
      lcd.print("10 bath");
      digitalWrite(MOTOR_PIN, LOW); // มอเตอร์หยุดทำงานเมื่อเข้าสู่สถานะนี้
      if (millis() - lastStateChangeTime >= buttonDisplayDuration) {
        currentState = WAITING;
        Serial.println("BUTTON_PRODUCT2 duration complete. Back to WAITING state.");
      }
      break;
  }
  delay(50); 
}
