// ============================================================================
//  PROJECT: Universal ESP32 IR Remote (v2.1.0 - Non-Blocking FSM Edition)
//  FEATURES:
//    - 100% Non-Blocking Finite State Machine (Zero delay() calls)
//    - Standard Protocols + Raw IR Capture (AC Remotes & Elevator Programmers)
//    - 50-Device Brute-Force Auto-Scan with Collision Skip & Target Button Select
//    - Continuous Hold-Repeat (every 500ms) & Ultra-Low-Power Deep Sleep (60s)
// ============================================================================

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <Preferences.h>
#include <esp_sleep.h>

// ============================================================================
//  SECTION 1: HARDWARE & PIN CONFIGURATION
// ============================================================================
const uint16_t kRecvPin      = 15;  // IR Receiver Sensor (GPIO 15)
const uint16_t kSendPin      = 14;  // IR LED Driver via 2N2222 NPN + 390R (GPIO 14)
const int      LED_GREEN     = 13;  // Green LED: TX Activity & Confirmation (GPIO 13)
const int      LED_RED_LAMP  = 25;  // Red LED: Solid = Learn Mode | Blink = Brute Force
const int      BTN_REC_PIN   = 33;  // Short: Learn | Hold 2.5s: Brute Force | RTC Wakeup

// ============================================================================
//  SECTION 2: TIMING & BUFFER CONSTANTS
// ============================================================================
const unsigned long DEBOUNCE_DELAY     = 50;     // Button debounce filter (ms)
const unsigned long REPEAT_INTERVAL    = 500;    // Continuous hold repeat interval (0.5s)
const unsigned long LONG_PRESS_TIME    = 2500;   // Hold duration to enter Brute Force (2.5s)
const unsigned long SCAN_INTERVAL      = 650;    // Time between Brute Force POWER shots (ms)
const unsigned long SCAN_BLINK_RATE    = 250;    // Red LED blink speed during Brute Force (ms)
const unsigned long INACTIVITY_TIMEOUT = 60000;  // Idle timeout before Deep Sleep (1 minute)
const unsigned long MODE_TIMEOUT       = 25000;  // Safety timeout for Learn/Select states (25s)

// Expanded IR Capture Buffers (Supports long AC & Elevator Programmer frames)
const uint16_t kCaptureBufferSize = 1024;
const uint8_t  kTimeout           = 50;     // 50ms packet end timeout
const uint16_t kMaxRawLen         = 350;    // Max raw pulses stored per button in NVS

// ============================================================================
//  SECTION 3: DATA STRUCTURES & FINITE STATE MACHINE (FSM)
// ============================================================================
enum SystemState {
  STATE_IDLE,                 // Normal remote operation & sleep monitoring
  STATE_RECORD_SELECT_BTN,    // Learn Mode (Solid Red): Waiting for target button
  STATE_RECORD_WAIT_IR,       // Learn Mode (Solid Red): Waiting for IR signal
  STATE_AUTOSCAN_RUNNING,     // Brute Force (Blinking Red): Testing POWER codes
  STATE_AUTOSCAN_SELECT_BTN   // Brute Force (Blinking Red): Assigning matched code to a button
};

struct RemoteButton {
  const char*   name;
  int           pin;
  bool          useInternalPullup;
  const char*   keyPrefix;          // NVS namespace key prefix (max 15 chars)

  // Standard Protocol Storage
  uint64_t      code;
  decode_type_t protocol;
  uint16_t      bits;

  // Raw IR Storage (For AC & Elevator Remotes)
  bool          isRaw;
  uint16_t      rawLen;
  uint16_t      rawData[kMaxRawLen];

  // Non-blocking Debounce & Hold-Repeat State
  bool          lastState;
  bool          lastReading;
  unsigned long lastDebounceTime;
  unsigned long lastRepeatTime;
};

// Device Profile: [0]=UP(CH+), [1]=DOWN(CH-), [2]=LEFT(VOL-), [3]=RIGHT(VOL+), [4]=EXTRA1(POWER), [5]=EXTRA2(MUTE)
struct DeviceProfile {
  const char*   brand;
  decode_type_t protocol;
  uint16_t      bits;
  uint64_t      codes[6];
};

