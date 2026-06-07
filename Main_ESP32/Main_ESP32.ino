/*
 * =============================================================
 *  Main_ESP32.ino — Jai RC Rover Central Hub
 *  Project by Jai Nitin
 * =============================================================
 *
 *  ROLE
 *    Wi-Fi SoftAP  →  WebSocket server  →  Web control UI
 *    Serial2 bridge  →  Arduino UNO (motor commands + telemetry)
 *    Serial1 bridge  →  ESP32-CAM    (telemetry passthrough)
 *    Dead-reckoning memory stack  →  RTH execution engine
 *
 *  WIRING
 *    Serial2 @ 9600  :  RX2 = GPIO 16  →  Arduino A3 (TX)
 *                       TX2 = GPIO 17  →  Arduino A2 (RX)
 *    Serial1 @ 115200:  RX1 = GPIO 25  →  CAM TX
 *                       TX1 = GPIO 26  →  CAM RX
 *
 *  LIBRARIES  (install via Arduino Library Manager)
 *    - WebSockets  by Markus Sattler  (WebSocketsServer.h)
 *
 *  COMMAND PROTOCOL  (single ASCII char)
 *    UI → ESP32 → Arduino
 *      F B L R G H I J  =  movement directions
 *      S                =  stop / heartbeat
 *      1 2 3 4          =  speed presets
 *      M                =  activate RTH
 *      m                =  cancel  RTH
 *
 *    ESP32 → Arduino (extra control chars)
 *      T / t            =  RTH mode ON / OFF
 *
 *    Arduino → ESP32 → WebSocket
 *      BAT:xxx,SPEED:yy =  telemetry line
 *      P                =  pause RTH timer (obstacle)
 *      R                =  resume RTH timer
 *
 *    ESP32 → WebSocket (status strings)
 *      MODE:RTH / MODE:MANUAL
 *      RTH:COMPLETE
 *      CAM:…            =  forwarded CAM telemetry
 *
 * =============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "index.h"

/* ─── AP Configuration ─────────────────────────────────────── */
static const char* AP_SSID = "Jai_RC_Rover";
static const char* AP_PASS = "password123";

static const IPAddress AP_IP(10, 10, 10, 10);
static const IPAddress AP_GW(10, 10, 10, 1);
static const IPAddress AP_SUB(255, 255, 255, 0);

/* ─── Serial Pin Mapping ──────────────────────────────────── */
#define RXD2  16          /* ← Arduino A3 (SoftwareSerial TX) */
#define TXD2  17          /* → Arduino A2 (SoftwareSerial RX) */
#define RXD1  25          /* ← ESP32-CAM TX                   */
#define TXD1  26          /* → ESP32-CAM RX                   */

/* ─── Servers ─────────────────────────────────────────────── */
WebServer       httpServer(80);
WebSocketsServer wsServer(81);

/* ─── Heartbeat Watchdog ──────────────────────────────────── */
static unsigned long lastPingMs      = 0;
static const unsigned long HB_TIMEOUT = 1500;   /* ms */
static bool clientAlive = false;

/* ─── Rover Mode ──────────────────────────────────────────── */
enum Mode { MANUAL, RTH };
static Mode roverMode = MANUAL;

/* ─── Movement Memory Stack ───────────────────────────────── */
struct MoveEntry { char cmd; unsigned long ms; };

static const int STACK_CAP = 150;
static MoveEntry stack[STACK_CAP];
static int       sp = -1;               /* top-of-stack index */

/* Tracking the active manual command for duration recording   */
static char          liveCmd      = 'S';
static unsigned long liveCmdStart = 0;

/* ─── RTH Execution State ────────────────────────────────── */
static int           rthIdx       = -1;  /* current stack pos  */
static char          rthCmd       = 'S'; /* inverted cmd       */
static unsigned long rthStepStart = 0;
static unsigned long rthElapsed   = 0;   /* survives pause     */
static bool          rthPaused    = false;
static char          lastSentCmd  = 0;   /* dedup serial writes*/

/* ─── RTH Kinematic Drift Compensation ────────────────────── */
static const float RTH_BATTERY_COMPENSATION = 1.12;

/* ─── Serial Receive Buffers (global to avoid heap fragmentation) ── */
static String ardBuf;
static String camBuf;

/* ══════════════════════════════════════════════════════════ */
/*  Forward declarations                                      */
/* ══════════════════════════════════════════════════════════ */
static bool isMoveCmd(char c);
static void pushMove(char c, unsigned long dur);
static void finalizeCmd();
static char invert(char c);
static void rthStart();
static void rthCancel();
static void rthTick();
static void toArduino(char c);
static void broadcast(const String &s);
static void onWsEvent(uint8_t num, WStype_t type,
                       uint8_t *payload, size_t len);
static void handleArduinoLine(const String &line);

