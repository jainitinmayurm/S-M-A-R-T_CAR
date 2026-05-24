/*
 * =============================================================
 *  ESP32_CAM.ino — Jai RC Rover Camera + Diagnostic Server
 *  Project by Jai Nitin
 * =============================================================
 *
 *  BOARD  :  AI-Thinker ESP32-CAM
 *  PARTITION: Huge APP (3MB No OTA / 1MB SPIFFS)
 *
 *  NETWORK
 *    Connects to : "Jai_RC_Rover"  password "password123"
 *    Static IP   : 10.10.10.11
 *
 *  ENDPOINTS
 *    http://10.10.10.11/          → Diagnostic dashboard (HTML)
 *    http://10.10.10.11/capture   → Single JPEG frame
 *    http://10.10.10.11/status    → JSON health check
 *    http://10.10.10.11:81/stream → MJPEG live stream
 *
 *  WIRING
 *    CAM TX (GPIO 1) → Main ESP32 GPIO 25 (RX1)
 *    CAM RX (GPIO 3) → Main ESP32 GPIO 26 (TX1)
 *    Common GND
 *
 * =============================================================
 */

#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>

/* ═══════════════════════════════════════════════════════════ */
/*  AI-Thinker ESP32-CAM Pin Map                              */
/* ═══════════════════════════════════════════════════════════ */
#define PWDN_GPIO_NUM      32
#define RESET_GPIO_NUM     -1
#define XCLK_GPIO_NUM       0
#define SIOD_GPIO_NUM      26
#define SIOC_GPIO_NUM      27
#define Y9_GPIO_NUM        35
#define Y8_GPIO_NUM        34
#define Y7_GPIO_NUM        39
#define Y6_GPIO_NUM        36
#define Y5_GPIO_NUM        21
#define Y4_GPIO_NUM        19
#define Y3_GPIO_NUM        18
#define Y2_GPIO_NUM         5
#define VSYNC_GPIO_NUM     25
#define HREF_GPIO_NUM      23
#define PCLK_GPIO_NUM      22
#define FLASH_GPIO_NUM      4

/* ═══════════════════════════════════════════════════════════ */
/*  Network Config                                            */
/* ═══════════════════════════════════════════════════════════ */
static const char* ROVER_SSID = "Jai_RC_Rover";
static const char* ROVER_PASS = "password123";

static const IPAddress CAM_IP(10, 10, 10, 11);
static const IPAddress CAM_GW(10, 10, 10, 1);
static const IPAddress CAM_SUB(255, 255, 255, 0);

/* ═══════════════════════════════════════════════════════════ */
/*  Error Log (ring buffer)                                   */
/* ═══════════════════════════════════════════════════════════ */
#define LOG_MAX 20
#define LOG_LEN 120
static char errLog[LOG_MAX][LOG_LEN];
static int  logIdx  = 0;
static int  logCount = 0;