// ============================================================================
//  SECTION 4: 50-DEVICE INFRARED DATABASE (STORED IN FLASH)
// ============================================================================
const DeviceProfile deviceDatabase[] = {
 {"Custom TV (0x00FF Type A)", NEC,    32, {0x00FF18E7, 0x00FF9867, 0x00FF58A7, 0x00FFD827, 0x00FF38C7, 0x00FF08F7}},
  {"Custom TV (0x00FF Type B)", NEC,    32, {0x00FF18E7, 0x00FF4AB5, 0x00FF10EF, 0x00FF5AA5, 0x00FF38C7, 0x00FFB04F}},
  {"Samsung TV",            SAMSUNG,    32, {0xE0E048B7, 0xE0E008F7, 0xE0E0D02F, 0xE0E0E01F, 0xE0E040BF, 0xE0E0F00F}},
  {"LG TV",                 NEC,        32, {0x20DF00FF, 0x20DF807F, 0x20DFC03F, 0x20DF40BF, 0x20DF10EF, 0x20DF906F}},
  {"Sony TV (12-bit)",      SONY,       12, {0x090,      0x890,      0xC90,      0x490,      0xA90,      0x290}},
  {"Sony TV (15-bit)",      SONY,       15, {0x090,      0x890,      0xC90,      0x490,      0xA90,      0x290}},
  {"Sony TV (20-bit)",      SONY,       20, {0x090,      0x890,      0xC90,      0x490,      0xA90,      0x290}},
  {"Toshiba TV",            NEC,        32, {0x2FDD827,  0x2FDF807,  0x2FD7887,  0x2FD58A7,  0x2FD48B7,  0x2FD08F7}},
  {"Tornado / Sharp NEC",   NEC,        32, {0x1FE58A7,  0x1FE7887,  0x1FED827,  0x1FEF807,  0x1FE48B7,  0x1FE08F7}},
  {"Sharp TV (15-bit)",     SHARP,      15, {0x4122,     0x4222,     0x42A2,     0x40A2,     0x41A2,     0x43A2}},
  {"Panasonic TV",          PANASONIC,  48, {0x400401002C2D, 0x40040100ACAD, 0x400401008485, 0x400401000405, 0x40040100BCBD, 0x400401004C4D}},
  {"Philips TV (RC5)",      RC5,        12, {0x020,      0x021,      0x011,      0x010,      0x00C,      0x00D}},
  {"Philips TV (RC6)",      RC6,        20, {0x00020,    0x00021,    0x00011,    0x00010,    0x0000C,    0x0000D}},
  {"Hisense TV",            NEC,        32, {0x00FD52AD, 0x00FDD22D, 0x00FD629D, 0x00FD22DD, 0x00FDB04F, 0x00FD708F}},
  {"TCL / Thomson TV",      NEC,        32, {0x40BE02FD, 0x40BE827D, 0x40BEC23D, 0x40BE42BD, 0x40BE629D, 0x40BEA25D}},
  {"Xiaomi / Mi TV",        NEC,        32, {0x807F18E7, 0x807F9867, 0x807F58A7, 0x807FD827, 0x807F02FD, 0x807F48B7}},
  {"Vizio TV",              NEC,        32, {0x04FB00FF, 0x04FB807F, 0x04FBC03F, 0x04FB40BF, 0x04FB4AB5, 0x04FB906F}},
  {"JVC TV",                JVC,        16, {0xC508,     0xC588,     0xC548,     0xC5C8,     0xC5E8,     0xC538}},
  {"Pioneer TV",            PIONEER,    32, {0xA55A08F7, 0xA55A8877, 0xA55AD02F, 0xA55A50AF, 0xA55A38C7, 0xA55A48B7}},
  {"Sanyo TV",              NEC,        32, {0x1CE350AF, 0x1CE3D02F, 0x1CE3F00F, 0x1CE3708F, 0x1CE348B7, 0x1CE318E7}},
  {"Hitachi TV",            NEC,        32, {0x50AF08F7, 0x50AF8877, 0x50AF708F, 0x50AFB04F, 0x50AF3EC1, 0x50AFD02F}},
  {"Haier TV",              NEC,        32, {0x807F08F7, 0x807F8877, 0x807FC837, 0x807F48B7, 0x807F807F, 0x807F28D7}},
  {"Skyworth TV",           NEC,        32, {0x0E0E48B7, 0x0E0E08F7, 0x0E0ED02F, 0x0E0EE01F, 0x0E0E40BF, 0x0E0EF00F}},
  {"Changhong TV",          NEC,        32, {0x40BF58A7, 0x40BF7887, 0x40BFD827, 0x40BFF807, 0x40BF48B7, 0x40BF08F7}},
  {"Konka TV",              NEC,        32, {0x02FD58A7, 0x02FD7887, 0x02FDD827, 0x02FDF807, 0x02FD00FF, 0x02FD08F7}},
  {"Daewoo TV",             NEC,        32, {0x55AA08F7, 0x55AA8877, 0x55AAC837, 0x55AA48B7, 0x55AA38C7, 0x55AA28D7}},
  {"Grundig / Beko TV",     NEC,        32, {0x40BF08F7, 0x40BF8877, 0x40BFC837, 0x40BF48B7, 0x40BF00FF, 0x40BF28D7}},
  {"RCA TV",                NEC,        32, {0xF4BA20DF, 0xF4BAA05F, 0xF4BAE01F, 0xF4BA609F, 0xF4BA2988, 0xF4BA10EF}},
  {"Mitsubishi TV",         MITSUBISHI, 16, {0xE220,     0xE2A0,     0xE260,     0xE2E0,     0xE240,     0xE210}},
  {"Insignia TV",           NEC,        32, {0x61A050AF, 0x61A0D02F, 0x61A0B04F, 0x61A030CF, 0x61A0F00F, 0x61A0708F}},
  {"Roku TV",               NEC,        32, {0x57E308F7, 0x57E38877, 0x57E3F00F, 0x57E3708F, 0x57E3E817, 0x57E304FB}},
  {"Vestel / Finlux TV",    NEC,        32, {0x00FF40BF, 0x00FFC03F, 0x00FF20DF, 0x00FFA05F, 0x00FF00FF, 0x00FF807F}},
  {"Unionaire / Generic1",  NEC,        32, {0x00FF18E7, 0x00FF9867, 0x00FF58A7, 0x00FFD827, 0x00FF00FF, 0x00FF48B7}},
  {"JAC / Generic2",        NEC,        32, {0x807F18E7, 0x807F9867, 0x807F58A7, 0x807FD827, 0x807F00FF, 0x807F48B7}},
  {"ATA / Generic3",        NEC,        32, {0x00FE18E7, 0x00FE9867, 0x00FE58A7, 0x00FED827, 0x00FE50AF, 0x00FE48B7}},
  {"Fresh / HoHo TV",       NEC,        32, {0x00FF58A7, 0x00FF7887, 0x00FFD827, 0x00FFF807, 0x00FF48B7, 0x00FF08F7}},
  {"Syinix / Itel TV",      NEC,        32, {0x40BF18E7, 0x40BF9867, 0x40BF58A7, 0x40BFD827, 0x40BF00FF, 0x40BF48B7}},
  {"Receiver (Sunplus 1)",  NEC,        32, {0x00FF18E7, 0x00FF9867, 0x00FF58A7, 0x00FFD827, 0x00FF48B7, 0x00FF08F7}},
  {"Receiver (Sunplus 2)",  NEC,        32, {0x00FF807F, 0x00FF40BF, 0x00FF20DF, 0x00FFA05F, 0x00FF00FF, 0x00FF609F}},
  {"Receiver (GX6605/Ali)", NEC,        32, {0x04FB18E7, 0x04FB9867, 0x04FB58A7, 0x04FBD827, 0x04FB08F7, 0x04FB48B7}},
  {"Receiver (Ali Alt)",    NEC,        32, {0x807F807F, 0x807F40BF, 0x807F20DF, 0x807FA05F, 0x807F00FF, 0x807F609F}},
  {"Starsat / Tiger Box",   NEC,        32, {0x00FF9867, 0x00FF18E7, 0x00FFD827, 0x00FF58A7, 0x00FF30CF, 0x00FFB04F}},
  {"Senator / Nova Box",    NEC,        32, {0x00FF609F, 0x00FFE01F, 0x00FF20DF, 0x00FFA05F, 0x00FF00FF, 0x00FF40BF}},
  {"Humax Receiver",        NEC,        32, {0x000818E7, 0x00089867, 0x000858A7, 0x0008D827, 0x000800FF, 0x000848B7}},
  {"Strong Receiver",       NEC,        32, {0x01FE18E7, 0x01FE9867, 0x01FE58A7, 0x01FED827, 0x01FE48B7, 0x01FE08F7}},
  {"Dreambox / Enigma2",    NEC,        32, {0x40BF807F, 0x40BF40BF, 0x40BF20DF, 0x40BFA05F, 0x40BF00FF, 0x40BF609F}},
  {"Trumann / Astra Box",   NEC,        32, {0x20DF807F, 0x20DF40BF, 0x20DF20DF, 0x20DFA05F, 0x20DF00FF, 0x20DF609F}},
  {"Qmax / Salik Box",      NEC,        32, {0x00FFA05F, 0x00FF20DF, 0x00FF609F, 0x00FFE01F, 0x00FF00FF, 0x00FF807F}},
  {"beIN Sports Receiver",  NEC,        32, {0x00FF30CF, 0x00FFB04F, 0x00FF708F, 0x00FFF00F, 0x00FF00FF, 0x00FF50AF}},
  {"OSN / Nilesat Box",     NEC,        32, {0x4FB018E7, 0x4FB09867, 0x4FB058A7, 0x4FB0D827, 0x4FB048B7, 0x4FB008F7}},
  {"China Receiver Type A", NEC,        32, {0xFF0018E7, 0xFF009867, 0xFF0058A7, 0xFF00D827, 0xFF0048B7, 0xFF0008F7}},
  {"China Receiver Type B", NEC,        32, {0x02FD18E7, 0x02FD9867, 0x02FD58A7, 0x02FDD827, 0x02FD48B7, 0x02FD08F7}}
};

