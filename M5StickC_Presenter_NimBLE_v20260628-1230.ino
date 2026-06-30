/*
 * M5StickC Presentation Controller — raw NimBLE HID keyboard
 * Version: v20260628-1230
 *
 * Hardware: M5StickC (original — ESP32-PICO-D4, AXP192 PMIC)
 *
 * Button -> Key mapping:
 *   M5 (BtnA / GPIO 37) -> RIGHT ARROW  (next slide)
 *   G39  (BtnB / GPIO 39) -> LEFT ARROW   (previous slide)
 *   Power (AXP192 PEK)   -> ESCAPE        (end slideshow)
 *
 * Dependencies (BOTH available in Arduino Library Manager — no ZIP install):
 *   - M5StickC       by M5Stack
 *   - NimBLE-Arduino by h2zero      (search "NimBLE")
 *
 * >>> VERSION NOTE <<<
 * This sketch targets NimBLE-Arduino 2.x (the current Library Manager release).
 * If you are on the older 1.x line, the NimBLEHIDDevice method names differ
 * (e.g. setManufacturer -> manufacturer, getInputReport -> inputReport,
 *  setReportMap -> reportMap, setPnp -> pnp, setHidInfo -> hidInfo, and the
 *  callback signatures drop the NimBLEConnInfo& argument). 1.x equivalents are
 *  noted in comments where they matter.
 *
 * Flash settings (Tools menu):
 *   Board            : M5Stick-C
 *   Partition Scheme : Huge APP (3MB No OTA)   <-- avoids flash overflow
 *   Upload Speed     : 1500000
 */

#include <M5StickC.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

// ── HID usage codes (USB HID Usage Tables, Keyboard/Keypad page 0x07) ─────────
static const uint8_t KEY_RIGHT_ARROW = 0x4F;
static const uint8_t KEY_LEFT_ARROW  = 0x50;
static const uint8_t KEY_ESCAPE      = 0x29;

// ── HID report descriptor: standard 101-key boot keyboard, Report ID 1 ────────
// Report layout (8 bytes): [modifiers][reserved][key1..key6]
static const uint8_t HID_REPORT_MAP[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
  0x19, 0xE0,        //   Usage Minimum (Left Control)
  0x29, 0xE7,        //   Usage Maximum (Right GUI)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data,Var,Abs)  ; modifier byte
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8)
  0x81, 0x01,        //   Input (Const)         ; reserved byte
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x65,        //   Logical Maximum (101)
  0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
  0x19, 0x00,        //   Usage Minimum (0)
  0x29, 0x65,        //   Usage Maximum (101)
  0x81, 0x00,        //   Input (Data,Array)    ; 6 key slots
  0xC0               // End Collection
};

// ── BLE objects ───────────────────────────────────────────────────────────────
NimBLEServer*       server   = nullptr;
NimBLEHIDDevice*    hid      = nullptr;
NimBLECharacteristic* input  = nullptr;   // input report characteristic
volatile bool       connected = false;

// ── Debounce ──────────────────────────────────────────────────────────────────
static const uint32_t KEY_DEBOUNCE_MS = 120;
uint32_t lastA = 0, lastB = 0, lastPwr = 0;
bool prevA = false, prevB = false, prevPwr = false;

// ── Display helpers ────────────────────────────────────────────────────────────
void drawStatus(const char* line1, const char* line2 = "") {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(4, 10);
  M5.Lcd.print(line1);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.setCursor(4, 42);
  M5.Lcd.print(line2);

  float bat = M5.Axp.GetBatVoltage();
  int pct = constrain((int)((bat - 3.2f) / (4.2f - 3.2f) * 100), 0, 100);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setCursor(4, 56);
  M5.Lcd.printf("BAT %d%%  %.2fV", pct, bat);
}

void flashKey(const char* label) {
  M5.Lcd.fillScreen(TFT_NAVY);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, TFT_NAVY);
  M5.Lcd.setCursor(8, 24);
  M5.Lcd.print(label);
  delay(80);
  drawStatus("CONNECTED", label);
}

