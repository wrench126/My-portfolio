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
const int LED_RED_LAMP  = 25;     // Record / Auto-Scan Indicator (GPIO 25)
const int BTN_REC_PIN   = 33;     // Short Press: Record | Long Press (2.5s): Auto-Scan

const unsigned long DEBOUNCE_DELAY   = 50;   // Debounce time in ms
const unsigned long LONG_PRESS_TIME  = 2500; // 2.5 seconds hold for Auto-Scan
const unsigned long SCAN_INTERVAL    = 650;  // Time between brute-force shots (ms)

// Button Configuration Structure
struct RemoteButton {
  const char* name;               // Display label
  int pin;                        // ESP32 GPIO pin number
  bool useInternalPullup;         // True for internal pull-up, False for external
  const char* keyPrefix;          // NVS Flash memory storage key (<= 15 chars)
  uint64_t code;                  // Saved IR code (64-bit)
  decode_type_t protocol;         // Saved IR protocol type
  uint16_t bits;                  // Saved bit length

  // Non-blocking debounce state variables
  bool lastState;
  bool lastReading;
  unsigned long lastDebounceTime;
};

// Auto-Scan Database Structure
struct ScanCode {
  const char* brand;
  decode_type_t protocol;
  uint64_t code;
  uint16_t bits;
};

// Expanded Database of 50 Famous TVs & Satellite Receivers (Channel Up / CH+ Codes)
const ScanCode scanDatabase[] = {
  // --- Major Global & Local TVs (CH+) ---
  {"Samsung TV",            SAMSUNG,    0xE0E048B7,     32},
  {"LG TV",                 NEC,        0x20DF00FF,     32},
  {"Sony TV (12-bit)",      SONY,       0x090,          12},
  {"Sony TV (15-bit)",      SONY,       0x090,          15},
  {"Sony TV (20-bit)",      SONY,       0x090,          20},
  {"Toshiba TV",            NEC,        0x2FDD827,      32},
  {"Tornado / Sharp NEC",   NEC,        0x1FE58A7,      32},
  {"Sharp TV (15-bit)",     SHARP,      0x4122,         15},
  {"Panasonic TV",          PANASONIC,  0x400401002C2D, 48},
  {"Philips TV (RC5)",      RC5,        0x020,          12},
  {"Philips TV (RC6)",      RC6,        0x00020,        20},
  {"Hisense TV",            NEC,        0x00FD52AD,     32},
  {"TCL / Thomson TV",      NEC,        0x40BE02FD,     32},
  {"Xiaomi / Mi TV",        NEC,        0x807F18E7,     32},
  {"Vizio TV",              NEC,        0x04FB00FF,     32},
  {"JVC TV",                JVC,        0xC508,         16},
  {"Pioneer TV",            PIONEER,    0xA55A08F7,     32},
  {"Sanyo TV",              NEC,        0x1CE350AF,     32},
  {"Hitachi TV",            NEC,        0x50AF08F7,     32},
  {"Haier TV",              NEC,        0x807F08F7,     32},
  {"Skyworth TV",           NEC,        0x0E0E48B7,     32},
  {"Changhong TV",          NEC,        0x40BF58A7,     32},
  {"Konka TV",              NEC,        0x02FD58A7,     32},
  {"Daewoo TV",             NEC,        0x55AA08F7,     32},
  {"Grundig / Beko TV",     NEC,        0x40BF08F7,     32},
  {"RCA TV",                NEC,        0xF4BA20DF,     32},
  {"Mitsubishi TV",         MITSUBISHI, 0xE220,         16},
  {"Insignia TV",           NEC,        0x61A050AF,     32},
  {"Roku TV",               NEC,        0x57E308F7,     32},
  {"Vestel / Finlux TV",    NEC,        0x00FF40BF,     32},
  {"Unionaire / Generic1",  NEC,        0x00FF18E7,     32},
  {"JAC / Generic2",        NEC,        0x807F18E7,     32},
  {"ATA / Generic3",        NEC,        0x00FE18E7,     32},
  {"Fresh / HoHo TV",       NEC,        0x00FF58A7,     32},
  {"Syinix / Itel TV",      NEC,        0x40BF18E7,     32},

  // --- Satellite Receivers / Set-Top Boxes (CH+) ---
  {"Receiver (Sunplus 1)",  NEC,        0x00FF18E7,     32},
  {"Receiver (Sunplus 2)",  NEC,        0x00FF807F,     32},
  {"Receiver (GX6605/Ali)", NEC,        0x04FB18E7,     32},
  {"Receiver (Ali Alt)",    NEC,        0x807F807F,     32},
  {"Starsat / Tiger Box",   NEC,        0x00FF9867,     32},
  {"Senator / Nova Box",    NEC,        0x00FF609F,     32},
  {"Humax Receiver",        NEC,        0x000818E7,     32},
  {"Strong Receiver",       NEC,        0x01FE18E7,     32},
  {"Dreambox / Enigma2",    NEC,        0x40BF807F,     32},
  {"Trumann / Astra Box",   NEC,        0x20DF807F,     32},
  {"Qmax / Salik Box",      NEC,        0x00FFA05F,     32},
  {"beIN Sports Receiver",  NEC,        0x00FF30CF,     32},
  {"OSN / Nilesat Box",     NEC,        0x4FB018E7,     32},
  {"China Receiver Type A", NEC,        0xFF0018E7,     32},
  {"China Receiver Type B", NEC,        0x02FD18E7,     32}
};

