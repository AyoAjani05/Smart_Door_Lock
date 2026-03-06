#include <Keypad_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Keypad Memory Address
#define I2CADDR 0x21
// Timing constants
#define TIMEOUT_MS 10000       // 10 seconds inactivity timeout
#define LOCK_DELAY_MS 2000     // Delay after door closes before locking

#define PIN1_ADDR  0   // addresses 0–4
#define PIN2_ADDR  5   // addresses 5–9

// Ultrasonic sensor pins
#define TRIG_PIN 7
#define ECHO_PIN 8

// Sensors & Actuators
// Servo pin
#define SERVO_PIN 16 // Physical Leg 25 (A2)

// Keypad pins (adjust as needed)
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// Digitran keypad, bit numbers of PCF8574 i/o port
byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6}; //connect to the column pinouts of the keypad


// ================== GLOBAL OBJECTS ==================
TwoWire *jwire = &Wire;   //test passing pointer to keypad lib
Keypad_I2C kpd( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574, jwire );
Servo lockServo;

// ================== STATE DEFINITIONS ==================
enum SystemState {
  STATE_BOOT,
  STATE_IDLE,
  STATE_PIN_ENTRY,
  STATE_INCORRECT_PIN,
  STATE_CORRECT_PIN,
  STATE_ADMIN_MODE,
  STATE_ADMIN_MENU,
  STATE_ADD_PIN_STEP1,
  STATE_ADD_PIN_STEP2,
  STATE_RESET_PIN,
  STATE_TIMEOUT
};

SystemState currentState = STATE_IDLE; // For returning after admin

// ================== GLOBAL VARIABLES ==================
char enteredPIN[4] = "";      // 4-digit + null terminator
int pinIndex = 0;
char newPIN[4] = "";          // For admin PIN change
int newPinIndex = 0;
char storedPIN[5];
char MasterPIN[5];

unsigned long lastActivityTime = 0;
unsigned long keyPressStartTime = 0;
bool keyHeld = false;

bool doorOpen = false;         // Track door state (true = open)
unsigned long doorCloseTime = 0;

// ================== STATE MANAGEMENT ==================
void displayIdle() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("System in Idle Mode"));
  display.display(); 
  delay(1000);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Press a key to exit  Idle Mode"));
  display.display(); 
  delay(1000);
}

void displayIncorrectPIN() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Access Denied"));
  display.println(F("Try Again"));
  display.display(); 
  delay(1000);
  goToState(STATE_IDLE);
}

void displayCorrectPIN() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Access Granted"));
  display.println(F("Unlocking..."));
  display.display(); 
  delay(500);
  unlockDoor();
  doorOpen = true;
  goToState(STATE_IDLE);
}

void displayAdminMode() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Admin Mode"));
  display.println(F("Loading..."));
  display.display(); 
  delay(1000);
  goToState(STATE_ADMIN_MENU);
}

void displayAdminMenu() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("1:Add PIN"));
  display.println(F("2:Main Menu"));
  display.display(); 
  delay(1000);
}

void displayAddPINStep1() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Enter New PIN:"));
  display.display(); 
  pinIndex = 0;
  while (pinIndex <= 3){
    char key = kpd.getKey();
    if (key){
      if ((key != "#") || (key != "*")) {
        display.print(F("*"));
        display.display(); 
        enteredPIN[pinIndex++] = key;
      }
    }
  }
  EEPROM.put(PIN1_ADDR, enteredPIN);
  delay(1000);
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Pin Changed Successfully"));
  display.display(); 
  goToState(STATE_IDLE);
}

void goToState(SystemState newState) {
  // Exit current state actions if needed
  currentState = newState;
  switch (currentState) {
    case STATE_PIN_ENTRY:
      // Clear PIN buffer
      pinIndex = 0;
      InputPin();
      break;

    case STATE_IDLE:
      displayIdle();
      break;
    case STATE_CORRECT_PIN:
      displayCorrectPIN();
      break;
    case STATE_INCORRECT_PIN:
      displayIncorrectPIN();
      break;
    case STATE_ADMIN_MODE:
      displayAdminMode();
      break;
    case STATE_ADMIN_MENU:
      displayAdminMenu();
      break;
    case STATE_ADD_PIN_STEP1:
      displayAddPINStep1();
      break;
    default:
      break;
  }
  
  lastActivityTime = millis();
}