// ── Server connection callbacks (2.x signatures) ──────────────────────────────
class SrvCallbacks : public NimBLEServerCallbacks {
  // 1.x: void onConnect(NimBLEServer* s)
  void onConnect(NimBLEServer* s, NimBLEConnInfo& connInfo) override {
    connected = true;
    Serial.println("[BLE] Host connected");
  }
  // 1.x: void onDisconnect(NimBLEServer* s)
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& connInfo, int reason) override {
    connected = false;
    Serial.printf("[BLE] Host disconnected (reason %d), re-advertising\n", reason);
    NimBLEDevice::startAdvertising();
  }
};

// ── Send a single keypress then release ───────────────────────────────────────
void sendKey(uint8_t keycode) {
  if (!connected || input == nullptr) return;

  digitalWrite(GPIO_NUM_10, LOW);      // LED on (LOW = on) — brief press feedback

  uint8_t report[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  report[2] = keycode;                 // first key slot
  input->setValue(report, sizeof(report));
  input->notify();

  delay(15);

  uint8_t release[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  input->setValue(release, sizeof(release));
  input->notify();

  digitalWrite(GPIO_NUM_10, HIGH);     // LED off again
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Axp.ScreenBreath(9);
  pinMode(GPIO_NUM_10, OUTPUT);        // built-in red LED
  digitalWrite(GPIO_NUM_10, HIGH);     // LED off (HIGH = off)
  drawStatus("BLE starting...");

  Serial.begin(115200);
  Serial.println("[M5 Presenter] NimBLE HID starting");

  NimBLEDevice::init("M5 Presenter");

  // Just-works bonding so the host reconnects automatically next time.
  // args: bonding, MITM, secure-connections
  NimBLEDevice::setSecurityAuth(true, false, true);

  server = NimBLEDevice::createServer();
  server->setCallbacks(new SrvCallbacks());

  hid = new NimBLEHIDDevice(server);

  // --- Device identity ---
  // 1.x equivalents: hid->manufacturer()->setValue("M5Stack");
  hid->setManufacturer("M5Stack");
  // sig=0x02 (USB), vendorId, productId, version
  hid->setPnp(0x02, 0x05AC, 0x820A, 0x0100);
  // country=0x00, flags: 0x01 = remote wake
  hid->setHidInfo(0x00, 0x01);

  // --- Report map ---
  // 1.x: hid->reportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
  hid->setReportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));

  // --- Input report, Report ID 1 ---
  // 1.x: input = hid->inputReport(1);
  input = hid->getInputReport(1);

  hid->setBatteryLevel(100);
  hid->startServices();

  // --- Advertising ---
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);          // HID Keyboard appearance
  adv->addServiceUUID(hid->getHidService()->getUUID());
  adv->enableScanResponse(true);       // 1.x: adv->setScanResponse(true);
  NimBLEDevice::startAdvertising();

  drawStatus("WAITING...", "Pair on host");
  Serial.println("[BLE] Advertising as 'M5 Presenter'");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  M5.update();
  uint32_t now = millis();

  // Connection-status display transitions
  static bool wasConnected = false;
  if (connected != wasConnected) {
    wasConnected = connected;
    drawStatus(connected ? "CONNECTED" : "WAITING...",
               connected ? "Pair OK"   : "Pair on host");
  }

  if (!connected) {
    delay(10);
    return;
  }

  // BtnA (GPIO 37) -> RIGHT
  bool curA = M5.BtnA.isPressed();
  if (curA && !prevA && (now - lastA > KEY_DEBOUNCE_MS)) {
    lastA = now;
    sendKey(KEY_RIGHT_ARROW);
    Serial.println("[KEY] RIGHT");
    flashKey(">> NEXT");
  }
  prevA = curA;

  // BtnB (GPIO 39) -> LEFT
  bool curB = M5.BtnB.isPressed();
  if (curB && !prevB && (now - lastB > KEY_DEBOUNCE_MS)) {
    lastB = now;
    sendKey(KEY_LEFT_ARROW);
    Serial.println("[KEY] LEFT");
    flashKey("<< BACK");
  }
  prevB = curB;

  // Power button (AXP192 PEK short press == 2) -> ESC
  bool curPwr = (M5.Axp.GetBtnPress() == 2);
  if (curPwr && !prevPwr && (now - lastPwr > KEY_DEBOUNCE_MS)) {
    lastPwr = now;
    sendKey(KEY_ESCAPE);
    Serial.println("[KEY] ESC");
    flashKey("ESC");
  }
  prevPwr = curPwr;

  delay(10);
}