const int TOTAL_SCAN_CODES = sizeof(scanDatabase) / sizeof(scanDatabase[0]);

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

// Record Button Debounce & Long-Press State
bool recLastState = HIGH;
bool recLastReading = HIGH;
unsigned long recLastDebounceTime = 0;
unsigned long recPressStartTime = 0;
bool recLongPressHandled = false;

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

// Helper: Check if ANY button is pressed (used to stop Auto-Scan)
bool isAnyButtonPressed() {
  if (isJustPressed(BTN_REC_PIN, recLastState, recLastReading, recLastDebounceTime)) return true;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (isButtonJustPressed(buttons[i])) return true;
  }
  return false;
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

// Helper: Wait for user to pick a target button
int waitForTargetButton(bool isAutoScan) {
  int selectedIndex = -1;
  unsigned long waitStart = millis();

  // 20-second safety timeout so the device never gets stuck
  while (selectedIndex == -1 && (millis() - waitStart < 20000)) {
    if (isJustPressed(BTN_REC_PIN, recLastState, recLastReading, recLastDebounceTime)) {
      Serial.println(isAutoScan ? "[AUTO-SCAN]: Cancelled." : "[RECORD MODE]: Cancelled.");
      digitalWrite(LED_RED_LAMP, LOW);
      return -1;
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (isButtonJustPressed(buttons[i])) {
        selectedIndex = i;
        break;
      }
    }
    yield(); // Feed Watchdog Timer
  }

  if (selectedIndex == -1) {
    Serial.println("[TIMEOUT]: No button selected.");
    digitalWrite(LED_RED_LAMP, LOW);
  }
  return selectedIndex;
}

