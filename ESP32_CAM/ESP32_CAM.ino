/*
 * =============================================================
 *  ESP32_CAM.ino — Jai RC Rover Camera Module
 *  Project by Jai Nitin
 * =============================================================
 *
 *  ROLE
 *    Connects to the rover's SoftAP ("Jai_RC_Rover") as a
 *    Wi-Fi station with a static IP.  Serves an MJPEG video
 *    stream that the Web UI pulls directly via an <img> tag.
 *    Optionally forwards Serial telemetry to the Main ESP32
 *    for broadcast to the UI.
 *
 *  BOARD
 *    AI-Thinker ESP32-CAM  (select in Arduino IDE)
 *    Partition Scheme: "Huge APP (3MB No OTA / 1MB SPIFFS)"
 *
 *  WIRING
 *    CAM TX  (GPIO 1)  →  Main ESP32 GPIO 25  (RX1)
 *    CAM RX  (GPIO 3)  →  Main ESP32 GPIO 26  (TX1)
 *    CAM GND            →  Common ground bus
 *    CAM 5V             →  USB power bank 5V
 *
 *  NETWORK
 *    Connects to :  SSID "Jai_RC_Rover" (open network)
 *    Static IP   :  10.10.10.11
 *    Stream URL  :  http://10.10.10.11:81/stream
 *    Status URL  :  http://10.10.10.11/status  (JSON health)
 *
 *  SERIAL TELEMETRY  (115200 baud, optional)
 *    Any Serial.println() output is picked up by the Main
 *    ESP32 on Serial1 and relayed to the Web UI as "CAM:…"
 *
 * =============================================================
 */

#include "esp_camera.h"
#include "esp_http_server.h"
#include <WiFi.h>

/* ═══════════════════════════════════════════════════════════ */
/*  AI-Thinker ESP32-CAM Pin Definitions                      */
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

/* On-board white LED (active HIGH) */
#define FLASH_GPIO_NUM      4

/* ═══════════════════════════════════════════════════════════ */
/*  Network Configuration                                     */
/* ═══════════════════════════════════════════════════════════ */
static const char* ROVER_SSID = "Jai_RC_Rover";
static const char* ROVER_PASS = "";              /* open network */

static const IPAddress CAM_IP(10, 10, 10, 11);
static const IPAddress CAM_GW(10, 10, 10, 1);
static const IPAddress CAM_SUB(255, 255, 255, 0);

/* ═══════════════════════════════════════════════════════════ */
/*  Stream Configuration                                      */
/* ═══════════════════════════════════════════════════════════ */
#define STREAM_PORT       81
#define STATUS_PORT       80

/* MJPEG boundary string */
#define BOUNDARY          "jairover"
static const char* STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" BOUNDARY;
static const char* STREAM_PART_HEADER =
    "\r\n--" BOUNDARY "\r\n"
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %u\r\n"
    "X-Timestamp: %lu\r\n\r\n";

/* ═══════════════════════════════════════════════════════════ */
/*  Server Handles                                            */
/* ═══════════════════════════════════════════════════════════ */
static httpd_handle_t stream_httpd = NULL;
static httpd_handle_t status_httpd = NULL;

/* Frame counter for telemetry */
static volatile unsigned long frameCount = 0;

/* ═══════════════════════════════════════════════════════════ */
/*  MJPEG Stream Handler                                      */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t    res = ESP_OK;
  char         partBuf[128];

  /* Set the response type to multipart MJPEG */
  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  /* Disable HTTP response caching */
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");

  Serial.println("STREAM:CLIENT_CONNECTED");

  /* ── Continuous frame loop ── */
  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("STREAM:FRAME_ERR");
      res = ESP_FAIL;
      break;
    }

    /* Build the multipart header for this frame */
    size_t hdrLen = snprintf(partBuf, sizeof(partBuf),
                             STREAM_PART_HEADER,
                             (unsigned)fb->len,
                             (unsigned long)millis());

    /* Send part header */
    res = httpd_resp_send_chunk(req, partBuf, hdrLen);
    if (res != ESP_OK) { esp_camera_fb_return(fb); break; }

    /* Send JPEG data */
    res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;

    frameCount++;
  }

  Serial.println("STREAM:CLIENT_DISCONNECTED");
  return res;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Status / Health Handler  (port 80, GET /status)           */
