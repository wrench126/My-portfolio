#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <Preferences.h>

// --- Pin Definitions ---
const uint16_t kRecvPin = 15;     // IR Receiver (GPIO 15)
const uint16_t kSendPin = 14;     // IR Transmitter LED via BC547 (GPIO 14)
const int LED_GREEN     = 13;     // Status LED / IR Activity (GPIO 13)
const int LED_RED_LAMP  = 25;     // Record Mode Indicator (GPIO 25)
const int BTN_REC_PIN   = 33;     // Record / Learn Mode Button (GPIO 33)

const unsigned long DEBOUNCE_DELAY = 50; // Debounce time in ms

// Button Configuration Structure
struct RemoteButton {
  const char* name;               // Display label
  int pin;                        // ESP32 GPIO pin number
  bool useInternalPullup;         // True for internal pull-up, False for external
  const char* keyPrefix;          // NVS Flash memory storage key
  uint64_t code;                  // Saved IR code (64-bit)
  decode_type_t protocol;         // Saved IR protocol type
  uint16_t bits;                  // Saved bit length

  // Non-blocking debounce state variables
  bool lastState;
  bool lastReading;
  unsigned long lastDebounceTime;
};

// Remote Buttons Map
RemoteButton buttons[] = {
  {"UP",      32, true,  "up",  0, UNKNOWN, 0, HIGH, HIGH, 0},
  {"DOWN",    19, true,  "dn",  0, UNKNOWN, 0, HIGH, HIGH, 0},
  {"LEFT",    22, true,  "lt",  0, UNKNOWN, 0, HIGH, HIGH, 0},
  {"RIGHT",   35, false, "rt",  0, UNKNOWN, 0, HIGH, HIGH, 0}, // External 10k pull-up required
  {"EXTRA 1", 34, false, "ex1", 0, UNKNOWN, 0, HIGH, HIGH, 0}, // External 10k pull-up required
  {"EXTRA 2", 21, true,  "ex2", 0, UNKNOWN, 0, HIGH, HIGH, 0}
};

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// Record Button Debounce State
bool recLastState = HIGH;
bool recLastReading = HIGH;
unsigned long recLastDebounceTime = 0;

IRrecv irrecv(kRecvPin);
IRsend irsend(kSendPin);
Preferences pref;
decode_results results;

// Non-blocking Falling-Edge Button Press Detector
bool isJustPressed(int pin, bool &lastState, bool &lastReading, unsigned long &lastDebounceTime) {
  bool reading = digitalRead(pin);
  bool justPressed = false;

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != lastState) {
      lastState = reading;
      if (lastState == LOW) {
        justPressed = true;
      }
    }
  }

  lastReading = reading;
  return justPressed;
}

bool isButtonJustPressed(RemoteButton &btn) {
  return isJustPressed(btn.pin, btn.lastState, btn.lastReading, btn.lastDebounceTime);
}

// Load IR Codes from NVS Flash
void loadAllButtonsData() {
  pref.begin("ir_remote", true);
  
  Serial.println("\n--- [Loading Saved IR Codes from NVS] ---");
  for (int i = 0; i < NUM_BUTTONS; i++) {
    String cKey = String(buttons[i].keyPrefix) + "_c";
    String pKey = String(buttons[i].keyPrefix) + "_p";
    String bKey = String(buttons[i].keyPrefix) + "_b";

    buttons[i].code     = pref.getULong64(cKey.c_str(), 0);
    buttons[i].protocol = (decode_type_t)pref.getInt(pKey.c_str(), UNKNOWN);
    buttons[i].bits     = pref.getUInt(bKey.c_str(), 0);

    if (buttons[i].code != 0) {
      Serial.printf("Button [%-7s]: 0x", buttons[i].name);
      serialPrintUint64(buttons[i].code, HEX);
      Serial.printf(" | %s (%d bits)\n", typeToString(buttons[i].protocol).c_str(), buttons[i].bits);
    } else {
      Serial.printf("Button [%-7s]: (Empty)\n", buttons[i].name);
    }
  }
  pref.end();
  Serial.println("-----------------------------------------\n");
}