static void logError(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(errLog[logIdx], LOG_LEN, fmt, ap);
  va_end(ap);
  logIdx = (logIdx + 1) % LOG_MAX;
  if (logCount < LOG_MAX) logCount++;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Global State                                              */
/* ═══════════════════════════════════════════════════════════ */
static bool    camReady    = false;
static int     camError    = 0;          /* esp_err_t from init */
static char    camSensor[32] = "unknown";
static bool    hasPSRAM    = false;
static volatile unsigned long frameCount = 0;
static unsigned long bootTime = 0;

static httpd_handle_t stream_httpd = NULL;
static httpd_handle_t dash_httpd   = NULL;

/* ═══════════════════════════════════════════════════════════ */
/*  MJPEG Stream  (port 81 — /stream)                         */
/* ═══════════════════════════════════════════════════════════ */
#define BOUNDARY "jairover"
static const char* STREAM_CT =
    "multipart/x-mixed-replace;boundary=" BOUNDARY;
static const char* PART_HDR =
    "\r\n--" BOUNDARY "\r\n"
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
  if (!camReady) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Camera not initialised");
    logError("Stream rejected — camera not ready");
    return ESP_FAIL;
  }

  esp_err_t res = httpd_resp_set_type(req, STREAM_CT);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  Serial.println("STREAM:CLIENT_CONNECTED");
  logError("Stream client connected");

  char hdr[100];
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      logError("Frame capture failed (fb null)");
      Serial.println("STREAM:FRAME_ERR");
      res = ESP_FAIL; break;
    }
    size_t hLen = snprintf(hdr, sizeof(hdr), PART_HDR, (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, hdr, hLen);
    if (res == ESP_OK)
      res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;
    frameCount++;
  }

  Serial.println("STREAM:CLIENT_DISCONNECTED");
  logError("Stream client disconnected");
  return res;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Single JPEG Capture  (port 80 — /capture)                 */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t capture_handler(httpd_req_t *req) {
  if (!camReady) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Camera not initialised");
    return ESP_FAIL;
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    logError("/capture failed — fb null");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Frame capture failed");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

/* ═══════════════════════════════════════════════════════════ */
/*  JSON Status  (port 80 — /status)                          */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t status_handler(httpd_req_t *req) {
  char buf[400];
  snprintf(buf, sizeof(buf),
    "{"
      "\"cam_ok\":%s,"
      "\"cam_err\":\"0x%X\","
      "\"sensor\":\"%s\","
      "\"psram\":%s,"
      "\"wifi\":\"%s\","
      "\"ip\":\"%s\","
      "\"rssi\":%d,"
      "\"heap\":%u,"
      "\"psram_free\":%u,"
      "\"uptime\":%lu,"
      "\"frames\":%lu"
    "}",
    camReady ? "true" : "false",
    camError,
    camSensor,
    hasPSRAM ? "true" : "false",
    WiFi.status() == WL_CONNECTED ? "connected" : "disconnected",
    WiFi.localIP().toString().c_str(),
    WiFi.RSSI(),
    ESP.getFreeHeap(),
    hasPSRAM ? ESP.getFreePsram() : 0,
    (millis() - bootTime) / 1000,
    frameCount
  );
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, buf, strlen(buf));
}