/* ═══════════════════════════════════════════════════════════ */
static esp_err_t status_handler(httpd_req_t *req) {
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{"
      "\"project\":\"Jai RC Rover\","
      "\"module\":\"ESP32-CAM\","
      "\"ip\":\"%s\","
      "\"stream\":\"http://%s:%d/stream\","
      "\"uptime_s\":%lu,"
      "\"frames\":%lu,"
      "\"rssi\":%d,"
      "\"heap\":%u"
    "}",
    WiFi.localIP().toString().c_str(),
    WiFi.localIP().toString().c_str(),
    STREAM_PORT,
    millis() / 1000,
    frameCount,
    WiFi.RSSI(),
    ESP.getFreeHeap()
  );

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, buf, strlen(buf));
}

/* Root handler — redirect to /status */
static esp_err_t root_handler(httpd_req_t *req) {
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/status");
  return httpd_resp_send(req, NULL, 0);
}

/* ═══════════════════════════════════════════════════════════ */
/*  Start HTTP Servers                                        */
/* ═══════════════════════════════════════════════════════════ */
static void startStreamServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port   = STREAM_PORT;
  cfg.ctrl_port     = STREAM_PORT;       /* unique control port */
  cfg.max_uri_handlers = 2;

  if (httpd_start(&stream_httpd, &cfg) == ESP_OK) {
    httpd_uri_t uri = {
      .uri      = "/stream",
      .method   = HTTP_GET,
      .handler  = stream_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(stream_httpd, &uri);
    Serial.printf("[CAM] Stream server on port %d\n", STREAM_PORT);
  }
}

static void startStatusServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port   = STATUS_PORT;
  cfg.ctrl_port     = STATUS_PORT + 100; /* unique control port */
  cfg.max_uri_handlers = 4;

  if (httpd_start(&status_httpd, &cfg) == ESP_OK) {
    /* /status — JSON health endpoint */
    httpd_uri_t statusUri = {
      .uri      = "/status",
      .method   = HTTP_GET,
      .handler  = status_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(status_httpd, &statusUri);

    /* / — redirect to /status */
    httpd_uri_t rootUri = {
      .uri      = "/",
      .method   = HTTP_GET,
      .handler  = root_handler,
      .user_ctx = NULL
    };
    httpd_register_uri_handler(status_httpd, &rootUri);

    Serial.printf("[CAM] Status server on port %d\n", STATUS_PORT);
  }
}

/* ═══════════════════════════════════════════════════════════ */
/*  Camera Initialisation                                     */
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

  /*
   * Use PSRAM if available for higher resolution.
   * Without PSRAM, fall back to lower resolution to
   * avoid frame-buffer allocation failures.
   */
  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;     /* 640×480 */
    config.jpeg_quality = 12;                /* 0-63, lower = better */
    config.fb_count     = 2;                 /* double-buffer for smooth stream */
    config.grab_mode    = CAMERA_GRAB_LATEST;
    Serial.println("[CAM] PSRAM detected — VGA, double-buffered");
  } else {
    config.frame_size   = FRAMESIZE_QVGA;    /* 320×240 */
    config.jpeg_quality = 14;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    Serial.println("[CAM] No PSRAM — QVGA, single-buffer");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAM] Init FAILED: 0x%x\n", err);
    return false;
  }

  /* ── Fine-tune image sensor ── */
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);             /* slight brightness boost   */
    s->set_contrast(s, 1);               /* slight contrast boost     */
    s->set_saturation(s, 0);             /* neutral saturation        */
    s->set_whitebal(s, 1);               /* auto white balance ON     */
    s->set_awb_gain(s, 1);               /* AWB gain ON               */
    s->set_wb_mode(s, 0);                /* auto WB mode              */
    s->set_exposure_ctrl(s, 1);          /* auto exposure ON          */
    s->set_aec2(s, 1);                   /* DSP auto exposure ON      */
    s->set_gain_ctrl(s, 1);              /* auto gain ON              */
    s->set_agc_gain(s, 0);               /* AGC gain = 0 (auto)       */
    s->set_gainceiling(s, (gainceiling_t)6);  /* max gain ceiling     */
    s->set_bpc(s, 1);                    /* black pixel correct ON    */
    s->set_wpc(s, 1);                    /* white pixel correct ON    */
    s->set_lenc(s, 1);                   /* lens correction ON        */
    s->set_hmirror(s, 0);               /* flip if mounting is mirrored */
    s->set_vflip(s, 0);                 /* flip if mounting is upside-down */
  }

  Serial.println("[CAM] Sensor initialised OK");
  return true;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Wi-Fi Connection (Station Mode)                           */
