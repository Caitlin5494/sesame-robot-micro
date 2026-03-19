#pragma once

#include <Arduino.h>

enum FaceAnimMode : uint8_t {
  FACE_ANIM_LOOP = 0,
  FACE_ANIM_ONCE = 1,
  FACE_ANIM_BOOMERANG = 2
};

// External globals and helpers used by movement/pose sequences
extern int frameDelay;
extern int walkCycles;
extern char currentCommand[];

extern void setServoAngle(uint8_t channel, int angle);
extern void setFace(const char* faceName);
extern void setFaceMode(FaceAnimMode mode);
extern void setFaceWithMode(const char* faceName, FaceAnimMode mode);
extern void delayWithFace(unsigned long ms);
extern void enterIdle();
extern bool pressingCheck(const char* cmd, int ms);

// Pose/animation prototypes
void runRestPose();
void runStandPose(int face = 1);
void runWavePose();
void runDancePose();
void runSwimPose();
void runPointPose();
void runPushupPose();
void runBowPose();
void runCutePose();
void runFreakyPose();
void runWormPose();
void runShakePose();
void runShrugPose();
void runDeadPose();
void runCrabPose();
void runWalkPose();
void runWalkBackward();
void runTurnLeft();
void runTurnRight();

// ====== POSES ======
inline void runRestPose() { 
  setFaceWithMode("rest", FACE_ANIM_BOOMERANG); 
  for (int i = 0; i < 8; i++) setServoAngle(i, 90); 
}

inline void runStandPose(int face) { 
  if (face == 1) setFaceWithMode("stand", FACE_ANIM_ONCE); 
  setServoAngle(0, 135); 
  setServoAngle(1, 45); 
  setServoAngle(2, 45); 
  setServoAngle(3, 135); 
  setServoAngle(4, 0); 
  setServoAngle(5, 180); 
  setServoAngle(6, 0); 
  setServoAngle(7, 180); 
  if (face == 1) enterIdle();
}

inline void runWavePose() { 
  setFaceWithMode("wave", FACE_ANIM_ONCE); 
  runStandPose(0); 
  delayWithFace(200);
  setServoAngle(4, 80); setServoAngle(6, 180); 
  setServoAngle(3, 60); setServoAngle(0, 100); 
  delayWithFace(200);
  setServoAngle(6, 180); 
  delayWithFace(300); 
  for (int i = 0; i < 4; i++) { 
    setServoAngle(6, 180); delayWithFace(300); 
    setServoAngle(6, 100); delayWithFace(300); 
  } 
  runStandPose(1); 
  if (strcmp(currentCommand, "wave") == 0) currentCommand[0] = '\0';
}

inline void runDancePose() { 
  setFaceWithMode("dance", FACE_ANIM_LOOP); 
  setServoAngle(0, 90); setServoAngle(1, 90); 
  setServoAngle(2, 90); setServoAngle(3, 90); 
  setServoAngle(4, 160); setServoAngle(5, 160); 
  setServoAngle(6, 10); setServoAngle(7, 10); 
  delayWithFace(300); 
  for (int i = 0; i < 5; i++) { 
    setServoAngle(4, 115); setServoAngle(5, 115); 
    setServoAngle(6, 10); setServoAngle(7, 10); 
    delayWithFace(300); 
    setServoAngle(4, 160); setServoAngle(5, 160); 
    setServoAngle(6, 65); setServoAngle(7, 65); 
    delayWithFace(300); 
  } 
  runStandPose(1); 
  if (strcmp(currentCommand, "dance") == 0) currentCommand[0] = '\0';
}

inline void runSwimPose() { 
  setFaceWithMode("swim", FACE_ANIM_ONCE); 
  for (int i = 0; i < 8; i++) setServoAngle(i, 90); 
  for (int i = 0; i < 4; i++) { 
    setServoAngle(0, 135); setServoAngle(1, 45); 
    setServoAngle(2, 45); setServoAngle(3, 135); 
    delayWithFace(400); 
    setServoAngle(0, 90); setServoAngle(1, 90); 
    setServoAngle(2, 90); setServoAngle(3, 90); 
    delayWithFace(400); 
  } 
  runStandPose(1); 
  if (strcmp(currentCommand, "swim") == 0) currentCommand[0] = '\0';
}

inline void runPointPose() { 
  setFaceWithMode("point", FACE_ANIM_BOOMERANG); 
  setServoAngle(3, 60); setServoAngle(0, 135); 
  setServoAngle(1, 100); setServoAngle(7, 180); 
  setServoAngle(2, 25); setServoAngle(6, 145);
  setServoAngle(4, 80); setServoAngle(5, 170); 
  delayWithFace(2000); 
  runStandPose(1); 
  if (strcmp(currentCommand, "point") == 0) currentCommand[0] = '\0';
}

