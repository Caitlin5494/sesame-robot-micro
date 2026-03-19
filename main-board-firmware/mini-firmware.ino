#include <Wire.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "face-bitmaps.h"
#include "movement-sequences.h"

#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT  32
#define OLED_RESET     -1
#define OLED_I2C_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global state
char currentCommand[16] = "";
char currentFaceName[16] = "default";
const unsigned char* const* currentFaceFrames = nullptr;
uint8_t  currentFaceFrameCount = 0;
uint8_t  currentFaceFrameIndex = 0;
unsigned long lastFaceFrameMs  = 0;
FaceAnimMode  currentFaceMode  = FACE_ANIM_LOOP;
int8_t   faceFrameDirection   = 1;
bool     faceAnimFinished     = false;
uint8_t  currentFaceFps       = 1;
bool     idleActive           = false;
bool     idleBlinkActive      = false;
unsigned long nextIdleBlinkMs = 0;
uint8_t  idleBlinkRepeatsLeft = 0;

Servo servos[8];
const int servoPins[8] = {8, 9, 2, 3, 4, 5, 6, 7};
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

SoftwareSerial wifiSerial(12, 255); // RX=pin 12 from ESP32-C6 D3, TX unused

int frameDelay       = 100;
int walkCycles       = 10;
int motorCurrentDelay = 55;
bool stopRequested   = false;

struct FaceEntry {
  const char* name;
  const unsigned char* const* frames;
  uint8_t maxFrames;
};

static const uint8_t MAX_FACE_FRAMES = 6;

#define MAKE_FACE_FRAMES(name) \
  const unsigned char* const face_##name##_frames[] = { \
    epd_bitmap_##name, epd_bitmap_##name##_1, epd_bitmap_##name##_2, \
    epd_bitmap_##name##_3, epd_bitmap_##name##_4, epd_bitmap_##name##_5 \
  };
#define X(name) MAKE_FACE_FRAMES(name)
FACE_LIST
#undef X
#undef MAKE_FACE_FRAMES

const FaceEntry faceEntries[] = {
#define X(name) { #name, face_##name##_frames, MAX_FACE_FRAMES },
  FACE_LIST
#undef X
  { "default", face_defualt_frames, MAX_FACE_FRAMES }
};

// Prototypes
void    setServoAngle(uint8_t channel, int angle);
void    updateFaceBitmap(const unsigned char* bitmap);
void    setFace(const char* faceName);
void    setFaceMode(FaceAnimMode mode);
void    setFaceWithMode(const char* faceName, FaceAnimMode mode);
void    updateAnimatedFace();
void    delayWithFace(unsigned long ms);
void    enterIdle();
void    exitIdle();
void    updateIdleBlink();
uint8_t getFaceFpsForName(const char* faceName);
bool    pressingCheck(const char* cmd, int ms);
void    pollWifiSerial();
void    processLine(const char* buf);

void setup() {
  Serial.begin(115200);
  wifiSerial.begin(9600);
  randomSeed(micros());
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("OLED fail"));
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Ready"));
  display.display();

  // Servos are attached lazily on first command — no movement on startup
  setFace("rest");
}

void loop() {
  updateAnimatedFace();
  updateIdleBlink();

  if (currentCommand[0] != '\0') {
    const char* cmd = currentCommand;
    if      (strcmp(cmd, "forward")  == 0) { runWalkPose();     if (strcmp(currentCommand, "forward")  == 0) currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "backward") == 0) { runWalkBackward(); if (strcmp(currentCommand, "backward") == 0) currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "left")     == 0) { runTurnLeft();     if (strcmp(currentCommand, "left")     == 0) currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "right")    == 0) { runTurnRight();    if (strcmp(currentCommand, "right")    == 0) currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "rest")     == 0) { runRestPose();   currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "stand")    == 0) { runStandPose(1); currentCommand[0] = '\0'; }
    else if (strcmp(cmd, "wave")     == 0) runWavePose();
    else if (strcmp(cmd, "dance")    == 0) runDancePose();
    else if (strcmp(cmd, "swim")     == 0) runSwimPose();
    else if (strcmp(cmd, "point")    == 0) runPointPose();
    else if (strcmp(cmd, "pushup")   == 0) runPushupPose();
    else if (strcmp(cmd, "bow")      == 0) runBowPose();
    else if (strcmp(cmd, "cute")     == 0) runCutePose();
    else if (strcmp(cmd, "freaky")   == 0) runFreakyPose();
    else if (strcmp(cmd, "worm")     == 0) runWormPose();
    else if (strcmp(cmd, "shake")    == 0) runShakePose();
    else if (strcmp(cmd, "shrug")    == 0) runShrugPose();
    else if (strcmp(cmd, "dead")     == 0) runDeadPose();
    else if (strcmp(cmd, "crab")     == 0) runCrabPose();
  }

  // USB serial CLI
  {
    static char buf[32];
    static byte pos = 0;
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (pos > 0) { buf[pos] = '\0'; pos = 0; processLine(buf); }
      } else if (pos < sizeof(buf) - 1) { buf[pos++] = c; }
    }
  }
  pollWifiSerial();
}