/* ═══════════════════════════════════════════════════════════ */
/*  Diagnostic Dashboard  (port 80 — /)                       */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t dashboard_handler(httpd_req_t *req) {
  /* Build the log section */
  char logHtml[LOG_MAX * (LOG_LEN + 30)];
  logHtml[0] = '\0';
  if (logCount == 0) {
    strcat(logHtml, "<div class='log-line'>No errors logged.</div>");
  } else {
    int start = (logCount < LOG_MAX) ? 0 : logIdx;
    int count = (logCount < LOG_MAX) ? logCount : LOG_MAX;
    for (int i = count - 1; i >= 0; i--) {
      int idx = (start + i) % LOG_MAX;
      char line[LOG_LEN + 30];
      snprintf(line, sizeof(line), "<div class='log-line'>%s</div>", errLog[idx]);
      strcat(logHtml, line);
    }
  }

  /* Build full page */
  char* page = (char*)malloc(4096 + strlen(logHtml));
  if (!page) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
    return ESP_FAIL;
  }

  snprintf(page, 4096 + strlen(logHtml),
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<meta http-equiv='refresh' content='5'>"
    "<title>CAM Diagnostics</title>"
    "<style>"
    "*, *::before, *::after{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#0a0a0f;color:#e0e0e0;font-family:system-ui,-apple-system,sans-serif;padding:16px;min-height:100vh}"
    "h1{font-size:1.2rem;color:#00ff9f;margin-bottom:16px;letter-spacing:.1em;text-transform:uppercase}"
    ".card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);border-radius:12px;padding:14px 18px;margin-bottom:12px}"
    ".card h2{font-size:.75rem;color:rgba(255,255,255,.4);letter-spacing:.15em;text-transform:uppercase;margin-bottom:10px}"
    ".row{display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(255,255,255,.04)}"
    ".row:last-child{border:none}"
    ".k{color:rgba(255,255,255,.5);font-size:.85rem}"
    ".v{font-family:'SF Mono',Consolas,monospace;font-size:.85rem}"
    ".ok{color:#00ff9f}.err{color:#ff3b4a}.warn{color:#ffb830}"
    ".links{display:flex;gap:10px;flex-wrap:wrap;margin-top:8px}"
    ".links a{display:inline-block;padding:8px 18px;border-radius:8px;background:rgba(0,255,159,.1);color:#00ff9f;text-decoration:none;font-size:.8rem;border:1px solid rgba(0,255,159,.25);transition:.15s}"
    ".links a:hover{background:rgba(0,255,159,.2)}"
    ".links a.danger{background:rgba(255,59,74,.1);color:#ff3b4a;border-color:rgba(255,59,74,.25)}"
    ".log-line{font-family:monospace;font-size:.75rem;color:rgba(255,255,255,.55);padding:3px 0;border-bottom:1px solid rgba(255,255,255,.03)}"
    ".footer{text-align:center;margin-top:20px;font-size:.65rem;color:rgba(255,255,255,.2)}"
    "</style></head><body>"
    "<h1>&#128247; ESP32-CAM Diagnostics</h1>"

    "<div class='card'><h2>Camera</h2>"
    "<div class='row'><span class='k'>Status</span><span class='v %s'>%s</span></div>"
    "<div class='row'><span class='k'>Error Code</span><span class='v %s'>0x%X</span></div>"
    "<div class='row'><span class='k'>Sensor</span><span class='v'>%s</span></div>"
    "<div class='row'><span class='k'>PSRAM</span><span class='v %s'>%s</span></div>"
    "<div class='row'><span class='k'>Frames Served</span><span class='v'>%lu</span></div>"
    "</div>"

    "<div class='card'><h2>Network</h2>"
    "<div class='row'><span class='k'>SSID</span><span class='v'>%s</span></div>"
    "<div class='row'><span class='k'>Wi-Fi</span><span class='v %s'>%s</span></div>"
    "<div class='row'><span class='k'>IP Address</span><span class='v'>%s</span></div>"
    "<div class='row'><span class='k'>RSSI</span><span class='v %s'>%d dBm</span></div>"
    "</div>"

    "<div class='card'><h2>System</h2>"
    "<div class='row'><span class='k'>Free Heap</span><span class='v'>%u bytes</span></div>"
    "<div class='row'><span class='k'>Free PSRAM</span><span class='v'>%u bytes</span></div>"
    "<div class='row'><span class='k'>Uptime</span><span class='v'>%lu s</span></div>"
    "</div>"

    "<div class='card'><h2>Test Endpoints</h2>"
    "<div class='links'>"
    "<a href='/capture'>&#128248; Single Capture</a>"
    "<a href='http://%s:81/stream'>&#127909; MJPEG Stream</a>"
    "<a href='/status'>&#128202; JSON Status</a>"
    "<a href='/restart' class='danger'>&#x21bb; Restart ESP</a>"
    "</div></div>"

    "<div class='card'><h2>Error Log (newest first)</h2>%s</div>"

    "<div class='footer'>Auto-refreshes every 5s &bull; Jai RC Rover &bull; ESP32-CAM Module</div>"
    "</body></html>",

    /* Camera section */
    camReady ? "ok" : "err",
    camReady ? "READY" : "FAILED",
    camError == 0 ? "ok" : "err",
    camError,
    camSensor,
    hasPSRAM ? "ok" : "warn",
    hasPSRAM ? "Detected" : "NOT FOUND",
    frameCount,

    /* Network section */
    ROVER_SSID,
    WiFi.status() == WL_CONNECTED ? "ok" : "err",
    WiFi.status() == WL_CONNECTED ? "Connected" : "DISCONNECTED",
    WiFi.localIP().toString().c_str(),
    WiFi.RSSI() > -60 ? "ok" : (WiFi.RSSI() > -75 ? "warn" : "err"),
    WiFi.RSSI(),

    /* System section */
    ESP.getFreeHeap(),
    hasPSRAM ? ESP.getFreePsram() : 0,
    (millis() - bootTime) / 1000,

    /* Stream link IP */
    WiFi.localIP().toString().c_str(),

    /* Log */
    logHtml
  );

  httpd_resp_set_type(req, "text/html");
  esp_err_t res = httpd_resp_send(req, page, strlen(page));
  free(page);
  return res;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Restart Handler  (/restart)                               */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t restart_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req,
    "<html><body style='background:#0a0a0f;color:#00ff9f;display:flex;"
    "align-items:center;justify-content:center;height:100vh;font-family:monospace'>"
    "<h2>Restarting...</h2></body></html>", HTTPD_RESP_USE_STRLEN);
  delay(500);
  ESP.restart();
  return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Start HTTP Servers                                        */