// --- Mode 1: Normal IR Learning Mode ---
void runRecordMode() {
  digitalWrite(LED_RED_LAMP, HIGH);
  digitalWrite(LED_GREEN, LOW);
  Serial.println("\n[RECORD MODE]: Press a target button on this remote to map...");

  int targetIdx = waitForTargetButton(false);
  if (targetIdx == -1) return;

  // Confirmation blink on RED LED
  digitalWrite(LED_RED_LAMP, LOW); delay(100);
  digitalWrite(LED_RED_LAMP, HIGH); delay(100);

  Serial.printf("[Target Button: %s]: Point original remote and press key...\n", buttons[targetIdx].name);
  irrecv.resume();

  bool captured = false;
  unsigned long startTime = millis();

  while (!captured && (millis() - startTime < 15000)) {
    if (irrecv.decode(&results)) {
      buttons[targetIdx].code     = results.value;
      buttons[targetIdx].protocol = results.decode_type;
      buttons[targetIdx].bits     = results.bits;

      saveButtonData(targetIdx);

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

// --- Mode 2: Auto-Scan (IR Brute-Force CH+) Mode ---
void runAutoScanMode() {
  // Fast 5x blink to indicate Auto-Scan activation (release BTN_REC now)
  for (int b = 0; b < 5; b++) {
    digitalWrite(LED_RED_LAMP, HIGH); delay(70);
    digitalWrite(LED_RED_LAMP, LOW);  delay(70);
  }
  digitalWrite(LED_RED_LAMP, HIGH);

  Serial.println("\n=== [AUTO-SCAN MODE ACTIVATED (CH+)] ===");
  Serial.println("Select the target button to store the discovered CH+ code...");

  // Wait until user releases BTN_REC first
  while (digitalRead(BTN_REC_PIN) == LOW) { yield(); }
  recLastState = HIGH;
  recLastReading = HIGH;

  int targetIdx = waitForTargetButton(true);
  if (targetIdx == -1) return;

  Serial.printf("[Target: %s] Point remote at TV/Receiver! Press ANY button when channel changes...\n", buttons[targetIdx].name);
  delay(500);

  int currentCodeIdx = -1;
  unsigned long lastShotTime = 0;
  bool locked = false;

  while (currentCodeIdx < TOTAL_SCAN_CODES - 1 && !locked) {
    // Non-blocking interval timer between IR shots
    if (millis() - lastShotTime >= SCAN_INTERVAL) {
      lastShotTime = millis();
      currentCodeIdx++;

      Serial.printf("[%d/%d] Testing CH+: %-22s (0x", currentCodeIdx + 1, TOTAL_SCAN_CODES, scanDatabase[currentCodeIdx].brand);
      serialPrintUint64(scanDatabase[currentCodeIdx].code, HEX);
      Serial.println(")");

      digitalWrite(LED_GREEN, HIGH);
      irsend.send(scanDatabase[currentCodeIdx].protocol,
                  scanDatabase[currentCodeIdx].code,
                  scanDatabase[currentCodeIdx].bits);
      digitalWrite(LED_GREEN, LOW);
    }

    // Check instantly if user pressed ANY button to lock the working code
    if (currentCodeIdx >= 0 && isAnyButtonPressed()) {
      locked = true;
      break;
    }

    yield(); // Keep Watchdog happy
  }

  if (locked) {
    buttons[targetIdx].code     = scanDatabase[currentCodeIdx].code;
    buttons[targetIdx].protocol = scanDatabase[currentCodeIdx].protocol;
    buttons[targetIdx].bits     = scanDatabase[currentCodeIdx].bits;

    saveButtonData(targetIdx);

    Serial.printf(">>> LOCKED! [%s] mapped to [%s CH+] <<<\n", buttons[targetIdx].name, scanDatabase[currentCodeIdx].brand);
    digitalWrite(LED_RED_LAMP, LOW);
    digitalWrite(LED_GREEN, HIGH);
    delay(1200);
    digitalWrite(LED_GREEN, LOW);
  } else {
    Serial.println("[AUTO-SCAN]: Finished all 50 codes without lock.");
    digitalWrite(LED_RED_LAMP, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  // Configure Status LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED_LAMP, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED_LAMP, LOW);

  // Configure Record / Auto-Scan Button
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

  Serial.println(">>> Universal IR Remote Ready (Short Press REC: Learn | Hold 2.5s: Auto-Scan CH+) <<<");
}

void loop() {
  // --- 1. Handle Record Button (Short Press vs Long Press) ---
  bool recReading = digitalRead(BTN_REC_PIN);

  if (recReading != recLastReading) {
    recLastDebounceTime = millis();
  }

  if ((millis() - recLastDebounceTime) > DEBOUNCE_DELAY) {
    if (recReading != recLastState) {
      recLastState = recReading;

      if (recLastState == LOW) {
        recPressStartTime = millis();
        recLongPressHandled = false;
      } else {
        if (!recLongPressHandled) {
          runRecordMode();
        }
      }
    }

    // Check if held down for >= 2.5 seconds
    if (recLastState == LOW && !recLongPressHandled && (millis() - recPressStartTime >= LONG_PRESS_TIME)) {
      recLongPressHandled = true;
      runAutoScanMode();
    }
  }
  recLastReading = recReading;

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