const int TOTAL_DEVICES = sizeof(deviceDatabase) / sizeof(deviceDatabase[0]);

// ============================================================================
//  SECTION 5: GLOBAL OBJECTS & RUNTIME VARIABLES
// ============================================================================
RemoteButton buttons[] = {
  {"UP",      32, true,  "up",  0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0},
  {"DOWN",    19, true,  "dn",  0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0},
  {"LEFT",    22, true,  "lt",  0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0},
  {"RIGHT",   35, false, "rt",  0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0},
  {"EXTRA 1", 34, false, "ex1", 0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0},
  {"EXTRA 2", 21, true,  "ex2", 0, UNKNOWN, 0, false, 0, {0}, HIGH, HIGH, 0, 0}
};

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

SystemState   currentState         = STATE_IDLE;
unsigned long lastActivityTime     = 0;
unsigned long stateStartTime       = 0;
int           recordTargetIndex    = -1;

// Brute-Force Auto-Scan State
int           scanDevIndex         = -1;
unsigned long lastScanShotTime     = 0;
bool          waitingForRecRelease = false;

// REC Button Debounce State
bool          recLastState         = HIGH;
bool          recLastReading       = HIGH;
unsigned long recLastDebounceTime  = 0;
unsigned long recPressStartTime    = 0;
bool          recLongPressHandled  = false;