/* ═══════════════════════════════════════════════════════════ */
static void startStreamServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port    = 81;
  cfg.ctrl_port      = 32768;
  cfg.max_uri_handlers = 2;

  if (httpd_start(&stream_httpd, &cfg) == ESP_OK) {
    httpd_uri_t uri = { "/stream", HTTP_GET, stream_handler, NULL };
    httpd_register_uri_handler(stream_httpd, &uri);
    Serial.println("[CAM] Stream server on :81");
  } else {
    logError("Failed to start stream server on :81");
    Serial.println("[CAM] FAILED to start stream server");
  }
}

static void startDashServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port    = 80;
  cfg.ctrl_port      = 32769;
  cfg.max_uri_handlers = 8;
  cfg.stack_size     = 8192;

  if (httpd_start(&dash_httpd, &cfg) == ESP_OK) {
    httpd_uri_t dashUri    = { "/",        HTTP_GET, dashboard_handler, NULL };
    httpd_uri_t captureUri = { "/capture",  HTTP_GET, capture_handler,   NULL };
    httpd_uri_t statusUri  = { "/status",   HTTP_GET, status_handler,    NULL };
    httpd_uri_t restartUri = { "/restart",  HTTP_GET, restart_handler,   NULL };
    httpd_register_uri_handler(dash_httpd, &dashUri);
    httpd_register_uri_handler(dash_httpd, &captureUri);
    httpd_register_uri_handler(dash_httpd, &statusUri);
    httpd_register_uri_handler(dash_httpd, &restartUri);
    Serial.println("[CAM] Dashboard server on :80");
  } else {
    logError("Failed to start dashboard server on :80");
    Serial.println("[CAM] FAILED to start dashboard server");
  }
}

/* ═══════════════════════════════════════════════════════════ */
/*  Camera Init                                               */
/* ═══════════════════════════════════════════════════════════ */
static bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  hasPSRAM = psramFound();

  if (hasPSRAM) {
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    Serial.println("[CAM] PSRAM found → VGA double-buffer");
  } else {
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 14;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    Serial.println("[CAM] No PSRAM → QVGA single-buffer");
    logError("WARNING: No PSRAM detected — using low resolution");
  }

  esp_err_t err = esp_camera_init(&config);
  camError = (int)err;

  if (err != ESP_OK) {
    Serial.printf("[CAM] INIT FAILED: 0x%X\n", err);
    logError("Camera init failed with error 0x%X", err);

    if (err == ESP_ERR_NOT_FOUND)
      logError("Cause: Camera sensor not detected — check ribbon cable");
    else if (err == ESP_ERR_NOT_SUPPORTED)
      logError("Cause: Sensor type not supported");
    else if (err == ESP_ERR_NO_MEM)
      logError("Cause: Not enough memory for frame buffer");
    else
      logError("Cause: Unknown — try power cycling the board");

    return false;
  }

  /* Identify sensor */
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    switch (s->id.PID) {
      case 0x26: strncpy(camSensor, "OV2640", sizeof(camSensor)); break;
      case 0x21: strncpy(camSensor, "OV7725", sizeof(camSensor)); break;
      case 0x36: strncpy(camSensor, "OV3660", sizeof(camSensor)); break;
      case 0x60: strncpy(camSensor, "OV5640", sizeof(camSensor)); break;
      default:   snprintf(camSensor, sizeof(camSensor), "PID:0x%02X", s->id.PID);
    }

    /* Fine-tune */
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 0);
    s->set_gainceiling(s, (gainceiling_t)6);
    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
  }

  Serial.printf("[CAM] Sensor: %s — OK\n", camSensor);
  return true;
}

