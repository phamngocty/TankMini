#include <Arduino.h>
#include "MotorDriver.h"
#include "IrTransceiver.h"
#include "Feedback.h"
#include "TankGameEngine.h"
#include "TankNetwork.h"

// ==========================================
// TANK IDENTITY CONFIGURATION
// Team: 1 = Blue Team, 2 = Red Team
// Player: 1 - 15 (Unique ID per tank)
// ==========================================
static const uint8_t TANK_TEAM_ID   = 1; // Team Blue
static const uint8_t TANK_PLAYER_ID = 1; // Tank #1

static const uint8_t PIN_BOOT_BTN = 9; // On-board BOOT button for Hard Reset

// Core Subsystem Instances
MotorDriver    motors;
IrTransceiver  ir;
Feedback       feedback;
TankGameEngine gameEngine;
TankNetwork    network;

void setup() {
    // 0. Early Hardware Safeguard: Lock GPIO 8 HIGH to protect ESP32-C3 strapping boot
    pinMode(MotorDriver::PIN_NSLEEP, OUTPUT);
    digitalWrite(MotorDriver::PIN_NSLEEP, HIGH);

    Serial.begin(115200);
    delay(500);

    // Check GPIO 9 (On-board BOOT Button) for Hardware NVS Reset (Hold 3 seconds)
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
    if (digitalRead(PIN_BOOT_BTN) == LOW) {
        Serial.println("[BOOT] Button GPIO 9 detected! Hold 3s for Factory Hard Reset...");
        uint32_t pressStart = millis();
        bool confirmed = false;
        while (digitalRead(PIN_BOOT_BTN) == LOW) {
            if (millis() - pressStart >= 3000) {
                confirmed = true;
                break;
            }
            delay(50);
        }
        if (confirmed) {
            Serial.println("[BOOT] Hard Reset confirmed! Emitting 3 beeps and clearing NVS...");
            pinMode(Feedback::PIN_BUZZER, OUTPUT);
            for (int i = 0; i < 3; i++) {
                tone(Feedback::PIN_BUZZER, 2000, 100);
                delay(180);
            }
            noTone(Feedback::PIN_BUZZER);
            network.clearWifiConfig();
            network.clearProfile();
            Serial.println("[BOOT] NVS cleared successfully. Rebooting to Tank_Setup mode...");
            delay(300);
            ESP.restart();
        }
    }

    Serial.println("\n==============================================");
    Serial.println("  MICRO TANK ARENA (ESP32-C3 LASER TAG) v1.0  ");
    Serial.println("==============================================");

    // 1. Initialize Motors (DRV8833 with PWM Slew-Rate Limiter)
    motors.begin();
    Serial.println("[OK] MotorDriver initialized on GPIO 0,1,2,3 (nSLEEP: GPIO 8).");

    // 2. Initialize Optical IR Transceiver (RMT 38kHz TX + TSOP38238 RX)
    ir.begin(TANK_TEAM_ID, TANK_PLAYER_ID);
    Serial.printf("[OK] IR Transceiver active (TX: GPIO 4, RX: GPIO 5) - Team: %d, ID: %d.\n",
                  TANK_TEAM_ID, TANK_PLAYER_ID);

    // 3. Initialize Audio & Visual Feedback (WS2812B + Passive Buzzer)
    feedback.begin(TANK_TEAM_ID);
    Serial.println("[OK] Feedback system active (RGB: GPIO 6, Buzzer: GPIO 7).");

    // 4. Initialize Tank Game Engine (FSM & Rules)
    gameEngine.begin(&motors, &ir, &feedback, TANK_TEAM_ID, TANK_PLAYER_ID);
    Serial.println("[OK] Game Engine online (HP: 5, Ammo: 10, I-Frame: 1.5s).");

    // 5. Initialize Network (Hybrid Always-On: 200ms SoftAP + Non-blocking STA + Captive Portal + WebSocket + ESP-NOW)
    network.begin(&gameEngine, TANK_TEAM_ID, TANK_PLAYER_ID);
    Serial.printf("[NET] Hybrid Always-On active: SoftAP 'Tank_%s_%02d' @ 192.168.4.1 (DNS Captive Portal ON)\n",
                  (TANK_TEAM_ID == 1) ? "Blue" : "Red", TANK_PLAYER_ID);

    Serial.println("==============================================");
    Serial.println("[READY] System initialized. Waiting for pilot connection...");
    Serial.println("==============================================\n");
}

void loop() {
    // Non-blocking real-time cooperative multitasking loop
    motors.update();
    ir.update();
    feedback.update();
    gameEngine.update();
    network.update();
    
    // Minimal yield for Wi-Fi stack
    delay(2);
}
