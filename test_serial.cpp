#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=====================================");
    Serial.println("ESP32-S3 Serial Test");
    Serial.println("=====================================");
    Serial.println("If you can see this, serial is working!");
    Serial.println("Type anything to test input:");
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        Serial.printf("You typed: %s\n", input.c_str());
    }
    
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        lastPrint = millis();
        Serial.printf("Heartbeat: %lu ms\n", millis());
    }
    
    delay(100);
}