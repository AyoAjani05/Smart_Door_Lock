# Smart Door Lock System

## Project Overview
The Smart Door Lock System is an Arduino-based electronic access-control solution. It provides secure PIN-based authentication via a matrix keypad to actuate an SG90 servo motor, which serves as the physical locking mechanism. Additionally, it utilizes an HC-SR04 ultrasonic sensor to continuously monitor the door's position, automatically re-engaging the lock once the door is confirmed closed. 

## Key Features
* **Dual-Tier PIN Authentication**: Supports a standard User PIN for daily access and a Master PIN for administrative functions.
* **Automated Mechanical Locking**: The servo motor engages the lock only after the ultrasonic sensor verifies the door is fully closed and settled, preventing mechanical damage.
* **Persistent Credential Storage**: PINs are saved in the Arduino's non-volatile EEPROM memory, ensuring credentials survive complete power losses.
* **I²C Bus Architecture**: Utilizes two PCF8574 I/O expanders to manage both the 4x3 keypad and the OLED display over a single 2-wire bus, significantly reducing GPIO pin usage.
* **Finite State Machine (FSM)**: Operates on a structured synchronous state machine to ensure predictable transitions between idle, entry, locking, and administrative states.
* **Interactive Interface**: Provides clear, real-time feedback through a 16x3 SSD1306 OLED display.

## Hardware Requirements

* Arduino UNO
* 4x3 Matrix Keypad
* 0.96" SSD1306 OLED Display
* 2x PCF8574 I²C I/O Expanders
* HC-SR04 Ultrasonic Sensor
* SG90 Micro Servo Motor

## Pin Mapping
| Interface | Device | Purpose | Arduino Pins |
| :--- | :--- | :--- | :--- |
| **I²C** | PCF8574 Expander (0x21) | Keypad scanning | A4 (SDA), A5 (SCL) |
| **I²C** | OLED Display (0x3C) | Visual feedback | SDA, SCL |
| **PWM GPIO** | SG90 Servo | Physical lock actuation | 16 (A2) |
| **Digital GPIO**| HC-SR04 Ultrasonic | Door state detection | 7 (TRIG), 8 (ECHO) |

## Software Dependencies
The system relies on the following Arduino libraries:
* `Keypad_I2C.h`
* `Wire.h`
* `EEPROM.h`
* `Servo.h`
* `Adafruit_GFX.h`
* `Adafruit_SSD1306.h`

## System Operation & Usage

### 1. Boot & Initialization
* On first boot, the system writes factory default credentials to the EEPROM.
* **Default User PIN**: `1234`
* **Default Master PIN**: `9999`
* The OLED displays an initializing message and the servo moves to the locked position (90°).

### 2. Standard Access
* The system idles until any keypad button is pressed.
* Input your 4-digit User PIN. The OLED masks the input with asterisks (*) for privacy.
* If correct, the system displays "Access Granted" and the servo retracts to 180° (unlocked).
* If incorrect, it displays "Access Denied" and triggers a 2-second lockout.

### 3. Auto-Lock Mechanism
* The ultrasonic sensor continuously polls distance. 
* Once the door registers a distance of < 10 cm (closed) for at least 2 seconds, the servo automatically rotates to 90° to secure the door.

### 4. Admin Mode
* Enter the Master PIN (`9999` by default) during the standard PIN entry sequence.
* This grants access to the configuration menu where you can press `1` to overwrite the current PIN or `3` to exit.
* Follow the on-screen prompts to capture exactly four digits for the new PIN, which is then committed to the EEPROM.

## Authors
**Computer Engineering Group 1**
Federal University of Technology Akure (FUTA)
