// =============================================================================
// wifi-bridge.ino  —  XIAO ESP32-C6 Wi-Fi bridge for mini robot
// =============================================================================
//
// This sketch turns the ESP32-C6 into a captive-portal access point. Any phone
// or laptop that connects to the AP is automatically served the robot controller
// UI. Commands are forwarded to the main robot board over UART1.
//
// WIRING
// ──────
//   ESP32-C6 GND    ←→ Main board GND       (required)
//   ESP32-C6 D3     ──→ Main board pin 12    (TX → SoftwareSerial RX)
//
//   One-way link — the ESP32 only sends commands, the main board does not
//   reply.  No voltage divider is needed: a 3.3 V high is above the
//   5 V AVR's Vil threshold, so the line is safe as-is.
//
// CAPTIVE-PORTAL HEADER
// ─────────────────────
//   Copy  captive-portal.h  from the mini-firmware folder into this folder
//   (wifi-bridge/) before compiling. It only contains the HTML string so it
//   compiles unchanged on ESP32.
//
// ARDUINO IDE SETUP
// ─────────────────
//   Board:  XIAO_ESP32C6  (Seeed Studio XIAO ESP32-C6)
//   Core :  esp32 by Espressif  (v3.x)
// =============================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#if defined(__has_include)
#if __has_include(<Bluepad32.h>)
#include <Bluepad32.h>
#define HAVE_BLUEPAD32 1
#else
#define HAVE_BLUEPAD32 0
#endif
#else
#define HAVE_BLUEPAD32 0
#endif
#include "captive-portal.h"

// ---------------------------------------------------------------------------
// Configuration — edit here
// ---------------------------------------------------------------------------
static const char AP_SSID[]  = "Sesame";   // network name shown on devices
static const char AP_PASS[]  = "";         // empty = open network (easiest for
                                           // captive portal auto-popup)
static const uint8_t AP_CHANNEL = 6;
static const uint8_t DNS_PORT   = 53;
static const uint32_t MOTION_REFRESH_MS = 140;
static const uint32_t WEB_MOTION_TIMEOUT_MS = 1400;
static const uint32_t BT_MOTION_TIMEOUT_MS  = 450;
static const int BT_AXIS_DEADBAND = 220;

// Single TX wire on XIAO ESP32-C6
static const int UART_RX = -1;   // not connected — one-way link
static const int UART_TX = 21;   // D3 — sends commands to main board pin 12

// ---------------------------------------------------------------------------
// Mirrored settings (returned to the UI via /getSettings)
// Updated locally and sent to the main board when changed via /setSettings
// ---------------------------------------------------------------------------
static int    cfg_frameDelay        = 100;
static int    cfg_walkCycles        = 10;
static int    cfg_motorCurrentDelay = 55;
static String cfg_motorSpeed        = "medium";

enum MotionSource : uint8_t {
  MOTION_NONE = 0,
  MOTION_WEB  = 1,
  MOTION_BT   = 2
};

static String      activeMotionCmd;
static MotionSource motionSource      = MOTION_NONE;
static uint32_t    motionLastTxMs     = 0;
static uint32_t    motionLastInputMs  = 0;
static uint8_t     btConnectedCount   = 0;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
DNSServer dns;
WebServer server(80);

#if HAVE_BLUEPAD32
struct PadEdgeState {
  bool a;
  bool b;
  bool x;
  bool y;
  bool l1;
  bool r1;
  bool thumbL;
  bool thumbR;
};

ControllerPtr btPads[BP32_MAX_GAMEPADS];
PadEdgeState  btEdges[BP32_MAX_GAMEPADS];
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void sendMain(const String& line) {
  Serial1.println(line);
}

static bool isValidDir(const String& s) {
  return s == "forward" || s == "backward" || s == "left" || s == "right";
}