/* ══════════════════════════════════════════════════════════ */
/*  SETUP                                                     */
/* ══════════════════════════════════════════════════════════ */
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n===== Jai RC Rover — Main ESP32 Hub ====="));

  /* ── High-Z boot delay — prevent 3.3V TX leakage into Arduino ── */
  pinMode(TXD2, INPUT);
  delay(3000);

  /* ── UART to Arduino & CAM ── */
  Serial2.begin(9600,   SERIAL_8N1, RXD2, TXD2);
  while (Serial2.available()) Serial2.read();      /* flush boot garbage */
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);

  /* ── Wi-Fi Soft-AP ── */
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);                  /* CRITICAL — keeps AP stable */
  WiFi.softAPConfig(AP_IP, AP_GW, AP_SUB);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WIFI] SSID : %s\n", AP_SSID);
  Serial.printf("[WIFI] IP   : %s\n", WiFi.softAPIP().toString().c_str());

  /* ── HTTP — serve the control page ── */
  httpServer.on("/", []() {
    httpServer.send_P(200, PSTR("text/html"), index_html);
  });
  httpServer.begin();
  Serial.println(F("[HTTP] Listening on :80"));

  /* ── WebSocket ── */
  wsServer.begin();
  wsServer.onEvent(onWsEvent);
  Serial.println(F("[WS]   Listening on :81"));

  /* ── Pre-allocate serial buffers to avoid heap fragmentation ── */
  ardBuf.reserve(128);
  camBuf.reserve(128);

  lastPingMs = millis();
  Serial.println(F("[BOOT] Ready.\n"));
}

/* ══════════════════════════════════════════════════════════ */
/*  LOOP                                                      */
/* ══════════════════════════════════════════════════════════ */
void loop() {
  httpServer.handleClient();
  wsServer.loop();

  /* ── Arduino serial (telemetry / handshake) ────────────── */
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\n') {
      ardBuf.trim();
      if (ardBuf.length()) handleArduinoLine(ardBuf);
      ardBuf = "";
    } else {
      if (ardBuf.length() < 80) ardBuf += c;     /* guard overflow */
    }
  }

  /* ── CAM serial (telemetry passthrough) ────────────────── */
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (c == '\n') {
      camBuf.trim();
      if (camBuf.length()) {
        broadcast("CAM:" + camBuf);
        Serial.print(F("[CAM] ")); Serial.println(camBuf);
      }
      camBuf = "";
    } else {
      if (camBuf.length() < 120) camBuf += c;
    }
  }

  /* ── Heartbeat watchdog (manual mode only) ─────────────── */
  if (roverMode == MANUAL && clientAlive) {
    if (millis() - lastPingMs > HB_TIMEOUT) {
      Serial.println(F("[WDG] Heartbeat lost → RTH"));
      clientAlive = false;
      finalizeCmd();
      rthStart();
    }
  }

  /* ── RTH engine tick ───────────────────────────────────── */
  if (roverMode == RTH) rthTick();

  /* ── Hardware UART heartbeat to Arduino (wire-break detection) ── */
  static unsigned long lastUartHbMs = 0;
  if (millis() - lastUartHbMs > 1000) {
    lastUartHbMs = millis();
    Serial2.write('\n');
  }
}

/* ══════════════════════════════════════════════════════════ */
/*  WebSocket Events                                          */
/* ══════════════════════════════════════════════════════════ */
static void onWsEvent(uint8_t num, WStype_t type,
                       uint8_t *payload, size_t len) {
  switch (type) {

    case WStype_CONNECTED:
      Serial.printf("[WS] #%u connected\n", num);
      clientAlive = true;
      lastPingMs  = millis();
      wsServer.sendTXT(num,
          roverMode == RTH ? "MODE:RTH" : "MODE:MANUAL");
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] #%u disconnected\n", num);
      break;

    case WStype_TEXT: {
      if (len < 1) return;
      lastPingMs  = millis();
      clientAlive = true;
      char cmd = (char)payload[0];

      /* ── Cancel RTH ── */
      if (cmd == 'm') { if (roverMode == RTH) rthCancel(); return; }

      /* ── During RTH only accept 'm' ── */
      if (roverMode == RTH) return;

      /* ── Activate RTH ── */
      if (cmd == 'M') {
        Serial.println(F("[CMD] Manual RTH trigger"));
        finalizeCmd();
        rthStart();
        return;
      }

      /* ── Speed presets ── */
      if (cmd >= '1' && cmd <= '4') { toArduino(cmd); return; }

      /* ── Stop / heartbeat ── */
      if (cmd == 'S') {
        if (liveCmd != 'S') finalizeCmd();
        liveCmd = 'S';
        toArduino('S');
        return;
      }

      /* ── Movement commands ── */
      if (isMoveCmd(cmd)) {
        if (cmd != liveCmd) {
          finalizeCmd();
          liveCmd      = cmd;
          liveCmdStart = millis();
        }
        toArduino(cmd);
      }
      break;
    }

    default: break;
  }
}

