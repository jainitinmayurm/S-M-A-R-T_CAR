/*
 * =============================================================
 *  Arduino_Controller.ino — Jai RC Rover Motor & Sensor Node
 *  Project by Jai Nitin
 * =============================================================
 *
 *  ROLE
 *    Receives single-char drive commands from the Main ESP32
 *    via SoftwareSerial and translates them into L293D motor
 *    outputs.  During RTH mode, actively scans for obstacles
 *    with HC-SR04 + servo sweep and executes a fully dynamic,
 *    symmetrical "Active Detour" manoeuvre.
 *
 *  HARDWARE — L293D Motor Drive Shield V1.0
 *    M1  = Front-Right       M2  = Front-Left
 *    M3  = Back-Left         M4  = Back-Right
 *    SERVO_1 (pin 10)        = Ultrasonic sweep servo
 *
 *  ANALOG-BLOCK WIRING  (A0-A5 breakout)
 *    A0  =  Left  IR sensor  (digital read)
 *    A1  =  Right IR sensor  (digital read)
 *    A2  =  SoftwareSerial RX  ← ESP32 TX2 (GPIO 17)
 *    A3  =  SoftwareSerial TX  → ESP32 RX2 (GPIO 16)
 *    A4  =  HC-SR04  TRIG
 *    A5  =  HC-SR04  ECHO
 *
 *  LIBRARIES
 *    <AFMotor.h>   — Adafruit Motor Shield V1
 *    <Servo.h>     — Arduino built-in (Timer1 — no conflict)
 *    <SoftwareSerial.h>
 *
 *  SPEED MAP   1→100   2→150   3→200   4→255
 *
 * =============================================================
 */

#include <AFMotor.h>
#include <Servo.h>
#include <SoftwareSerial.h>

/* ─── Motor Objects ───────────────────────────────────────── */
AF_DCMotor mFR(1);      /* M1 — Front-Right */
AF_DCMotor mFL(2);      /* M2 — Front-Left  */
AF_DCMotor mBL(3);      /* M3 — Back-Left   */
AF_DCMotor mBR(4);      /* M4 — Back-Right  */

/* ─── Servo ───────────────────────────────────────────────── */
Servo scanServo;
#define SERVO_PIN 10

/* ─── IR Sensors (digital read on analog pins) ────────────── */
#define IR_LEFT   A0     /* LOW = object detected */
#define IR_RIGHT  A1

/* ─── SoftwareSerial to Main ESP32 ────────────────────────── */
SoftwareSerial espSerial(A2, A3);        /* RX, TX */

/* ─── Ultrasonic HC-SR04 ──────────────────────────────────── */
#define US_TRIG  A4
#define US_ECHO  A5

/* ─── Tuneable Constants ──────────────────────────────────── */
static const uint8_t  SPEED_MAP[]      = {0, 100, 150, 200, 255}; /* idx 1-4 */
static const int      OBSTACLE_CM      = 15;
static const unsigned long TURN_90_MS  = 420;   /* ← calibrate for your chassis */
static const unsigned long IR_TIMEOUT  = 12000; /* ms — max wait for IR edges    */
static const unsigned long POST_CLEAR  = 500;   /* ms — extra drive after clear  */
static const unsigned long TELEM_INT   = 500;   /* ms — telemetry interval        */

/* ─── Runtime State ───────────────────────────────────────── */
static uint8_t       curSpeed    = 200;         /* default speed level 3 */
static bool          rthActive   = false;
static unsigned long lastTelemMs = 0;
static unsigned long lastUsMs    = 0;           /* ultrasonic poll timer */
static unsigned long lastEspCmdMs = 0;          /* wire-break watchdog   */

/* ══════════════════════════════════════════════════════════ */
/*  Low-Level Motor Helpers                                   */
/* ══════════════════════════════════════════════════════════ */

static void setAllSpeed(uint8_t spd) {
  mFR.setSpeed(spd); mFL.setSpeed(spd);
  mBL.setSpeed(spd); mBR.setSpeed(spd);
}

static void halt() {
  mFR.run(RELEASE); mFL.run(RELEASE);
  mBL.run(RELEASE); mBR.run(RELEASE);
}