/* ═══════════════════════════════════════════════════════════ */
/*  WiFi                                                      */
/* ═══════════════════════════════════════════════════════════ */
static bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.config(CAM_IP, CAM_GW, CAM_SUB);
  WiFi.begin(ROVER_SSID, ROVER_PASS);

  Serial.printf("[WIFI] Connecting to \"%s\"", ROVER_SSID);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
    if (millis() - t0 > 15000) {
      Serial.println("\n[WIFI] TIMEOUT");
      logError("WiFi connection timeout (15s) to SSID: %s", ROVER_SSID);
      return false;
    }
  }

  Serial.printf("\n[WIFI] Connected  IP: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}

static unsigned long lastWiFiCheck = 0;
static void wifiWatchdog() {
  if (millis() - lastWiFiCheck < 5000) return;
  lastWiFiCheck = millis();
  if (WiFi.status() != WL_CONNECTED) {
    logError("WiFi lost — attempting reconnect");
    Serial.println("[WIFI] Lost — reconnecting…");
    WiFi.disconnect();
    WiFi.begin(ROVER_SSID, ROVER_PASS);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(300);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Reconnected  IP: %s\n", WiFi.localIP().toString().c_str());
      logError("WiFi reconnected OK");
    } else {
      logError("WiFi reconnect failed — will retry");
    }
  }
}

/* ═══════════════════════════════════════════════════════════ */
/*  Periodic Telemetry to Main ESP32                          */
/* ═══════════════════════════════════════════════════════════ */
static unsigned long lastTelemMs = 0;
static void sendTelemetry() {
  if (millis() - lastTelemMs < 3000) return;
  lastTelemMs = millis();
  Serial.printf("STATUS:RSSI=%d,HEAP=%u,FPS=%lu,CAM=%s\n",
                WiFi.RSSI(), ESP.getFreeHeap(), frameCount,
                camReady ? "OK" : "FAIL");
}

/* ═══════════════════════════════════════════════════════════ */
/*  SETUP                                                     */
/* ═══════════════════════════════════════════════════════════ */
void setup() {
  bootTime = millis();
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println(F("\n===== Jai RC Rover — ESP32-CAM Module ====="));

  /* Flash LED OFF */
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  /* 1. Camera */
  camReady = initCamera();
  if (!camReady) {
    Serial.println(F("[WARN] Camera failed — dashboard will show diagnostics"));
  }

  /* 2. WiFi */
  if (!connectWiFi()) {
    Serial.println(F("[WARN] WiFi failed — will retry in loop"));
    logError("Initial WiFi connection failed");
  }

  /* 3. Servers — start ALWAYS, even if camera failed */
  startDashServer();
  startStreamServer();

  Serial.println(F("[BOOT] ESP32-CAM ready."));
  Serial.printf("[BOOT] Dashboard → http://%s/\n", CAM_IP.toString().c_str());
  Serial.printf("[BOOT] Stream    → http://%s:81/stream\n", CAM_IP.toString().c_str());
}

/* ═══════════════════════════════════════════════════════════ */
/*  LOOP                                                      */
/* ═══════════════════════════════════════════════════════════ */
void loop() {
  wifiWatchdog();
  sendTelemetry();
  delay(10);
}