/* ══════════════════════════════════════════════════════════ */
/*  Arduino Message Handler                                   */
/* ══════════════════════════════════════════════════════════ */
static void handleArduinoLine(const String &line) {
  /* Handshake: Pause */
  if (line == "P" && roverMode == RTH) {
    rthPaused = true;
    rthElapsed += (millis() - rthStepStart);
    Serial.println(F("[RTH] Paused (obstacle)"));
    return;
  }
  /* Handshake: Resume */
  if (line == "R" && roverMode == RTH && rthPaused) {
    rthPaused    = false;
    rthStepStart = millis();
    toArduino(rthCmd);                   /* re-issue movement  */
    Serial.println(F("[RTH] Resumed"));
    return;
  }
  /* Everything else is telemetry → relay to UI */
  broadcast(line);
}

/* ══════════════════════════════════════════════════════════ */
/*  Stack Helpers                                             */
/* ══════════════════════════════════════════════════════════ */
static bool isMoveCmd(char c) {
  return c=='F'||c=='B'||c=='L'||c=='R'||
         c=='G'||c=='H'||c=='I'||c=='J';
}

static void pushMove(char c, unsigned long dur) {
  if (dur < 60) return;                  /* discard noise */
  if (sp >= STACK_CAP - 1) {
    /* Clamp — refuse to overwrite stack[0] (true home origin) */
    Serial.println(F("[STK] FULL — memory clamped, movement ignored"));
    return;
  }
  sp++;
  stack[sp].cmd = c;
  stack[sp].ms  = dur;
  Serial.printf("[STK] PUSH #%d  '%c'  %lu ms\n", sp, c, dur);
}

static void finalizeCmd() {
  if (isMoveCmd(liveCmd) && liveCmdStart > 0) {
    pushMove(liveCmd, millis() - liveCmdStart);
  }
  liveCmd      = 'S';
  liveCmdStart = 0;
}

/* ══════════════════════════════════════════════════════════ */
/*  Direction Inversion                                       */
/* ══════════════════════════════════════════════════════════ */
static char invert(char c) {
  switch (c) {
    case 'F': return 'B';  case 'B': return 'F';
    case 'L': return 'R';  case 'R': return 'L';
    case 'G': return 'J';  case 'J': return 'G';
    case 'H': return 'I';  case 'I': return 'H';
    default:  return 'S';
  }
}

/* ══════════════════════════════════════════════════════════ */
/*  RTH Engine                                                */
/* ══════════════════════════════════════════════════════════ */
static void rthStart() {
  roverMode  = RTH;
  rthPaused  = false;
  rthElapsed = 0;

  toArduino('S');                        /* full stop first    */
  delay(150);
  toArduino('T');                        /* signal RTH to UNO  */
  broadcast("MODE:RTH");

  rthIdx = sp;
  Serial.printf("[RTH] Start — depth %d\n", sp + 1);

  if (rthIdx >= 0) {
    rthCmd       = invert(stack[rthIdx].cmd);
    rthStepStart = millis();
    rthElapsed   = 0;
    toArduino(rthCmd);
    Serial.printf("[RTH] Step %d: '%c'→'%c' %lu ms\n",
                  rthIdx, stack[rthIdx].cmd, rthCmd, stack[rthIdx].ms);
  }
}

static void rthCancel() {
  roverMode = MANUAL;
  rthPaused = false;
  toArduino('S');
  toArduino('t');                        /* tell UNO RTH is off */
  sp = -1;                               /* flush stack         */
  broadcast("MODE:MANUAL");
  Serial.println(F("[RTH] Cancelled → MANUAL"));
}

static void rthTick() {
  /* Stack exhausted → home */
  if (rthIdx < 0) {
    toArduino('S');
    toArduino('t');
    roverMode = MANUAL;
    sp = -1;
    broadcast("MODE:MANUAL");
    broadcast("RTH:COMPLETE");
    Serial.println(F("[RTH] *** HOME REACHED ***"));
    return;
  }

  if (rthPaused) return;                 /* Arduino handling obstacle */

  unsigned long total = rthElapsed + (millis() - rthStepStart);
  unsigned long targetTime = (unsigned long)(stack[rthIdx].ms * RTH_BATTERY_COMPENSATION);

  if (total >= targetTime) {
    /* Current step finished — advance */
    rthIdx--;
    rthElapsed = 0;
    if (rthIdx >= 0) {
      rthCmd       = invert(stack[rthIdx].cmd);
      rthStepStart = millis();
      toArduino(rthCmd);
      Serial.printf("[RTH] Step %d: '%c'→'%c' %lu ms\n",
                    rthIdx, stack[rthIdx].cmd, rthCmd, stack[rthIdx].ms);
    } else {
      toArduino('S');
    }
  }
}

/* ══════════════════════════════════════════════════════════ */
/*  Utilities                                                 */
/* ══════════════════════════════════════════════════════════ */
static void toArduino(char c) {
  /* De-duplicate rapid identical writes to keep serial clean */
  if (c == lastSentCmd && isMoveCmd(c)) return;
  lastSentCmd = c;
  Serial2.write(c);
}

static void broadcast(const String &s) {
  String msg = s;                        /* mutable copy for library API */
  wsServer.broadcastTXT(msg);
}