static bool isValidPose(const String& s) {
  static const char* const poses[] = {
    "rest","stand","wave","dance","swim","point","pushup",
    "bow","cute","freaky","worm","shake","shrug","dead","crab", nullptr
  };
  for (int i = 0; poses[i]; i++)
    if (s == poses[i]) return true;
  return false;
}

static void stopMotion() {
  if (activeMotionCmd.length() > 0) {
    sendMain("stop");
  }
  activeMotionCmd  = "";
  motionSource     = MOTION_NONE;
  motionLastTxMs   = 0;
  motionLastInputMs = 0;
}

static void refreshMotion(const char* dir, MotionSource source) {
  if (!dir) return;

  const uint32_t now = millis();
  const bool changedDir = (activeMotionCmd != dir) || (motionSource != source);
  if (changedDir || (now - motionLastTxMs >= MOTION_REFRESH_MS)) {
    sendMain(String(dir));
    motionLastTxMs = now;
  }

  activeMotionCmd  = dir;
  motionSource     = source;
  motionLastInputMs = now;
}

static void serviceMotionWatchdog() {
  if (activeMotionCmd.length() == 0) return;

  const uint32_t now = millis();
  const uint32_t inputTimeoutMs =
      (motionSource == MOTION_WEB) ? WEB_MOTION_TIMEOUT_MS : BT_MOTION_TIMEOUT_MS;

  if (now - motionLastInputMs > inputTimeoutMs) {
    stopMotion();
    return;
  }

  if (now - motionLastTxMs >= MOTION_REFRESH_MS) {
    sendMain(activeMotionCmd);
    motionLastTxMs = now;
  }
}

static void sendPoseCommand(const char* pose) {
  if (!pose) return;
  stopMotion();
  sendMain(String(pose));
}

// Build the JSON settings response without heap-allocating a large object
static String buildSettingsJson() {
  String j;
  j.reserve(80);
  j  = F("{\"frameDelay\":");
  j += cfg_frameDelay;
  j += F(",\"walkCycles\":");
  j += cfg_walkCycles;
  j += F(",\"motorCurrentDelay\":");
  j += cfg_motorCurrentDelay;
  j += F(",\"motorSpeed\":\"");
  j += cfg_motorSpeed;
  j += F("\"}");
  return j;
}

static String buildBtStatusJson() {
  String j;
  j.reserve(120);
  j  = F("{\"supported\":");
  j += HAVE_BLUEPAD32 ? F("true") : F("false");
  j += F(",\"connected\":");
  j += btConnectedCount;
  j += F(",\"motion\":\"");
  j += activeMotionCmd;
  j += F("\"}");
  return j;
}

#if HAVE_BLUEPAD32
static int findPadSlot(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    if (btPads[i] == ctl) return i;
  }
  return -1;
}

static int findFreePadSlot() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    if (btPads[i] == nullptr) return i;
  }
  return -1;
}

void onBtConnected(ControllerPtr ctl) {
  int slot = findPadSlot(ctl);
  if (slot < 0) slot = findFreePadSlot();
  if (slot < 0) {
    Serial.println(F("[BT] Controller connected but no free slot"));
    return;
  }

  btPads[slot] = ctl;
  btEdges[slot] = {};
  btConnectedCount++;
  Serial.print(F("[BT] Controller connected in slot "));
  Serial.println(slot);
}

void onBtDisconnected(ControllerPtr ctl) {
  int slot = findPadSlot(ctl);
  if (slot < 0) return;

  btPads[slot] = nullptr;
  btEdges[slot] = {};
  if (btConnectedCount > 0) btConnectedCount--;

  if (motionSource == MOTION_BT) {
    stopMotion();
  }

  Serial.print(F("[BT] Controller disconnected from slot "));
  Serial.println(slot);
}