static void driveForward() {
  setAllSpeed(curSpeed);
  mFL.run(FORWARD); mFR.run(FORWARD);
  mBL.run(FORWARD); mBR.run(FORWARD);
}

static void driveBackward() {
  setAllSpeed(curSpeed);
  mFL.run(BACKWARD); mFR.run(BACKWARD);
  mBL.run(BACKWARD); mBR.run(BACKWARD);
}

static void spinLeft() {                         /* pivot in place */
  setAllSpeed(curSpeed);
  mFL.run(BACKWARD); mBL.run(BACKWARD);
  mFR.run(FORWARD);  mBR.run(FORWARD);
}

static void spinRight() {
  setAllSpeed(curSpeed);
  mFL.run(FORWARD);  mBL.run(FORWARD);
  mFR.run(BACKWARD); mBR.run(BACKWARD);
}

/* Diagonal — one side full speed, other side ~40 % */
static void driveForwardLeft() {
  mFR.setSpeed(curSpeed);      mBR.setSpeed(curSpeed);
  mFL.setSpeed((curSpeed * 4) / 10); mBL.setSpeed((curSpeed * 4) / 10);
  mFL.run(FORWARD); mFR.run(FORWARD);
  mBL.run(FORWARD); mBR.run(FORWARD);
}
static void driveForwardRight() {
  mFL.setSpeed(curSpeed);      mBL.setSpeed(curSpeed);
  mFR.setSpeed((curSpeed * 4) / 10); mBR.setSpeed((curSpeed * 4) / 10);
  mFL.run(FORWARD); mFR.run(FORWARD);
  mBL.run(FORWARD); mBR.run(FORWARD);
}
static void driveBackwardLeft() {
  mFR.setSpeed(curSpeed);      mBR.setSpeed(curSpeed);
  mFL.setSpeed((curSpeed * 4) / 10); mBL.setSpeed((curSpeed * 4) / 10);
  mFL.run(BACKWARD); mFR.run(BACKWARD);
  mBL.run(BACKWARD); mBR.run(BACKWARD);
}
static void driveBackwardRight() {
  mFL.setSpeed(curSpeed);      mBL.setSpeed(curSpeed);
  mFR.setSpeed((curSpeed * 4) / 10); mBR.setSpeed((curSpeed * 4) / 10);
  mFL.run(BACKWARD); mFR.run(BACKWARD);
  mBL.run(BACKWARD); mBR.run(BACKWARD);
}

/* ── 90° Turns (blocking) ────────────────────────────────── */
static void turnLeft90()  { spinLeft();  delay(TURN_90_MS); halt(); }
static void turnRight90() { spinRight(); delay(TURN_90_MS); halt(); }

/* ══════════════════════════════════════════════════════════ */
/*  Command Dispatcher                                        */
/* ══════════════════════════════════════════════════════════ */
static void execCmd(char c) {
  switch (c) {
    case 'F': driveForward();       break;
    case 'B': driveBackward();      break;
    case 'L': spinLeft();           break;
    case 'R': spinRight();          break;
    case 'G': driveForwardLeft();   break;
    case 'H': driveForwardRight();  break;
    case 'I': driveBackwardLeft();  break;
    case 'J': driveBackwardRight(); break;
    case 'S': halt();               break;
    case '1': curSpeed = SPEED_MAP[1]; setAllSpeed(curSpeed); break;
    case '2': curSpeed = SPEED_MAP[2]; setAllSpeed(curSpeed); break;
    case '3': curSpeed = SPEED_MAP[3]; setAllSpeed(curSpeed); break;
    case '4': curSpeed = SPEED_MAP[4]; setAllSpeed(curSpeed); break;
    case 'T': rthActive = true;  break;  /* ESP32 signals RTH start */
    case 't': rthActive = false; break;  /* ESP32 signals RTH end   */
    default:  break;
  }
}