/* ═══════════════════════════════════════════════════════════ */
static bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                  /* keep radio active for stream */
  WiFi.config(CAM_IP, CAM_GW, CAM_SUB);
  WiFi.begin(ROVER_SSID, ROVER_PASS);

  Serial.printf("[WIFI] Connecting to \"%s\"", ROVER_SSID);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
    /* 15-second timeout — the Main ESP32 AP might not be up yet */
    if (millis() - t0 > 15000) {
      Serial.println("\n[WIFI] Connection TIMEOUT");
      return false;
    }
  }

  Serial.printf("\n[WIFI] Connected!  IP: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}

/* ═══════════════════════════════════════════════════════════ */
/*  Wi-Fi Reconnection Watchdog                               */
/* ═══════════════════════════════════════════════════════════ */
static unsigned long lastWiFiCheck = 0;

static void wifiWatchdog() {
  if (millis() - lastWiFiCheck < 5000) return;   /* check every 5 s */
  lastWiFiCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Lost connection — reconnecting…");
    WiFi.disconnect();
    WiFi.begin(ROVER_SSID, ROVER_PASS);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
      delay(300);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Reconnected  IP: %s\n",
                    WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[WIFI] Reconnect failed — will retry");
    }
  }
}

/* ═══════════════════════════════════════════════════════════ */
/*  Periodic Telemetry  (sent over Serial → Main ESP32)       */
/* ═══════════════════════════════════════════════════════════ */
static unsigned long lastTelemMs = 0;

static void sendTelemetry() {
  if (millis() - lastTelemMs < 3000) return;     /* every 3 s */
  lastTelemMs = millis();

  Serial.printf("STATUS:RSSI=%d,HEAP=%u,FPS=%lu\n",
                WiFi.RSSI(),
                ESP.getFreeHeap(),
                frameCount);
  /* frameCount keeps incrementing — Main ESP32 can compute
     delta for FPS if needed. */
}

/* ═══════════════════════════════════════════════════════════ */
/*  SETUP                                                     */
/* ═══════════════════════════════════════════════════════════ */
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println(F("\n===== Jai RC Rover — ESP32-CAM Module ====="));

  /* Turn off the blinding flash LED */
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  /* ── 1. Camera ── */
  if (!initCamera()) {
    Serial.println(F("[FATAL] Camera init failed. Restarting in 5 s…"));
    delay(5000);
    ESP.restart();
  }

  /* ── 2. Wi-Fi ── */
  if (!connectWiFi()) {
    Serial.println(F("[WARN] Wi-Fi not available yet — will retry in loop"));
  }

  /* ── 3. HTTP Servers ── */
  startStreamServer();
  startStatusServer();

  Serial.println(F("[BOOT] ESP32-CAM ready."));
  Serial.printf("[BOOT] Stream → http://%s:%d/stream\n",
                CAM_IP.toString().c_str(), STREAM_PORT);
  Serial.printf("[BOOT] Status → http://%s/status\n",
                CAM_IP.toString().c_str());
  Serial.println();
}

/* ═══════════════════════════════════════════════════════════ */
/*  LOOP                                                      */
/* ═══════════════════════════════════════════════════════════ */
void loop() {
  wifiWatchdog();
  sendTelemetry();
  delay(10);                             /* yield to RTOS tasks */
}