// Non-Blocking LED Control Timers
unsigned long greenLedOffTime      = 0;
unsigned long redLedOffTime        = 0;
unsigned long lastRedBlinkTime     = 0;
bool          redBlinkState        = false;

// Hardware Peripheral Instances
IRrecv         irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
IRsend         irsend(kSendPin);
Preferences    pref;
decode_results results;

// ============================================================================
//  SECTION 6: NON-BLOCKING LED & INPUT DRIVERS
// ============================================================================
void pulseGreenLED(unsigned long durationMs) {
  digitalWrite(LED_GREEN, HIGH);
  greenLedOffTime = millis() + durationMs;
}

void pulseRedLED(unsigned long durationMs) {
  digitalWrite(LED_RED_LAMP, HIGH);
  redLedOffTime = millis() + durationMs;
}

// Updates LED states cleanly based on active SystemState (Solid vs Blinking)
void updateLEDs() {
  unsigned long now = millis();

  // 1. Green LED auto-off timer
  if (greenLedOffTime > 0 && now >= greenLedOffTime) {
    digitalWrite(LED_GREEN, LOW);
    greenLedOffTime = 0;
  }

  // 2. Red LED Mode Indicator
  if (currentState == STATE_RECORD_SELECT_BTN || currentState == STATE_RECORD_WAIT_IR) {
    // LEARN MODE -> Solid ON
    digitalWrite(LED_RED_LAMP, HIGH);
  } 
  else if (currentState == STATE_AUTOSCAN_RUNNING || currentState == STATE_AUTOSCAN_SELECT_BTN) {
    // BRUTE FORCE MODE -> Blinking every 250ms
    if (now - lastRedBlinkTime >= SCAN_BLINK_RATE) {
      lastRedBlinkTime = now;
      redBlinkState = !redBlinkState;
      digitalWrite(LED_RED_LAMP, redBlinkState ? HIGH : LOW);
    }
  } 
  else {
    // IDLE MODE -> Off (unless temporary warning pulse is active)
    if (redLedOffTime > 0) {
      if (now >= redLedOffTime) {
        digitalWrite(LED_RED_LAMP, LOW);
        redLedOffTime = 0;
      }
    } else {
      digitalWrite(LED_RED_LAMP, LOW);
    }
  }
}

