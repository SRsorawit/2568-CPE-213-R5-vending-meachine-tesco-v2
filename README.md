# ESP32 Vending Machine Prototype

โปรเจกต์นี้เป็นการสร้างต้นแบบตู้กดสินค้าอัตโนมัติ 
โดยใช้ ESP32 ร่วมกับอุปกรณ์ต่าง ๆ เช่น Ultrasonic Sensor, IR Sensor, LCD I2C, Motor Driver, ปุ่มกด และ LED

library ที่ต้องติดตั้งเพิ่ม
LiquidCrystal_I2C.h,
NewPing.h

การทำงานหลัก:
- ตรวจจับคนเข้าใกล้ด้วย Ultrasonic Sensor
- แสดงเมนูบน LCD
- เลือกสินค้าโดยกดปุ่ม
- ยืนยันการชำระด้วย IR Sensor
- มอเตอร์จ่ายสินค้าออกมา