static const char* directionFromController(const ControllerPtr& ctl) {
  const uint8_t dpad = ctl->dpad();
  if (dpad & DPAD_UP)    return "forward";
  if (dpad & DPAD_DOWN)  return "backward";
  if (dpad & DPAD_LEFT)  return "left";
  if (dpad & DPAD_RIGHT) return "right";

  const int ax = ctl->axisX();
  const int ay = ctl->axisY();
  if (abs(ax) < BT_AXIS_DEADBAND && abs(ay) < BT_AXIS_DEADBAND) {
    return nullptr;
  }

  if (abs(ay) >= abs(ax)) {
    return (ay < 0) ? "forward" : "backward";
  }
  return (ax < 0) ? "left" : "right";
}

static void handleBtButtons(int slot, ControllerPtr ctl) {
  PadEdgeState& e = btEdges[slot];

  const bool a = ctl->a();
  const bool b = ctl->b();
  const bool x = ctl->x();
  const bool y = ctl->y();
  const bool l1 = ctl->l1();
  const bool r1 = ctl->r1();
  const bool thumbL = ctl->thumbL();
  const bool thumbR = ctl->thumbR();

  if (b && !e.b) {
    stopMotion();
  }
  if (a && !e.a) {
    sendPoseCommand("wave");
  }
  if (x && !e.x) {
    sendPoseCommand("dance");
  }
  if (y && !e.y) {
    sendPoseCommand("stand");
  }
  if (l1 && !e.l1) {
    sendPoseCommand("rest");
  }
  if (r1 && !e.r1) {
    sendPoseCommand("cute");
  }
  if (thumbL && !e.thumbL) {
    sendPoseCommand("dead");
  }
  if (thumbR && !e.thumbR) {
    sendPoseCommand("crab");
  }

  e.a = a;
  e.b = b;
  e.x = x;
  e.y = y;
  e.l1 = l1;
  e.r1 = r1;
  e.thumbL = thumbL;
  e.thumbR = thumbR;
}

static void serviceBluetoothControllers() {
  BP32.update();

  for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
    ControllerPtr ctl = btPads[i];
    if (!ctl || !ctl->isConnected() || !ctl->isGamepad()) continue;

    if (ctl->hasData()) {
      const char* dir = directionFromController(ctl);
      if (dir) {
        refreshMotion(dir, MOTION_BT);
      } else if (motionSource == MOTION_BT) {
        stopMotion();
      }

      handleBtButtons(i, ctl);
    }
  }
}
#endif

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
void handleRoot() {
  // index_html is declared PROGMEM in captive-portal.h; on ESP32 PROGMEM is
  // a no-op so we can pass it directly as a const char*.
  server.send_P(200, "text/html", index_html);
}

void handleCmd() {
  if (server.hasArg("stop")) {
    stopMotion();

  } else if (server.hasArg("go")) {
    String dir = server.arg("go");
    if (isValidDir(dir)) refreshMotion(dir.c_str(), MOTION_WEB);

  } else if (server.hasArg("pose")) {
    String pose = server.arg("pose");
    if (isValidPose(pose)) sendPoseCommand(pose.c_str());

  } else if (server.hasArg("motor") && server.hasArg("value")) {
    // UI sends motor numbers 1-8; firmware expects 0-7
    int ch  = server.arg("motor").toInt() - 1;
    int val = server.arg("value").toInt();
    if (ch >= 0 && ch < 8 && val >= 0 && val <= 180) {
      stopMotion();
      sendMain(String(ch) + " " + String(val));
    }
  }

  server.send(200, "text/plain", "OK");
}

void handleGetSettings() {
  server.send(200, "application/json", buildSettingsJson());
}