// Evaluates button press & 500ms continuous hold-repeat
// Returns: 0 = None | 1 = Initial Press | 2 = Hold Repeat (every 500ms)
int checkButtonAction(RemoteButton &btn) {
  bool reading = digitalRead(btn.pin);
  int action = 0;
  unsigned long now = millis();

  // Keep system awake continuously while ANY button is physically held down
  if (reading == LOW) {
    lastActivityTime = now;
  }

  if (reading != btn.lastReading) {
    btn.lastDebounceTime = now;
  }

  if ((now - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != btn.lastState) {
      btn.lastState = reading;
      if (btn.lastState == LOW) {
        action = 1; // Initial press
        btn.lastRepeatTime = now;
        lastActivityTime = now;
      }
    } else if (btn.lastState == LOW) {
      // Trigger repeat every 500ms while held down
      if (now - btn.lastRepeatTime >= REPEAT_INTERVAL) {
        action = 2; // Repeat press
        btn.lastRepeatTime = now;
        lastActivityTime = now;
      }
    }
  }

  btn.lastReading = reading;
  return action;
}

// ============================================================================
//  SECTION 7: NVS FLASH STORAGE MANAGER
// ============================================================================
void loadAllButtonsData() {
  pref.begin("ir_remote", true);
  Serial.println("\n--- [Loading Saved IR Profiles from NVS] ---");

  for (int i = 0; i < NUM_BUTTONS; i++) {
    String cKey  = String(buttons[i].keyPrefix) + "_c";
    String pKey  = String(buttons[i].keyPrefix) + "_p";
    String bKey  = String(buttons[i].keyPrefix) + "_b";
    String rKey  = String(buttons[i].keyPrefix) + "_r";
    String rlKey = String(buttons[i].keyPrefix) + "_rl";

    buttons[i].isRaw    = pref.getBool(rKey.c_str(), false);
    buttons[i].rawLen   = pref.getUInt(rlKey.c_str(), 0);
    buttons[i].code     = pref.getULong64(cKey.c_str(), 0);
    buttons[i].protocol = (decode_type_t)pref.getInt(pKey.c_str(), UNKNOWN);
    buttons[i].bits     = pref.getUInt(bKey.c_str(), 0);

    if (buttons[i].isRaw && buttons[i].rawLen > 0 && buttons[i].rawLen <= kMaxRawLen) {
      pref.getBytes(String(buttons[i].keyPrefix).c_str(), buttons[i].rawData, buttons[i].rawLen * sizeof(uint16_t));
      Serial.printf("Button [%-7s]: [RAW SIGNAL] (%d pulses)\n", buttons[i].name, buttons[i].rawLen);
    } else if (buttons[i].code != 0) {
      Serial.printf("Button [%-7s]: 0x", buttons[i].name);
      serialPrintUint64(buttons[i].code, HEX);
      Serial.printf(" | %s (%d bits)\n", typeToString(buttons[i].protocol).c_str(), buttons[i].bits);
    } else {
      Serial.printf("Button [%-7s]: (Empty)\n", buttons[i].name);
    }
  }
  pref.end();
  Serial.println("--------------------------------------------\n");
}

void saveButtonData(int index) {
  pref.begin("ir_remote", false);

  String cKey  = String(buttons[index].keyPrefix) + "_c";
  String pKey  = String(buttons[index].keyPrefix) + "_p";
  String bKey  = String(buttons[index].keyPrefix) + "_b";
  String rKey  = String(buttons[index].keyPrefix) + "_r";
  String rlKey = String(buttons[index].keyPrefix) + "_rl";

  pref.putBool(rKey.c_str(), buttons[index].isRaw);
  pref.putUInt(rlKey.c_str(), buttons[index].rawLen);
  pref.putULong64(cKey.c_str(), buttons[index].code);
  pref.putInt(pKey.c_str(), (int)buttons[index].protocol);
  pref.putUInt(bKey.c_str(), buttons[index].bits);

  if (buttons[index].isRaw && buttons[index].rawLen > 0) {
    pref.putBytes(String(buttons[index].keyPrefix).c_str(), buttons[index].rawData, buttons[index].rawLen * sizeof(uint16_t));
  }

  pref.end();
  Serial.printf("[NVS]: Saved [%s] successfully.\n", buttons[index].name);
}