inline void runPushupPose() {
  setFaceWithMode("pushup", FACE_ANIM_ONCE);
  runStandPose(0); 
  delayWithFace(200);
  setServoAngle(2, 0);
  setServoAngle(0, 180);
  setServoAngle(6, 90);
  setServoAngle(5, 90);
  delayWithFace(500);
  for (int i = 0; i < 4; i++) {
    setServoAngle(6, 0);
    setServoAngle(5, 180);
    delayWithFace(600);
    setServoAngle(6, 90);
    setServoAngle(5, 90);
    delayWithFace(500);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "pushup") == 0) currentCommand[0] = '\0';
}

inline void runBowPose() {
  setFaceWithMode("bow", FACE_ANIM_ONCE);
  runStandPose(0); 
  delayWithFace(200);
  setServoAngle(2, 0);
  setServoAngle(0, 180);
  setServoAngle(6, 0);
  setServoAngle(5, 180);
  setServoAngle(3, 180);
  setServoAngle(1, 0);
  setServoAngle(4, 0);
  setServoAngle(7, 180);
  delayWithFace(600);
  setServoAngle(6, 90);
  setServoAngle(5, 90);
  delayWithFace(3000);
  runStandPose(1);
  if (strcmp(currentCommand, "bow") == 0) currentCommand[0] = '\0';
}

inline void runCutePose() {
  setFaceWithMode("cute", FACE_ANIM_ONCE);
  runStandPose(0); 
  delayWithFace(200);
  setServoAngle(3, 160);
  setServoAngle(1, 20);
  setServoAngle(4, 180);
  setServoAngle(7, 0);

  setServoAngle(2, 0);
  setServoAngle(0, 180);
  setServoAngle(6, 180);
  setServoAngle(5, 0);
  delayWithFace(200);
  for (int i = 0; i < 5; i++) {
    setServoAngle(4, 180);
    setServoAngle(7, 45);
    delayWithFace(300);
    setServoAngle(4, 135);
    setServoAngle(7, 0);
    delayWithFace(300);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "cute") == 0) currentCommand[0] = '\0';
}

inline void runFreakyPose() {
  setFaceWithMode("freaky", FACE_ANIM_ONCE);
  runStandPose(0); 
  delayWithFace(200);
  setServoAngle(2, 0);
  setServoAngle(0, 180);
  setServoAngle(3, 180);
  setServoAngle(1, 0);
  setServoAngle(4, 90);
  setServoAngle(5, 0);
  delayWithFace(200);
  for (int i = 0; i < 3; i++) {
    setServoAngle(5, 25);
    delayWithFace(400);
    setServoAngle(5, 0);
    delayWithFace(400);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "freaky") == 0) currentCommand[0] = '\0';
}

inline void runWormPose() {
  setFaceWithMode("worm", FACE_ANIM_ONCE);
  runStandPose(0);
  delayWithFace(200);
  setServoAngle(0, 180); setServoAngle(1, 0); setServoAngle(2, 0); setServoAngle(3, 180);
  setServoAngle(4, 90); setServoAngle(5, 90); setServoAngle(6, 90); setServoAngle(7, 90);
  delayWithFace(200);
  for(int i=0; i<5; i++) {
    setServoAngle(5, 45); setServoAngle(6, 135); setServoAngle(4, 45); setServoAngle(7, 135);
    delayWithFace(300);
    setServoAngle(5, 135); setServoAngle(6, 45); setServoAngle(4, 135); setServoAngle(7, 45);
    delayWithFace(300);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "worm") == 0) currentCommand[0] = '\0';
}

inline void runShakePose() {
  setFaceWithMode("shake", FACE_ANIM_ONCE);
  runStandPose(0);
  delayWithFace(200);
  setServoAngle(0, 135); setServoAngle(2, 45); setServoAngle(6, 90); setServoAngle(5, 90);
  setServoAngle(3, 90); setServoAngle(1, 90);
  delayWithFace(200);
  for(int i=0; i<5; i++) {
    setServoAngle(4, 45); setServoAngle(7, 135);
    delayWithFace(300);
    setServoAngle(4, 0); setServoAngle(7, 180);
    delayWithFace(300);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "shake") == 0) currentCommand[0] = '\0';
}

inline void runShrugPose() {
  runStandPose(0);
  setFaceWithMode("dead", FACE_ANIM_ONCE);
  delayWithFace(200);
  setServoAngle(5, 90); setServoAngle(4, 90); setServoAngle(6, 90); setServoAngle(7, 90);
  delayWithFace(1000);
  setFaceWithMode("shrug", FACE_ANIM_ONCE);
  setServoAngle(5, 0); setServoAngle(4, 180); setServoAngle(6, 180); setServoAngle(7, 0);
  delayWithFace(1500);
  runStandPose(1);
  if (strcmp(currentCommand, "shrug") == 0) currentCommand[0] = '\0';
}