void updateFaceBitmap(const unsigned char* bitmap) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmap, 64, 32, SSD1306_WHITE);
  display.display();
}

uint8_t countFrames(const unsigned char* const* frames, uint8_t maxFrames) {
  if (!frames || !frames[0]) return 0;
  uint8_t n = 0;
  while (n < maxFrames && frames[n]) n++;
  return n;
}

void setFace(const char* faceName) {
  if (strcmp(faceName, currentFaceName) == 0 && currentFaceFrames) return;
  strncpy(currentFaceName, faceName, 15); currentFaceName[15] = '\0';
  currentFaceFrameIndex = 0;
  lastFaceFrameMs = 0;
  faceFrameDirection = 1;
  faceAnimFinished = false;
  currentFaceFps = getFaceFpsForName(faceName);

  currentFaceFrames = face_defualt_frames;
  currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);
  for (size_t i = 0; i < sizeof(faceEntries) / sizeof(faceEntries[0]); i++) {
    if (strcmp(faceName, faceEntries[i].name) == 0) {
      currentFaceFrames = faceEntries[i].frames;
      currentFaceFrameCount = countFrames(faceEntries[i].frames, faceEntries[i].maxFrames);
      break;
    }
  }
  if (currentFaceFrameCount == 0) {
    currentFaceFrames = face_defualt_frames;
    currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);
    strcpy(currentFaceName, "default");
    currentFaceFps = 1;
  }
  if (currentFaceFrameCount > 0 && currentFaceFrames[0])
    updateFaceBitmap(currentFaceFrames[0]);
}

void setFaceMode(FaceAnimMode mode) {
  currentFaceMode = mode; faceFrameDirection = 1; faceAnimFinished = false;
}

void setFaceWithMode(const char* faceName, FaceAnimMode mode) {
  setFaceMode(mode); setFace(faceName);
}

uint8_t getFaceFpsForName(const char* name) {
  if (strcmp(name, "idle_blink") == 0) return 7;
  if (strcmp(name, "point")      == 0) return 5;
  if (strcmp(name, "dead")       == 0) return 2;
  return 1;
}

void updateAnimatedFace() {
  if (!currentFaceFrames || currentFaceFrameCount <= 1) return;
  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) return;
  unsigned long now = millis();
  if (now - lastFaceFrameMs < 1000UL / max((uint8_t)1, currentFaceFps)) return;
  lastFaceFrameMs = now;

  if (currentFaceMode == FACE_ANIM_LOOP) {
    currentFaceFrameIndex = (currentFaceFrameIndex + 1) % currentFaceFrameCount;
  } else if (currentFaceMode == FACE_ANIM_ONCE) {
    if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) {
      currentFaceFrameIndex = currentFaceFrameCount - 1;
      faceAnimFinished = true;
    } else { currentFaceFrameIndex++; }
  } else { // BOOMERANG
    if (faceFrameDirection > 0) {
      if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) {
        faceFrameDirection = -1;
        if (currentFaceFrameIndex > 0) currentFaceFrameIndex--;
      } else { currentFaceFrameIndex++; }
    } else {
      if (currentFaceFrameIndex == 0) {
        faceFrameDirection = 1;
        if (currentFaceFrameCount > 1) currentFaceFrameIndex++;
      } else { currentFaceFrameIndex--; }
    }
  }
  updateFaceBitmap(currentFaceFrames[currentFaceFrameIndex]);
}

void delayWithFace(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    pollWifiSerial();
    updateAnimatedFace();
    delay(5);
  }
}

void scheduleNextIdleBlink(unsigned long minMs, unsigned long maxMs) {
  nextIdleBlinkMs = millis() + (unsigned long)random(minMs, maxMs);
}