void saveAllButtonsData() {
  pref.begin("ir_remote", false);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    String cKey  = String(buttons[i].keyPrefix) + "_c";
    String pKey  = String(buttons[i].keyPrefix) + "_p";
    String bKey  = String(buttons[i].keyPrefix) + "_b";
    String rKey  = String(buttons[i].keyPrefix) + "_r";

    pref.putBool(rKey.c_str(), false);
    pref.putULong64(cKey.c_str(), buttons[i].code);
    pref.putInt(pKey.c_str(), (int)buttons[i].protocol);
    pref.putUInt(bKey.c_str(), buttons[i].bits);
  }

  pref.end();
  Serial.println("[NVS]: All 6 buttons mapped and saved to Flash!");
}

// ============================================================================
//  SECTION 8: INFRARED TRANSMISSION & POWER MANAGEMENT
// ============================================================================
void transmitButton(int idx, bool isRepeat) {
  if (buttons[idx].isRaw && buttons[idx].rawLen > 0) {
    Serial.printf("[TX %s]: Sending RAW Signal [%s] (%d pulses)\n", isRepeat ? "HOLD" : "ONCE", buttons[idx].name, buttons[idx].rawLen);
    pulseGreenLED(100);
    irsend.sendRaw(buttons[idx].rawData, buttons[idx].rawLen, 38);
  } else if (buttons[idx].code != 0) {
    Serial.printf("[TX %s]: Sending %s (0x", isRepeat ? "HOLD" : "ONCE", buttons[idx].name);
    serialPrintUint64(buttons[idx].code, HEX);
    Serial.println(")");
    pulseGreenLED(100);
    irsend.send(buttons[idx].protocol, buttons[idx].code, buttons[idx].bits);
  } else if (!isRepeat) {
    Serial.printf("[WARNING]: Button [%s] is empty!\n", buttons[idx].name);
    pulseRedLED(150);
  }
}

void enterDeepSleep() {
  Serial.println("\n[SLEEP]: 1 minute of inactivity detected. Entering Deep Sleep...");
  Serial.println("[SLEEP]: Press REC button (GPIO 33) to wake up.\n");
  Serial.flush();

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED_LAMP, LOW);

  // Configure EXT0 Wakeup on RTC GPIO 33 (Active LOW)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
  esp_deep_sleep_start();
}

// ============================================================================
//  SECTION 9: SETUP & MAIN NON-BLOCKING LOOP
// ============================================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED_LAMP, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED_LAMP, LOW);

  pinMode(BTN_REC_PIN, INPUT_PULLUP);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, buttons[i].useInternalPullup ? INPUT_PULLUP : INPUT);
  }

  irsend.begin();
  irrecv.enableIRIn();

  loadAllButtonsData();
  lastActivityTime = millis();

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println(">>> Woke up from Deep Sleep! <<<");
    pulseGreenLED(250);
  }

  Serial.println(">>> Universal IR Remote v2.1.0 Ready <<<");
}