void InputPin(){
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("Enter PIN:"));
  display.display(); 
  while (pinIndex <= 3){
    char key = kpd.getKey();
    if (key){
      if ((key != "#") || (key != "*")) {
        display.print(F("*"));
        display.display();
        enteredPIN[pinIndex++] = key;
      }
    }
  }

  if (strcmp(enteredPIN, storedPIN) == 4) {
    goToState(STATE_CORRECT_PIN);
  } else if (strcmp(enteredPIN, MasterPIN) == 4) {
    goToState(STATE_ADMIN_MODE);
  } else {
    goToState(STATE_INCORRECT_PIN);
  }
}

void displayBoot() {
  display.clearDisplay();
  display.setCursor(0, 0);     
  display.println(F("SMART LOCK SYSTEM"));
  display.println(F("Initializing..."));
  display.display(); 
  delay(500);
}

bool checkDoorState() {
  // Ultrasonic sensor to detect if door is open
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;
//  Serial.println(distance);

  if (distance < 10){
    return false;
  } else {
    return true;
  }
}

void unlockDoor() {
  lockServo.write(180); // Unlock position
}

void lockDoor() {
  lockServo.write(90); // Lock position
}

// ================== SETUP ==================
void setup() {
  Serial.begin(9600);
//
//  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  delay(2000);         // Wait for initialization
  display.clearDisplay();

  
//  // Display static welcome message
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text

  // Start with boot screen
  jwire->begin( );

  // Initialize Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  // Start with boot screen
  displayBoot();

  kpd.begin( );

  EEPROM.put(PIN1_ADDR, "1234\0");
  EEPROM.put(PIN2_ADDR, "9999\0");
    
  // Read stored PINs into variables
  EEPROM.get(PIN1_ADDR, storedPIN);
  EEPROM.get(PIN2_ADDR, MasterPIN);

  // Initialize Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(90); // Locked position (adjust as needed)

  currentState = STATE_IDLE;
  displayIdle();
  lastActivityTime = millis();
}

// ================== MAIN LOOP ==================
void loop() {
  unsigned long currentMillis = millis();

  // 1. High-Priority Input (Check as fast as possible)
  handleKeypad();

  // 2. Low-Priority State Monitoring (e.g., every 50ms)
  static unsigned long lastMonitorTime = 0;
  if (currentMillis - lastMonitorTime >= 100) {
    monitorDoorState();
    lastMonitorTime = currentMillis;
  }

  // 3. Timers and Timeouts
  checkAutoLock(currentMillis);
}

void monitorDoorState() {
  bool currentDoorState = checkDoorState();
//  Serial.println(currentDoorState != doorOpen);
  if (currentDoorState != doorOpen) {
    doorOpen = currentDoorState;
    Serial.print(doorOpen);
    if (!doorOpen) { 
      doorCloseTime = millis(); // Mark the moment of closure
    }
    // Only print on change to save Serial bandwidth
    display.clearDisplay();
    display.setCursor(0, 0);     
    display.println(F("Door Status Changed: "));
    display.println(doorOpen ? "Open" : "Closed");
    display.display(); 
    delay(500);    
  }
}

void checkAutoLock(unsigned long now) {

  if (!doorOpen && currentState == STATE_IDLE) {
    if (now - doorCloseTime > LOCK_DELAY_MS) {
      lockDoor();
    }
  }
}



// ================== INPUT HANDLING ==================
void handleKeypad() {
  char key = kpd.getKey();
  
  if (key) {
    lastActivityTime = millis(); // Reset timeout on any key press
    keyPressStartTime = millis();
    keyHeld = true;
    
    // Handle key press based on current state
    switch (currentState) {
      case STATE_IDLE:
        goToState(STATE_PIN_ENTRY);
        break; 
      
      case STATE_ADMIN_MENU:
        displayAdminMenu();
        if (key == '1') {
          // Reset PIN
          newPinIndex = 0;
          newPIN[0] = '\0';
          display.clearDisplay();
          display.setCursor(0, 0);     
          display.println(F("1 Selected"));
          display.display(); 
          delay(1000);
          goToState(STATE_ADD_PIN_STEP1);
        } else if (key == '2') {
          display.clearDisplay();
          display.setCursor(0, 0);     
          display.println(F("2 Selected"));
          display.display(); 
          delay(1000);
          goToState(STATE_IDLE);
        }
        break;
    }
  }
}