// Save Single Button IR Data to NVS Flash
void saveButtonData(int index) {
  pref.begin("ir_remote", false);
  
  String cKey = String(buttons[index].keyPrefix) + "_c";
  String pKey = String(buttons[index].keyPrefix) + "_p";
  String bKey = String(buttons[index].keyPrefix) + "_b";

  pref.putULong64(cKey.c_str(), buttons[index].code);
  pref.putInt(pKey.c_str(), (int)buttons[index].protocol);
  pref.putUInt(bKey.c_str(), buttons[index].bits);
  
  pref.end();
  Serial.printf("[NVS]: Saved data for button [%s] successfully.\n", buttons[index].name);
}

void setup() {
  Serial.begin(115200);

  // Configure Status LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED_LAMP, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED_LAMP, LOW);

  // Configure Record Button
  pinMode(BTN_REC_PIN, INPUT_PULLUP);

  // Configure Remote Control Buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttons[i].useInternalPullup) {
      pinMode(buttons[i].pin, INPUT_PULLUP);
    } else {
      pinMode(buttons[i].pin, INPUT);
    }
  }

  irsend.begin();
  irrecv.enableIRIn();

  // Load stored codes on boot
  loadAllButtonsData();

  Serial.println(">>> Universal IR Remote Ready <<<");
}

void loop() {
  // --- 1. Record Mode ---
  if (isJustPressed(BTN_REC_PIN, recLastState, recLastReading, recLastDebounceTime)) {
    digitalWrite(LED_RED_LAMP, HIGH);
    digitalWrite(LED_GREEN, LOW);
    Serial.println("\n[RECORD MODE]: Press a target button on this remote to map...");
    
    int selectedButtonIndex = -1;
    
    // Wait for button selection
    while (selectedButtonIndex == -1) {
      // Cancel record mode on second press
      if (isJustPressed(BTN_REC_PIN, recLastState, recLastReading, recLastDebounceTime)) {
        Serial.println("[RECORD MODE]: Cancelled.");
        digitalWrite(LED_RED_LAMP, LOW);
        return;
      }
      
      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (isButtonJustPressed(buttons[i])) {
          selectedButtonIndex = i;
          break;
        }
      }
      yield();
    }

    // Confirmation blink on RED LED
    digitalWrite(LED_RED_LAMP, LOW); delay(100);
    digitalWrite(LED_RED_LAMP, HIGH); delay(100);

    Serial.printf("[Target Button: %s]: Point original remote and press key...\n", buttons[selectedButtonIndex].name);
    irrecv.resume();

    // Wait for IR signal with 15s timeout
    bool captured = false;
    unsigned long startTime = millis();
    
    while (!captured && (millis() - startTime < 15000)) {
      if (irrecv.decode(&results)) {
        buttons[selectedButtonIndex].code     = results.value;
        buttons[selectedButtonIndex].protocol = results.decode_type;
        buttons[selectedButtonIndex].bits     = results.bits;

        // Persist captured IR data
        saveButtonData(selectedButtonIndex);

        digitalWrite(LED_RED_LAMP, LOW);
        digitalWrite(LED_GREEN, HIGH);
        delay(1000);
        digitalWrite(LED_GREEN, LOW);

        captured = true;
        irrecv.resume();
      }
      yield();
    }

    if (!captured) {
      Serial.println("[RECORD MODE]: Timeout! No IR signal detected.");
      digitalWrite(LED_RED_LAMP, LOW);
    }
  }

  // --- 2. Normal Transmission Mode ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (isButtonJustPressed(buttons[i])) {
      if (buttons[i].code != 0) {
        Serial.printf("[SEND]: Transmitting %s (0x", buttons[i].name);
        serialPrintUint64(buttons[i].code, HEX);
        Serial.println(") via GPIO 14");

        digitalWrite(LED_GREEN, HIGH);
        irsend.send(buttons[i].protocol, buttons[i].code, buttons[i].bits);
        delay(150);
        digitalWrite(LED_GREEN, LOW);
      } else {
        Serial.printf("[WARNING]: Button [%s] is not programmed yet!\n", buttons[i].name);
      }
    }
  }
}