void loop() {
  unsigned long now = millis();
  updateLEDs();

  // --------------------------------------------------------------------------
  //  A. REC BUTTON HANDLER (Short Press = Learn | Hold 2.5s = Brute Force)
  // --------------------------------------------------------------------------
  bool recReading = digitalRead(BTN_REC_PIN);

  // Prevent Deep Sleep while REC button is held down
  if (recReading == LOW) {
    lastActivityTime = now;
  }

  if (recReading != recLastReading) {
    recLastDebounceTime = now;
  }

  if ((now - recLastDebounceTime) > DEBOUNCE_DELAY) {
    if (recReading != recLastState) {
      recLastState = recReading;
      lastActivityTime = now;

      if (recLastState == LOW) {
        recPressStartTime = now;
        recLongPressHandled = false;
      } else {
        // Button released before 2.5s -> Handle Short Press
        if (!recLongPressHandled) {
          if (currentState == STATE_IDLE) {
            currentState = STATE_RECORD_SELECT_BTN;
            stateStartTime = now;
            Serial.println("\n[LEARN MODE - Red LED Solid]: Press target button to program (or REC to cancel)...");
          } 
          else if (currentState == STATE_AUTOSCAN_SELECT_BTN) {
            // User chose to auto-map ALL 6 buttons for the confirmed device
            for (int i = 0; i < NUM_BUTTONS; i++) {
              buttons[i].isRaw    = false;
              buttons[i].rawLen   = 0;
              buttons[i].code     = deviceDatabase[scanDevIndex].codes[i];
              buttons[i].protocol = deviceDatabase[scanDevIndex].protocol;
              buttons[i].bits     = deviceDatabase[scanDevIndex].bits;
            }
            saveAllButtonsData();
            Serial.printf("\n>>> AUTO-MAPPED ALL 6 BUTTONS FOR: [%s] <<<\n", deviceDatabase[scanDevIndex].brand);
            currentState = STATE_IDLE;
            pulseGreenLED(1000);
          } 
          else {
            // Cancel any active mode and return to IDLE
            currentState = STATE_IDLE;
            Serial.println("[CANCELLED]: Returned to normal mode.");
          }
        }
      }
    }

    // Trigger Brute Force Auto-Scan after 2.5s hold
    if (recLastState == LOW && !recLongPressHandled && (now - recPressStartTime >= LONG_PRESS_TIME)) {
      recLongPressHandled  = true;
      currentState         = STATE_AUTOSCAN_RUNNING;
      scanDevIndex         = -1;
      lastScanShotTime     = 0;
      waitingForRecRelease = true;
      Serial.println("\n=== [BRUTE FORCE STARTED - Red LED Blinking] ===");
      Serial.println("Testing POWER codes: [UP]=Confirm Match | [DOWN]=Skip Collision | [REC]=Cancel");
    }
  }
  recLastReading = recReading;

  // --------------------------------------------------------------------------
  //  B. FINITE STATE MACHINE EXECUTION (100% NON-BLOCKING)
  // --------------------------------------------------------------------------
  switch (currentState) {

    // ========================================================================
    // STATE 0: NORMAL OPERATION (IDLE)
    // ========================================================================
    case STATE_IDLE: {
      bool anyButtonPressed = (recReading == LOW);

      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(buttons[i].pin) == LOW) {
          anyButtonPressed = true;
        }
        int action = checkButtonAction(buttons[i]);
        if (action == 1 || action == 2) {
          transmitButton(i, (action == 2));
        }
      }

      // Only enter Deep Sleep if completely idle AND no buttons are held down
      if (!anyButtonPressed && (now - lastActivityTime >= INACTIVITY_TIMEOUT)) {
        enterDeepSleep();
      }
      break;
    }

    // ========================================================================
    // STATE 1: LEARN MODE - SELECT TARGET BUTTON (SOLID RED LED)
    // ========================================================================
    case STATE_RECORD_SELECT_BTN: {
      lastActivityTime = now; // Never sleep during active mode

      if (now - stateStartTime >= MODE_TIMEOUT) {
        currentState = STATE_IDLE;
        Serial.println("[TIMEOUT]: Learn mode cancelled.");
        break;
      }

      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (checkButtonAction(buttons[i]) == 1) {
          recordTargetIndex = i;
          currentState = STATE_RECORD_WAIT_IR;
          stateStartTime = now;
          irrecv.resume();
          Serial.printf("[TARGET: %s]: Point original remote (TV / AC / Elevator) and press key...\n", buttons[i].name);
          break;
        }
      }
      break;
    }

    // ========================================================================
    // STATE 2: LEARN MODE - CAPTURE STANDARD OR RAW IR (SOLID RED LED)
    // ========================================================================
    case STATE_RECORD_WAIT_IR: {
      lastActivityTime = now; // Never sleep during active mode

      if (now - stateStartTime >= MODE_TIMEOUT) {
        currentState = STATE_IDLE;
        Serial.println("[TIMEOUT]: No IR signal received.");
        break;
      }

      if (irrecv.decode(&results)) {
        if (results.repeat) {
          irrecv.resume();
          break;
        }

        uint16_t rawLen = getCorrectedRawLength(&results);

        // Save as RAW if UNKNOWN, AC State, or long packet (>70 pulses)
        if (results.decode_type == UNKNOWN || hasACState(results.decode_type) || rawLen > 70) {
          uint16_t* rawPtr = resultToRawArray(&results);
          uint16_t copyLen = (rawLen > kMaxRawLen) ? kMaxRawLen : rawLen;

          buttons[recordTargetIndex].isRaw    = true;
          buttons[recordTargetIndex].rawLen   = copyLen;
          buttons[recordTargetIndex].protocol = results.decode_type;
          buttons[recordTargetIndex].code     = 0;
          memcpy(buttons[recordTargetIndex].rawData, rawPtr, copyLen * sizeof(uint16_t));

          delete[] rawPtr;
          Serial.printf("[CAPTURED RAW]: 0x");
          serialPrintUint64(results.value, HEX);
          Serial.printf(" saved for [%s]!\n", buttons[recordTargetIndex].name);
        } else {
          // Standard Decoded Protocol
          buttons[recordTargetIndex].isRaw    = false;
          buttons[recordTargetIndex].rawLen   = 0;
          buttons[recordTargetIndex].code     = results.value;
          buttons[recordTargetIndex].protocol = results.decode_type;
          buttons[recordTargetIndex].bits     = results.bits;
          Serial.printf("[CAPTURED]: %s (0x", typeToString(results.decode_type).c_str());
          serialPrintUint64(results.value, HEX);
          Serial.printf(") saved for [%s]!\n", buttons[recordTargetIndex].name);
        }

        saveButtonData(recordTargetIndex);
        irrecv.resume();

        currentState = STATE_IDLE;
        pulseGreenLED(800);
        lastActivityTime = now;
      }
      break;
    }

    // ========================================================================
    // STATE 3: BRUTE FORCE - SCANNING POWER CODES (BLINKING RED LED)
    // ========================================================================
    case STATE_AUTOSCAN_RUNNING: {
      lastActivityTime = now; // Never sleep during Brute Force scan

      if (waitingForRecRelease) {
        if (digitalRead(BTN_REC_PIN) == HIGH) {
          waitingForRecRelease = false;
        }
        break;
      }

      // Send next POWER code every 650ms
      if (now - lastScanShotTime >= SCAN_INTERVAL) {
        lastScanShotTime = now;
        scanDevIndex++;

        if (scanDevIndex >= TOTAL_DEVICES) {
          currentState = STATE_IDLE;
          Serial.println("[BRUTE FORCE]: Reached end of 50-device database.");
          break;
        }

        Serial.printf("[%d/%d] Testing POWER: %-22s (0x", scanDevIndex + 1, TOTAL_DEVICES, deviceDatabase[scanDevIndex].brand);
        serialPrintUint64(deviceDatabase[scanDevIndex].codes[4], HEX);
        Serial.println(")");

        pulseGreenLED(80);
        irsend.send(deviceDatabase[scanDevIndex].protocol,
                    deviceDatabase[scanDevIndex].codes[4],
                    deviceDatabase[scanDevIndex].bits);
      }

      // Evaluate user feedback during scan
      if (scanDevIndex >= 0) {
        // UP Button (Index 0) -> Device responded! Ask user which button to assign
        if (checkButtonAction(buttons[0]) == 1) {
          currentState = STATE_AUTOSCAN_SELECT_BTN;
          stateStartTime = now;
          Serial.printf("\n>>> MATCH FOUND: [%s] <<<\n", deviceDatabase[scanDevIndex].brand);
          Serial.println("Which button do you want to save this POWER code to?");
          Serial.println(" -> Press ANY target button (UP/DOWN/LEFT/RIGHT/EXTRA1/EXTRA2) to save to that button only.");
          Serial.println(" -> OR press [REC] to auto-map ALL 6 buttons of this device!");
        }
        // DOWN Button (Index 1) -> Skip collision & continue scanning
        else if (checkButtonAction(buttons[1]) == 1) {
          Serial.printf("[SKIPPED]: Ignoring [%s]. Continuing scan...\n", deviceDatabase[scanDevIndex].brand);
          lastScanShotTime = 0; // Fire next code immediately
        }
      }
      break;
    }

    // ========================================================================
    // STATE 4: BRUTE FORCE - ASSIGN MATCHED CODE TO CHOSEN BUTTON
    // ========================================================================
    case STATE_AUTOSCAN_SELECT_BTN: {
      lastActivityTime = now; // Never sleep while choosing button

      if (now - stateStartTime >= MODE_TIMEOUT) {
        currentState = STATE_IDLE;
        Serial.println("[TIMEOUT]: Brute force assignment cancelled.");
        break;
      }

      // Wait for user to pick which button gets the matched POWER code
      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (checkButtonAction(buttons[i]) == 1) {
          buttons[i].isRaw    = false;
          buttons[i].rawLen   = 0;
          buttons[i].code     = deviceDatabase[scanDevIndex].codes[4]; // Save matched POWER code
          buttons[i].protocol = deviceDatabase[scanDevIndex].protocol;
          buttons[i].bits     = deviceDatabase[scanDevIndex].bits;

          saveButtonData(i);
          Serial.printf(">>> SAVED [%s] POWER code to button [%s]! <<<\n",
                        deviceDatabase[scanDevIndex].brand, buttons[i].name);

          currentState = STATE_IDLE;
          pulseGreenLED(1000);
          lastActivityTime = now;
          break;
        }
      }
      break;
    }
  }
}