/* ══════════════════════════════════════════════════════════ */
/*  Ultrasonic Distance  (cm)                                 */
/* ══════════════════════════════════════════════════════════ */
static long pingCm() {
  digitalWrite(US_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  unsigned long dur = pulseIn(US_ECHO, HIGH, 6000);   /* ~1.0 m max — saves CPU */
  if (dur == 0) return 999;              /* no echo = far away       */
  return (long)(dur * 0.034 / 2.0);
}

/* ══════════════════════════════════════════════════════════ */
/*  Non-Blocking Detour Helpers                               */
/* ══════════════════════════════════════════════════════════ */
static bool abortDetour = false;

/* Poll serial for cancel commands during blocking manoeuvres */
static void checkSerialFailsafe() {
  while (espSerial.available()) {
    char c = (char)espSerial.read();
    lastEspCmdMs = millis();
    if (c == 't' || c == 'S' || c == 'm') {
      halt();
      rthActive = false;
      abortDetour = true;
    }
    if (c >= ' ') execCmd(c);
  }
}

/* Non-blocking delay that checks for cancel commands */
static bool smartDelay(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    checkSerialFailsafe();
    if (abortDetour) return false;
  }
  return true;
}

/* Non-blocking IR wait that checks for cancel commands */
static bool smartWaitIR(int pin, int state, unsigned long tmo) {
  unsigned long t0 = millis();
  while (digitalRead(pin) != state) {
    checkSerialFailsafe();
    if (abortDetour) return false;
    if (millis() - t0 > tmo) return false;
  }
  return true;
}

/* Send a message to ESP32 and wait for a single-char ACK */
static bool sendAndWaitForAck(const char* msg, char ack, unsigned long timeout) {
  espSerial.println(msg);
  unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    if (espSerial.available()) {
      char c = (char)espSerial.read();
      lastEspCmdMs = millis();
      if (c == ack) return true;
      if (c == 't' || c == 'S' || c == 'm') {
        halt();
        rthActive = false;
        abortDetour = true;
        return false;
      }
      if (c >= ' ') execCmd(c);
    }
  }
  return false;                          /* timed out — no ACK */
}

/* ══════════════════════════════════════════════════════════ */
/*  Active Detour — Fully Dynamic & Symmetrical               */
/* ══════════════════════════════════════════════════════════ */
/*
 *  1. HALT  → Pause ESP32 stack timer ('P')
 *  2. SWEEP → servo 0° (left) and 180° (right), pick open side
 *  3. CLEAR WIDTH
 *       • Turn 90° in Turn_A direction
 *       • Drive forward, start width_timer
 *       • Wait: Active_IR  HIGH → LOW (detect) → HIGH (clear)
 *       • Drive POST_CLEAR more, stop, save width_duration
 *  4. CLEAR LENGTH
 *       • Turn 90° in Turn_B direction
 *       • Drive forward
 *       • Wait: Active_IR  LOW → HIGH
 *       • Drive POST_CLEAR more, stop
 *  5. REJOIN LINE
 *       • Turn 90° in Turn_B direction
 *       • Drive forward for width_duration, stop
 *  6. RESUME VECTOR
 *       • Turn 90° in Turn_A direction (now facing home)
 *       • Send 'R' to ESP32 to resume stack
 */