inline void runDeadPose() {
  runStandPose(0);
  setFaceWithMode("dead", FACE_ANIM_BOOMERANG);
  delayWithFace(200);
  setServoAngle(5, 90); setServoAngle(4, 90); setServoAngle(6, 90); setServoAngle(7, 90);
  if (strcmp(currentCommand, "dead") == 0) currentCommand[0] = '\0';
}

inline void runCrabPose() {
  setFaceWithMode("crab", FACE_ANIM_ONCE);
  runStandPose(0);
  delayWithFace(200);
  setServoAngle(0, 90); setServoAngle(1, 90); setServoAngle(2, 90); setServoAngle(3, 90);
  setServoAngle(4, 0); setServoAngle(5, 180); setServoAngle(6, 45); setServoAngle(7, 135);
  for(int i=0; i<5; i++) {
    setServoAngle(4, 45); setServoAngle(5, 135); setServoAngle(6, 0); setServoAngle(7, 180);
    delayWithFace(300);
    setServoAngle(4, 0); setServoAngle(5, 180); setServoAngle(6, 45); setServoAngle(7, 135);
    delayWithFace(300);
  }
  runStandPose(1);
  if (strcmp(currentCommand, "crab") == 0) currentCommand[0] = '\0';
}

// --- MOVEMENT ANIMATIONS ---
inline void runWalkPose() {
  setFaceWithMode("walk", FACE_ANIM_ONCE);
  // Initial Step
  setServoAngle(5, 135); setServoAngle(6, 45);
  setServoAngle(1, 100); setServoAngle(2, 25);
  if (!pressingCheck("forward", frameDelay)) return;
  
  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(5, 135); setServoAngle(6, 0);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(7, 135); setServoAngle(3, 90);
    setServoAngle(4, 0); setServoAngle(0, 180);
    if (!pressingCheck("forward", frameDelay)) return;    
    setServoAngle(1, 45); setServoAngle(2, 90);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(4, 45); setServoAngle(7, 180);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(5, 180); setServoAngle(6, 45);
    setServoAngle(1, 90); setServoAngle(2, 0);
    if (!pressingCheck("forward", frameDelay)) return;  
    setServoAngle(3, 135); setServoAngle(0, 90);
    if (!pressingCheck("forward", frameDelay)) return;
  }
  runStandPose(1);
}

// Logic reversed from Walk
inline void runWalkBackward() {
  setFaceWithMode("walk", FACE_ANIM_ONCE);
  if (!pressingCheck("backward", frameDelay)) return;
  
  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(5, 135); setServoAngle(6, 0);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(7, 135); setServoAngle(3, 135);
    setServoAngle(4, 0); setServoAngle(0, 90);
    if (!pressingCheck("backward", frameDelay)) return;    
    setServoAngle(1, 90); setServoAngle(2, 0);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(4, 45); setServoAngle(7, 180);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(5, 180); setServoAngle(6, 45);
    setServoAngle(1, 45); setServoAngle(2, 90);
    if (!pressingCheck("backward", frameDelay)) return;  
    setServoAngle(3, 90); setServoAngle(0, 180);
    if (!pressingCheck("backward", frameDelay)) return;
  }
  runStandPose(1);
}

// Simple turn logic
inline void runTurnLeft() {
  setFaceWithMode("walk", FACE_ANIM_ONCE);
  for (int i = 0; i < walkCycles; i++) {
    //legset 1 (R1 L2)
    setServoAngle(5, 135); setServoAngle(7, 135); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(0, 180); setServoAngle(3, 180); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(5, 180); setServoAngle(7, 180); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(0, 135); setServoAngle(3, 135);
    if (!pressingCheck("left", frameDelay)) return;
      //legset 2 (R2 L1)
    setServoAngle(4, 45); setServoAngle(6, 45); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(1, 90); setServoAngle(2, 90); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(4, 0); setServoAngle(6, 0); 
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(1, 45); setServoAngle(2, 45);
    if (!pressingCheck("left", frameDelay)) return;  
  }
  runStandPose(1);
}

inline void runTurnRight() {
  setFaceWithMode("walk", FACE_ANIM_ONCE);
  for (int i = 0; i < walkCycles; i++) {
    //legset 2 (R2 L1)
    setServoAngle(4, 45); setServoAngle(6, 45); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(1, 0); setServoAngle(2, 0); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(4, 0); setServoAngle(6, 0); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(1, 45); setServoAngle(2, 45);
    if (!pressingCheck("right", frameDelay)) return;  
    //legset 1 (R1 L2)
    setServoAngle(5, 135); setServoAngle(7, 135); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(0, 90); setServoAngle(3, 90); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(5, 180); setServoAngle(7, 180); 
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(0, 135); setServoAngle(3, 135);
    if (!pressingCheck("right", frameDelay)) return;
  }
  runStandPose(1);
}