void enterIdle() {
  idleActive = true; idleBlinkActive = false; idleBlinkRepeatsLeft = 0;
  setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
  scheduleNextIdleBlink(3000, 7000);
}

void exitIdle() { idleActive = false; idleBlinkActive = false; }

void updateIdleBlink() {
  if (!idleActive) return;
  if (!idleBlinkActive) {
    if (millis() < nextIdleBlinkMs) return;
    idleBlinkActive = true;
    if (idleBlinkRepeatsLeft == 0 && random(100) < 30) idleBlinkRepeatsLeft = 1;
    setFaceWithMode("idle_blink", FACE_ANIM_ONCE);
    return;
  }
  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) {
    idleBlinkActive = false;
    setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
    if (idleBlinkRepeatsLeft > 0) { idleBlinkRepeatsLeft--; scheduleNextIdleBlink(120, 220); }
    else scheduleNextIdleBlink(3000, 7000);
  }
}

void setServoAngle(uint8_t channel, int angle) {
  if (stopRequested) return;
  if (channel < 8) {
    if (!servos[channel].attached()) servos[channel].attach(servoPins[channel]);
    servos[channel].write(constrain(angle + servoSubtrim[channel], 0, 180));
    delayWithFace(motorCurrentDelay);
  }
}

void pollWifiSerial() {
  static char wbuf[32];
  static byte wpos = 0;
  while (wifiSerial.available()) {
    char c = wifiSerial.read();
    if (c == '\n' || c == '\r') {
      if (wpos > 0) { wbuf[wpos] = '\0'; wpos = 0; processLine(wbuf); }
    } else if (wpos < sizeof(wbuf) - 1) { wbuf[wpos++] = c; }
  }
}

bool pressingCheck(const char* cmd, int ms) {
  if (stopRequested) {
    stopRequested = false;
    runStandPose(1);
    return false;
  }

  unsigned long start = millis();
  while (millis() - start < (unsigned long)ms) {
    pollWifiSerial();
    updateAnimatedFace();
    if (stopRequested || strcmp(currentCommand, cmd) != 0) {
      stopRequested = false;
      runStandPose(1);
      return false;
    }
  }
  return true;
}

void processLine(const char* buf) {
  int m, a;
  if (strcmp(buf, "stop") == 0) {
    stopRequested = true;
    currentCommand[0] = '\0';
  } else if (strcmp(buf, "subtrim") == 0) {
    for (int i = 0; i < 8; i++) {
      Serial.print(i); Serial.print(F(": "));
      if (servoSubtrim[i] >= 0) Serial.print('+');
      Serial.println(servoSubtrim[i]);
    }
  } else if (strcmp(buf, "subtrim save") == 0) {
    Serial.print(F("int8_t servoSubtrim[8]={"));
    for (int i = 0; i < 8; i++) { Serial.print(servoSubtrim[i]); if (i < 7) Serial.print(','); }
    Serial.println(F("};"));
  } else if (strncmp(buf, "subtrim reset", 13) == 0) {
    for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
  } else if (strncmp(buf, "subtrim ", 8) == 0) {
    if (sscanf(buf + 8, "%d %d", &m, &a) == 2 && m >= 0 && m < 8 && a >= -90 && a <= 90)
      servoSubtrim[m] = (int8_t)a;
  } else if (strncmp(buf, "frameDelay ", 11) == 0) {
    if (sscanf(buf + 11, "%d", &a) == 1 && a >= 1 && a <= 1000)
      frameDelay = a;
  } else if (strncmp(buf, "walkCycles ", 11) == 0) {
    if (sscanf(buf + 11, "%d", &a) == 1 && a >= 1 && a <= 50)
      walkCycles = a;
  } else if (strncmp(buf, "motorDelay ", 11) == 0) {
    if (sscanf(buf + 11, "%d", &a) == 1 && a >= 0 && a <= 500)
      motorCurrentDelay = a;
  } else if (strncmp(buf, "all ", 4) == 0) {
    if (sscanf(buf + 4, "%d", &a) == 1)
      for (int i = 0; i < 8; i++) setServoAngle(i, a);
  } else if (sscanf(buf, "%d %d", &m, &a) == 2 && m >= 0 && m < 8) {
    setServoAngle(m, a);
  } else {
    stopRequested = false;
    strncpy(currentCommand, buf, 15);
    currentCommand[15] = '\0';
    exitIdle();
  }
}