static void activeDetour() {
  abortDetour = false;

  /* ── 1. Halt & signal pause (with ACK) ── */
  halt();
  if (!sendAndWaitForAck("P", 'p', 2000)) return;

  /* ── 2. Servo sweep ── */
  scanServo.write(90);  if (!smartDelay(350)) return;

  scanServo.write(0);   if (!smartDelay(500)) return;
  long distLeft = pingCm();

  scanServo.write(180); if (!smartDelay(600)) return;
  long distRight = pingCm();

  scanServo.write(90);  if (!smartDelay(300)) return;

  /* ── Decide open side ── */
  bool goRight = (distRight >= distLeft);

  int activeIR;
  void (*turnA90)();
  void (*turnB90)();

  if (goRight) {
    turnA90  = turnRight90;
    turnB90  = turnLeft90;
    activeIR = IR_LEFT;
  } else {
    turnA90  = turnLeft90;
    turnB90  = turnRight90;
    activeIR = IR_RIGHT;
  }

  /* ── 3. Clear Width ── */
  turnA90();
  if (!smartDelay(150)) return;

  unsigned long widthStart = millis();
  driveForward();

  if (!smartWaitIR(activeIR, LOW, IR_TIMEOUT)) { halt(); return; }
  if (!smartWaitIR(activeIR, HIGH, IR_TIMEOUT)) { halt(); return; }

  if (!smartDelay(POST_CLEAR)) return;
  halt();
  unsigned long widthDuration = millis() - widthStart;
  if (!smartDelay(150)) return;

  /* ── 4. Clear Length ── */
  turnB90();
  if (!smartDelay(150)) return;

  driveForward();

  if (!smartWaitIR(activeIR, LOW, IR_TIMEOUT)) { halt(); return; }
  if (!smartWaitIR(activeIR, HIGH, IR_TIMEOUT)) { halt(); return; }

  if (!smartDelay(POST_CLEAR)) return;
  halt();
  if (!smartDelay(150)) return;

  /* ── 5. Rejoin Line (with kamikaze prevention) ── */
  turnB90();
  if (!smartDelay(150)) return;

  driveForward();
  {
    unsigned long t0 = millis();
    while (millis() - t0 < widthDuration) {
      checkSerialFailsafe();
      if (abortDetour) return;
      if (pingCm() < OBSTACLE_CM) {
        halt();
        return;                          /* abort — wall ahead */
      }
    }
  }
  halt();
  if (!smartDelay(150)) return;

  /* ── 6. Resume Vector (with ACK) ── */
  turnA90();
  if (!smartDelay(150)) return;

  /* Flush stale serial commands accumulated during detour */
  while (espSerial.available()) espSerial.read();

  /* Signal ESP32 to resume RTH stack timer (with handshake) */
  sendAndWaitForAck("R", 'r', 2000);
}

/* ══════════════════════════════════════════════════════════ */
/*  Telemetry                                                 */
/* ══════════════════════════════════════════════════════════ */
static void sendTelemetry() {
  int speedPct = (int)((unsigned long)curSpeed * 100UL / 255UL);
  espSerial.print("BAT:100,SPEED:");
  espSerial.println(speedPct);
}

/* ══════════════════════════════════════════════════════════ */
/*  SETUP                                                     */
/* ══════════════════════════════════════════════════════════ */
void setup() {
  Serial.begin(9600);                    /* debug monitor              */

  /* ── High-Z boot delay — prevent 5V TX leakage into ESP32 ── */
  pinMode(A3, INPUT);
  delay(3000);

  espSerial.begin(9600);                 /* link to Main ESP32         */
  while (espSerial.available()) espSerial.read();  /* flush boot garbage */

  /* IR sensors — digital inputs */
  pinMode(IR_LEFT,  INPUT);
  pinMode(IR_RIGHT, INPUT);

  /* Ultrasonic */
  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);
  digitalWrite(US_TRIG, LOW);

  /* Servo — write angle BEFORE attach to prevent startup jerk */
  scanServo.write(90);
  scanServo.attach(SERVO_PIN);
  delay(500);

  /* Motors — default speed, stopped */
  setAllSpeed(curSpeed);
  halt();

  Serial.println(F("[UNO] Jai RC Rover — Arduino Controller ready."));
}

/* ══════════════════════════════════════════════════════════ */
/*  LOOP                                                      */
/* ══════════════════════════════════════════════════════════ */
void loop() {

  /* ── Incoming commands from ESP32 ── */
  while (espSerial.available()) {
    char c = (char)espSerial.read();
    lastEspCmdMs = millis();              /* ANY byte feeds the watchdog */
    if (c >= ' ') execCmd(c);
  }

  /* ── Wire-break failsafe (universal — halts even during RTH) ── */
  if (millis() - lastEspCmdMs > 2500) {
    halt();
    rthActive = false;
    lastEspCmdMs = millis();
  }

  /* ── RTH obstacle scanning (every 120 ms while RTH active) ── */
  if (rthActive) {
    if (millis() - lastUsMs > 120) {
      lastUsMs = millis();
      long d = pingCm();
      if (d > 0 && d < OBSTACLE_CM) {
        activeDetour();
      }
    }
  }

  /* ── Periodic telemetry ── */
  if (millis() - lastTelemMs > TELEM_INT) {
    lastTelemMs = millis();
    sendTelemetry();
  }
}