void handleSetSettings() {
  if (server.hasArg("frameDelay")) {
    int v = server.arg("frameDelay").toInt();
    if (v >= 1 && v <= 1000) {
      cfg_frameDelay = v;
      sendMain("frameDelay " + String(v));
    }
  }
  if (server.hasArg("walkCycles")) {
    int v = server.arg("walkCycles").toInt();
    if (v >= 1 && v <= 50) {
      cfg_walkCycles = v;
      sendMain("walkCycles " + String(v));
    }
  }
  if (server.hasArg("motorCurrentDelay")) {
    int v = server.arg("motorCurrentDelay").toInt();
    if (v >= 0 && v <= 500) {
      cfg_motorCurrentDelay = v;
      cfg_motorSpeed = "medium";          // numeric input overrides preset
      sendMain("motorDelay " + String(v));
    }
  }
  // motorSpeed preset — maps to a motorDelay value on the main board
  if (server.hasArg("motorSpeed")) {
    String s = server.arg("motorSpeed");
    if (s == "slow" || s == "medium" || s == "fast") {
      cfg_motorSpeed = s;
      int d = (s == "slow") ? 80 : (s == "fast") ? 25 : 55;
      cfg_motorCurrentDelay = d;
      sendMain("motorDelay " + String(d));
    }
  }

  server.send(200, "text/plain", "OK");
}

void handleBtStatus() {
  server.send(200, "application/json", buildBtStatusJson());
}

void handleBtForget() {
#if HAVE_BLUEPAD32
  stopMotion();
  BP32.forgetBluetoothKeys();
  server.send(200, "application/json", buildBtStatusJson());
#else
  server.send(501, "text/plain", "Bluepad32 not installed");
#endif
}

// Captive-portal redirect: sends any unrecognised request back to the root
void handleCaptiveRedirect() {
  String url = "http://";
  url += WiFi.softAPIP().toString();
  url += "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, UART_RX, UART_TX);

#if HAVE_BLUEPAD32
  BP32.setup(&onBtConnected, &onBtDisconnected);
  Serial.println(F("[BT] Bluepad32 ready - pair a BLE gamepad now"));
#else
  Serial.println(F("[BT] Bluepad32 host stack not found for this build. Install ESP32 + Bluepad32 board package (not NINA-only library)."));
#endif

  // Start Wi-Fi access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, strlen(AP_PASS) > 0 ? AP_PASS : nullptr, AP_CHANNEL);
  delay(200);  // let the AP fully start before we read the IP

  IPAddress ip = WiFi.softAPIP();
  Serial.print(F("[WiFi] AP:  ")); Serial.println(AP_SSID);
  Serial.print(F("[WiFi] IP:  ")); Serial.println(ip);

  // DNS: resolve every domain to our IP so the captive-portal popup fires
  dns.start(DNS_PORT, "*", ip);

  // Robot control routes
  server.on("/",            HTTP_GET, handleRoot);
  server.on("/cmd",         HTTP_GET, handleCmd);
  server.on("/getSettings", HTTP_GET, handleGetSettings);
  server.on("/setSettings", HTTP_GET, handleSetSettings);
  server.on("/btStatus",    HTTP_GET, handleBtStatus);
  server.on("/btForget",    HTTP_GET, handleBtForget);

  // Captive-portal detection endpoints (OS-specific probes)
  server.on("/generate_204",              HTTP_GET, handleCaptiveRedirect);  // Android Chrome
  server.on("/gen_204",                   HTTP_GET, handleCaptiveRedirect);
  server.on("/hotspot-detect.html",       HTTP_GET, handleRoot);             // Apple
  server.on("/library/test/success.html", HTTP_GET, handleRoot);             // Apple iOS 14+
  server.on("/connecttest.txt",           HTTP_GET, handleCaptiveRedirect);  // Windows
  server.on("/ncsi.txt",                  HTTP_GET, handleCaptiveRedirect);
  server.on("/success.txt",               HTTP_GET, handleCaptiveRedirect);  // Firefox
  server.onNotFound(handleCaptiveRedirect);

  server.begin();
  Serial.println(F("[HTTP] Server started"));
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
  dns.processNextRequest();
  server.handleClient();
  serviceMotionWatchdog();

#if HAVE_BLUEPAD32
  serviceBluetoothControllers();
#endif
}
