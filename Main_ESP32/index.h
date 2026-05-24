/*
 * =============================================================
 *  index.h — Jai RC Rover Web Control Interface  (Cyber-Blue)
 *  Project by Jai Nitin
 * =============================================================
 */

#ifndef INDEX_H
#define INDEX_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <title>Jai RC Rover</title>
  <style>
    :root {
      --black: #000000;
      --accent: #00d2ff;
      --accent-bright: #33dfff;
      --accent-glow: rgba(0, 210, 255, 0.55);
      --accent-soft: rgba(0, 210, 255, 0.18);
      --glass: rgba(255, 255, 255, 0.06);
      --glass-edge: rgba(255, 255, 255, 0.1);
      --text: #ffffff;
      --muted: rgba(255, 255, 255, 0.45);
      --danger: #ff3b4a;
      --btn: clamp(44px, 9vmin, 56px);
      --joy: clamp(140px, 38vmin, 280px);
      --aux: clamp(44px, 8vmin, 52px);
      --mono: 'SF Mono', Consolas, 'Liberation Mono', monospace;
    }

    *, ::before, ::after { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    html, body { width: 100%; height: 100%; overflow: hidden; background: var(--black); color: var(--text); font-family: system-ui, -apple-system, sans-serif; }
    body { touch-action: manipulation; }

    /* ── Landscape Lock Hint ─────────────────────────── */
    .rotate-hint { display: none; position: fixed; inset: 0; z-index: 9000; align-items: center; justify-content: center; background: rgba(0,0,0,0.78); backdrop-filter: blur(8px); }
    .rotate-hint span { font-size: 0.85rem; letter-spacing: 0.28em; text-transform: uppercase; color: var(--accent); text-shadow: 0 0 24px var(--accent-glow); }
    @media (orientation: portrait) { .rotate-hint { display: flex; } }

    /* ── App Shell ───────────────────────────────────── */
    .app { position: relative; display: flex; flex-direction: column; width: 100%; height: 100%; padding: max(6px, env(safe-area-inset-top)) max(10px, env(safe-area-inset-right)) max(6px, env(safe-area-inset-bottom)) max(10px, env(safe-area-inset-left)); }
    .stage { flex: 1; display: grid; grid-template-columns: minmax(0, 0.85fr) minmax(0, 2.5fr) minmax(0, 0.85fr); grid-template-rows: auto 1fr; gap: 8px; min-height: 0; }

    /* ── Top Row ─────────────────────────────────────── */
    .top-left  { grid-column: 1; grid-row: 1; display: flex; align-items: center; gap: 8px; }
    .top-center{ grid-column: 2; grid-row: 1; }
    .top-right { grid-column: 3; grid-row: 1; display: flex; justify-content: flex-end; gap: 8px; align-items: center; }

    /* ── Side Panels ─────────────────────────────────── */
    .side { grid-row: 2; display: flex; align-items: center; justify-content: center; position: relative; }
    .side-left  { grid-column: 1; }
    .side-right { grid-column: 3; }

    /* ── Shared Button Styles ────────────────────────── */
    .round-btn, .pill-btn { height: var(--btn); border: 1px solid var(--glass-edge); background: var(--glass); color: var(--text); display: inline-flex; align-items: center; justify-content: center; cursor: pointer; transition: 0.15s; }
    .round-btn { width: var(--btn); border-radius: 50%; }
    .pill-btn  { padding: 0 14px; border-radius: 999px; gap: 6px; }
    .round-btn:active, .pill-btn:active { transform: scale(0.94); }
    .active { border-color: var(--accent); box-shadow: 0 0 16px var(--accent-glow); }

    svg { width: 22px; height: 22px; stroke: currentColor; fill: none; stroke-width: 1.75; stroke-linecap: round; stroke-linejoin: round; }

    /* ── Sync Pill ───────────────────────────────────── */
    .pill-btn .sync-s { font-size: 0.8rem; font-weight: 700; color: var(--danger); font-family: var(--mono); padding: 2px 6px; border-radius: 999px; background: rgba(255,59,74,0.2); border: 1px solid rgba(255,59,74,0.45); }
    .pill-btn.live .sync-s { color: var(--accent); background: rgba(0,210,255,0.12); border-color: rgba(0,210,255,0.4); }

    /* ── RTH Button ──────────────────────────────────── */
    .rth-active { border-color: var(--danger) !important; box-shadow: 0 0 16px rgba(255,59,74,0.5) !important; }
    .rth-active svg path { fill: var(--danger) !important; stroke: none !important; }
    @keyframes pulseRth { 0%,100%{opacity:1} 50%{opacity:.5} }

    /* ── Joysticks ───────────────────────────────────── */
    .joy-zone { position: relative; width: var(--joy); height: var(--joy); }
    .joystick { width: 100%; height: 100%; border-radius: 50%; background: radial-gradient(circle at 50% 50%, rgba(4,12,24,0.94), rgba(0,0,0,0.98)); border: 2px solid rgba(0,210,255,0.35); box-shadow: inset 0 0 40px rgba(0,0,0,0.88); position: relative; transition: opacity 0.25s, filter 0.25s; }
    .joystick.active { border-color: var(--accent-bright); filter: drop-shadow(0 0 14px var(--accent-glow)); }
    .joystick.disabled { opacity: 0.22; pointer-events: none; filter: grayscale(0.6); }

    .joy-knob { position: absolute; width: 44px; height: 44px; margin: -22px 0 0 -22px; left: 50%; top: 50%; border-radius: 50%; background: radial-gradient(circle, rgba(0,210,255,0.38), rgba(0,20,60,0.55)); border: 1px solid rgba(0,210,255,0.5); box-shadow: 0 0 18px var(--accent-glow); pointer-events: none; opacity: 0.35; transition: opacity 0.12s; }
    .joystick.active .joy-knob { opacity: 0.9; }

    /* ── Aux Buttons (around joysticks) ──────────────── */
    .aux-btn { position: absolute; width: var(--aux); height: var(--aux); border-radius: 50%; border: 1px solid var(--glass-edge); background: var(--glass); color: var(--text); display: flex; align-items: center; justify-content: center; cursor: pointer; z-index: 2; backdrop-filter: blur(8px); transition: 0.15s; }
    .aux-btn:active { transform: scale(0.94); }
    .aux-btn.active { border-color: var(--accent); box-shadow: 0 0 14px var(--accent-glow); color: var(--accent); }
    .side-left  .aux-rth  { bottom: 0%; right: -22%; }
    .side-right .aux-gyro { bottom: 0%; left: -22%; }

    /* ── Camera Panel (maximised centre) ─────────────── */
    .cam-panel { grid-column: 2; grid-row: 2; display: flex; align-items: center; justify-content: center; min-height: 0; overflow: hidden; }
    .cam-wrap { width: 100%; height: 100%; border-radius: 12px; overflow: hidden; border: 1px solid rgba(0,210,255,0.18); background: #000; position: relative; display: flex; align-items: center; justify-content: center; }
    .cam-wrap img { max-width: 100%; max-height: 100%; display: block; object-fit: contain; background: #050a14; }
    .cam-ph { position: absolute; inset: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 6px; color: var(--muted); font-size: 0.7rem; letter-spacing: 0.1em; pointer-events: none; }
    .cam-ph svg { width: 36px; height: 36px; opacity: 0.22; }
    .cam-live { position: absolute; top: 6px; left: 8px; padding: 1px 8px; background: rgba(255,59,74,0.8); color: #fff; font-size: 0.55rem; font-weight: 700; letter-spacing: 1.5px; border-radius: 3px; opacity: 0; transition: opacity 0.4s; display: flex; align-items: center; gap: 4px; }
    .cam-live.on { opacity: 1; }
    .cam-live::before { content: ''; width: 5px; height: 5px; background: #fff; border-radius: 50%; animation: blink 1.1s infinite; }
    @keyframes blink { 0%,100%{opacity:1} 50%{opacity:.2} }

    /* ── Telemetry (floating bottom-right) ────────────── */
    .telem-float { position: fixed; bottom: max(10px, env(safe-area-inset-bottom)); right: max(14px, env(safe-area-inset-right)); display: flex; gap: 18px; padding: 7px 20px; border-radius: 999px; background: rgba(0,0,0,0.65); border: 1px solid var(--glass-edge); backdrop-filter: blur(14px); z-index: 60; pointer-events: none; }
    .metric { text-align: center; }
    .metric-label { font-size: 0.55rem; letter-spacing: 0.18em; color: var(--muted); margin-bottom: 1px; }
    .metric-val { font-family: var(--mono); font-size: 1.15rem; color: var(--accent); text-shadow: 0 0 12px var(--accent-glow); }

    /* ── Gear Dock ───────────────────────────────────── */
    .gear-dock { display: flex; flex-direction: column; align-items: center; width: min(96%, 480px); margin: 0 auto; flex-shrink: 0; }
    .gear-tab { width: 68px; height: 22px; border: 1px solid var(--glass-edge); border-bottom: none; border-radius: 10px 10px 0 0; background: var(--glass); color: var(--text); cursor: pointer; font-size: 0.7rem; }
    .gear-panel { width: 100%; display: flex; justify-content: space-evenly; padding: 10px 24px; border-radius: 18px; background: var(--glass); border: 1px solid var(--glass-edge); backdrop-filter: blur(14px); }
    .gear-btn { width: 48px; height: 40px; border: none; background: transparent; color: rgba(255,255,255,0.5); font-family: var(--mono); font-size: 1.05rem; cursor: pointer; transition: 0.15s; }
    .gear-btn.active { color: var(--accent); text-shadow: 0 0 16px var(--accent-glow); }

    /* ── Gyro Indicator ──────────────────────────────── */
    .gyro-hud { position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%); padding: 4px 16px; background: rgba(0,0,0,0.6); border: 1px solid rgba(0,210,255,0.25); border-radius: 10px; font-family: var(--mono); font-size: 0.7rem; color: var(--accent); pointer-events: none; opacity: 0; transition: opacity 0.3s; z-index: 70; letter-spacing: 0.08em; }
    .gyro-hud.on { opacity: 1; }
  </style>
</head>
<body>
  <div class="rotate-hint" aria-hidden="true"><span>Rotate to landscape</span></div>

  <div class="app">
    <div class="stage">

      <!-- ── Top Row ─────────────────────────────── -->
      <div class="top-left">
        <button type="button" class="pill-btn" id="syncPill">
          <svg viewBox="0 0 24 24"><path d="M7 7h5v5M17 17h-5v-5"/><path d="M7 12a5 5 0 0 1 5-5M17 12a5 5 0 0 1-5 5"/></svg>
          <span class="sync-s" id="syncS">S</span>
        </button>
      </div>
      <div class="top-center"></div>
      <div class="top-right">
        <button type="button" class="round-btn" id="autoBtn"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"/><path d="M12 7V4M12 20v-3M7 12H4M20 12h-3"/></svg></button>
        <button type="button" class="round-btn" id="rthBtn"><svg viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z" fill="currentColor" stroke="none"/></svg></button>
      </div>

      <!-- ── Left: Drive Joystick ────────────────── -->
      <div class="side side-left">
        <div class="joy-zone">
          <div class="joystick" id="driveJoy"><div class="joy-knob" id="driveKnob"></div></div>
        </div>
      </div>

      <!-- ── Centre: Camera Feed (maximised) ─────── -->
      <div class="cam-panel">
        <div class="cam-wrap">
          <img id="camStream" src="http://10.10.10.11:81/stream" alt="CAM">
          <div class="cam-ph" id="camPh">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z"/></svg>
            <span>AWAITING FEED</span>
          </div>
          <div class="cam-live" id="camLive">LIVE</div>
        </div>
      </div>

      <!-- ── Right: Steer Joystick + Gyro ────────── -->
      <div class="side side-right">
        <div class="joy-zone">
          <button type="button" class="aux-btn aux-gyro" id="gyroBtn"><svg viewBox="0 0 24 24"><path d="M12 3a9 9 0 1 0 9 9"/><path d="M12 3v4M12 12h4"/></svg></button>
          <div class="joystick" id="steerJoy"><div class="joy-knob" id="steerKnob"></div></div>
        </div>
      </div>

    </div>

    <!-- ── Gear Dock ──────────────────────────────── -->
    <footer class="gear-dock" id="gearDock">
      <button type="button" class="gear-tab" id="gearTab"><span>^</span></button>
      <div class="gear-panel" id="gearPanel">
        <button type="button" class="gear-btn active" data-gear="1">F1</button>
        <button type="button" class="gear-btn" data-gear="2">F2</button>
        <button type="button" class="gear-btn" data-gear="3">F3</button>
        <button type="button" class="gear-btn" data-gear="4">F4</button>
      </div>
    </footer>
  </div>

  <!-- ── Telemetry (absolute bottom-right) ─────────── -->
  <div class="telem-float" id="telemFloat">
    <div class="metric"><div class="metric-label">BAT</div><div class="metric-val" id="batVal">&mdash;</div></div>
    <div class="metric"><div class="metric-label">SPD</div><div class="metric-val" id="spdVal">&mdash;</div></div>
    <div class="metric"><div class="metric-label">TILT</div><div class="metric-val" id="tiltVal">&mdash;</div></div>
  </div>

  <!-- ── Gyroscope HUD (visible when gyro active) ──── -->
  <div class="gyro-hud" id="gyroHud">GYRO: --</div>

<script>
(function () {
  'use strict';

  /* ═══ CONFIG ══════════════════════════════════════════════ */
  var WS_URL = 'ws://' + location.hostname + ':81';
  var HB_MS  = 100;
  var GYRO_DZ = 15;          /* deadzone in degrees */

  /* ═══ STATE ═══════════════════════════════════════════════ */
  var ws         = null;
  var connected  = false;
  var currentCmd = 'S';
  var rthActive  = false;
  var gyroActive = false;
  var gyroCmd    = 'S';
  var activeGear = '1';
  var gearOpen   = true;

  /* ═══ DOM ═════════════════════════════════════════════════ */
  function $(id) { return document.getElementById(id); }
  var syncPill   = $('syncPill'),  syncS    = $('syncS');
  var batVal     = $('batVal'),    spdVal   = $('spdVal'),  tiltVal = $('tiltVal');
  var rthBtn     = $('rthBtn'),    autoBtn  = $('autoBtn'), gyroBtn = $('gyroBtn');
  var gearTab    = $('gearTab'),   gearPanel= $('gearPanel');
  var camImg     = $('camStream'), camPh    = $('camPh'),   camLive = $('camLive');
  var driveJoyEl = $('driveJoy'),  steerJoyEl = $('steerJoy');
  var gyroHud    = $('gyroHud');
  var gearBtns   = document.querySelectorAll('.gear-btn');

  /* ═══ WEBSOCKET ═══════════════════════════════════════════ */
  function wsConnect() {
    if (ws && (ws.readyState === 0 || ws.readyState === 1)) return;
    try { ws = new WebSocket(WS_URL); } catch (e) {
      setTimeout(wsConnect, 2000); return;
    }
    ws.onopen = function () {
      connected = true;
      syncPill.classList.add('live');
      syncS.textContent = 'OK';
      ws.send(activeGear);
    };
    ws.onclose = function () {
      connected = false;
      syncPill.classList.remove('live');
      syncS.textContent = 'S';
      setTimeout(wsConnect, 1500);
    };
    ws.onerror = function () { ws.close(); };
    ws.onmessage = function (ev) {
      var d = ev.data;
      if (d.indexOf('BAT:') === 0) {
        var parts = d.split(',');
        for (var i = 0; i < parts.length; i++) {
          var kv = parts[i].split(':');
          if (kv[0] === 'BAT')   batVal.textContent = kv[1];
          if (kv[0] === 'SPEED') spdVal.textContent = kv[1];
        }
        return;
      }
      if (d === 'MODE:RTH') {
        rthActive = true; rthBtn.classList.add('rth-active');
        driveJoyEl.classList.add('disabled');
        steerJoyEl.classList.add('disabled');
        return;
      }
      if (d === 'MODE:MANUAL') {
        rthActive = false; rthBtn.classList.remove('rth-active');
        if (!gyroActive) {
          driveJoyEl.classList.remove('disabled');
          steerJoyEl.classList.remove('disabled');
        }
        return;
      }
    };
  }
  function wsSend(c) { if (ws && ws.readyState === 1) ws.send(c); }

  /* ═══ HEARTBEAT ═══════════════════════════════════════════ */
  setInterval(function () { wsSend(currentCmd); }, HB_MS);

  /* ═══ CAMERA ══════════════════════════════════════════════ */
  camImg.onload = function () { camPh.style.display = 'none'; camLive.classList.add('on'); };
  camImg.onerror = function () {
    camPh.style.display = 'flex'; camLive.classList.remove('on');
    setTimeout(function () { camImg.src = 'http://10.10.10.11:81/stream?t=' + Date.now(); }, 4000);
  };

  /* ═══ JOYSTICK ════════════════════════════════════════════ */
  function Joystick(joyId, knobId, axis) {
    this.joy = $(joyId); this.knob = $(knobId);
    this.axis = axis; this.on = false; this.tid = null; this.val = 0;
    this.cx = 0; this.cy = 0; this.maxR = 0; this.rect = null;
    var self = this;

    this.joy.addEventListener('touchstart', function (e) {
      e.preventDefault(); if (self.on) return;
      self._measure();
      var t = e.changedTouches[0]; self.tid = t.identifier; self.on = true;
      self.joy.classList.add('active'); self._move(t.clientX, t.clientY);
    }, { passive: false });

    this.joy.addEventListener('touchmove', function (e) {
      e.preventDefault();
      for (var i = 0; i < e.changedTouches.length; i++)
        if (e.changedTouches[i].identifier === self.tid) {
          self._move(e.changedTouches[i].clientX, e.changedTouches[i].clientY); break;
        }
    }, { passive: false });

    var endFn = function (e) {
      for (var i = 0; i < e.changedTouches.length; i++)
        if (e.changedTouches[i].identifier === self.tid) { self._release(); break; }
    };
    this.joy.addEventListener('touchend', endFn);
    this.joy.addEventListener('touchcancel', endFn);

    this.joy.addEventListener('mousedown', function (e) {
      self._measure(); self.on = true; self.joy.classList.add('active');
      self._move(e.clientX, e.clientY);
    });
    document.addEventListener('mousemove', function (e) { if (self.on) self._move(e.clientX, e.clientY); });
    document.addEventListener('mouseup', function () { if (self.on) self._release(); });
  }
  Joystick.prototype._measure = function () {
    this.rect = this.joy.getBoundingClientRect();
    this.cx = this.rect.left + this.rect.width / 2;
    this.cy = this.rect.top  + this.rect.height / 2;
    this.maxR = this.rect.width / 2 - 22;
  };
  Joystick.prototype._move = function (px, py) {
    var dx = px - this.cx, dy = py - this.cy;
    if (this.axis === 'y') dx = 0;
    if (this.axis === 'x') dy = 0;
    var d = Math.sqrt(dx * dx + dy * dy);
    if (d > this.maxR) { dx = dx * this.maxR / d; dy = dy * this.maxR / d; }
    var hw = this.rect.width / 2, hh = this.rect.height / 2;
    this.knob.style.left = (50 + (dx / hw) * 50) + '%';
    this.knob.style.top  = (50 + (dy / hh) * 50) + '%';
    this.val = (this.axis === 'y') ? -(dy / this.maxR) : (dx / this.maxR);
    if (Math.abs(this.val) < 0.18) this.val = 0;
  };
  Joystick.prototype._release = function () {
    this.on = false; this.tid = null; this.val = 0;
    this.joy.classList.remove('active');
    this.knob.style.left = '50%'; this.knob.style.top = '50%';
  };

  var joyDrive = new Joystick('driveJoy', 'driveKnob', 'y');
  var joySteer = new Joystick('steerJoy', 'steerKnob', 'x');

  /* ═══ COMMAND MAPPING ═════════════════════════════════════ */
  function mapCmd(d, s) {
    if (d === 'F' && s === 'L') return 'G';
    if (d === 'F' && s === 'R') return 'H';
    if (d === 'B' && s === 'L') return 'I';
    if (d === 'B' && s === 'R') return 'J';
    if (d === 'F') return 'F';
    if (d === 'B') return 'B';
    if (s === 'L') return 'L';
    if (s === 'R') return 'R';
    return 'S';
  }

  function joyCmd() {
    var ly = joyDrive.val, rx = joySteer.val;
    var d = 'N', s = 'N';
    if (ly > 0) d = 'F'; else if (ly < 0) d = 'B';
    if (rx > 0) s = 'R'; else if (rx < 0) s = 'L';
    return mapCmd(d, s);
  }

  /* 50 ms poll — pick joystick or gyro source */
  setInterval(function () {
    currentCmd = gyroActive ? gyroCmd : joyCmd();
  }, 50);

  /* ═══ GYROSCOPE CONTROL ═══════════════════════════════════ */
  function orientAngle() {
    if (screen.orientation && screen.orientation.angle !== undefined)
      return screen.orientation.angle;
    return window.orientation || 0;
  }

  function onDeviceOrientation(e) {
    if (!gyroActive) return;
    if (e.beta === null || e.gamma === null) return;

    var a = orientAngle();
    var driveVal = 0, steerVal = 0;

    /*  Axis mapping per screen orientation.
     *  Landscape-primary (90°):  phone top = left edge
     *    Forward tilt → gamma goes negative → driveVal positive
     *    Right   tilt → beta  goes positive → steerVal positive
     *  Landscape-secondary (-90° / 270°): invert both.
     */
    switch (a) {
      case 90:
        driveVal = -e.gamma;
        steerVal = e.beta;
        break;
      case -90: case 270:
        driveVal = e.gamma;
        steerVal = -e.beta;
        break;
      default: /* portrait fallback */
        driveVal = e.beta;
        steerVal = e.gamma;
    }

    /* Clamp to usable range */
    driveVal = Math.max(-90, Math.min(90, driveVal));
    steerVal = Math.max(-90, Math.min(90, steerVal));

    /* Deadzone */
    var dAbs = Math.abs(driveVal), sAbs = Math.abs(steerVal);
    if (dAbs < GYRO_DZ) driveVal = 0;
    if (sAbs < GYRO_DZ) steerVal = 0;

    /* Update TILT telemetry */
    tiltVal.textContent = Math.round(driveVal);

    /* HUD overlay */
    var dChar = driveVal > 0 ? 'F' : (driveVal < 0 ? 'B' : '-');
    var sChar = steerVal > 0 ? 'R' : (steerVal < 0 ? 'L' : '-');
    gyroHud.textContent = 'GYRO: ' + dChar + ' ' + sChar;

    /* Map to rover command */
    var d = 'N', s = 'N';
    if (driveVal > 0) d = 'F'; else if (driveVal < 0) d = 'B';
    if (steerVal > 0) s = 'R'; else if (steerVal < 0) s = 'L';
    gyroCmd = mapCmd(d, s);
  }

  function startGyro() {
    window.addEventListener('deviceorientation', onDeviceOrientation, true);
    gyroActive = true;
    gyroBtn.classList.add('active');
    gyroHud.classList.add('on');
    /* Dim joysticks */
    driveJoyEl.classList.add('disabled');
    steerJoyEl.classList.add('disabled');
    joyDrive._release(); joySteer._release();
  }

  function stopGyro() {
    window.removeEventListener('deviceorientation', onDeviceOrientation, true);
    gyroActive = false;
    gyroCmd = 'S';
    currentCmd = 'S';
    wsSend('S');
    gyroBtn.classList.remove('active');
    gyroHud.classList.remove('on');
    tiltVal.textContent = '\u2014';
    /* Re-enable joysticks (unless RTH is active) */
    if (!rthActive) {
      driveJoyEl.classList.remove('disabled');
      steerJoyEl.classList.remove('disabled');
    }
  }

  gyroBtn.addEventListener('click', function () {
    if (gyroActive) {
      stopGyro();
      return;
    }

    /* iOS 13+ requires explicit permission request */
    if (typeof DeviceOrientationEvent !== 'undefined' &&
        typeof DeviceOrientationEvent.requestPermission === 'function') {
      DeviceOrientationEvent.requestPermission()
        .then(function (state) {
          if (state === 'granted') {
            startGyro();
          } else {
            alert('Gyroscope permission denied. Enable it in Settings > Safari > Motion & Orientation Access.');
          }
        })
        .catch(function (err) {
          alert('Gyroscope error: ' + err.message);
        });
    } else if ('DeviceOrientationEvent' in window) {
      startGyro();
    } else {
      alert('Gyroscope not supported on this device.');
    }
  });

  /* ═══ RTH BUTTON ══════════════════════════════════════════ */
  rthBtn.addEventListener('click', function () {
    if (!rthActive) { wsSend('M'); } else { wsSend('m'); }
  });

  /* ═══ AUTO BUTTON (cosmetic toggle) ═══════════════════════ */
  autoBtn.addEventListener('click', function () { autoBtn.classList.toggle('active'); });

  /* ═══ GEAR BUTTONS ════════════════════════════════════════ */
  for (var i = 0; i < gearBtns.length; i++) {
    gearBtns[i].addEventListener('click', (function (btn) {
      return function () {
        activeGear = btn.getAttribute('data-gear');
        for (var j = 0; j < gearBtns.length; j++)
          gearBtns[j].classList.toggle('active', gearBtns[j] === btn);
        wsSend(activeGear);
      };
    })(gearBtns[i]));
  }

  /* ═══ GEAR PANEL TOGGLE ═══════════════════════════════════ */
  gearTab.addEventListener('click', function () {
    gearOpen = !gearOpen;
    gearPanel.style.display = gearOpen ? 'flex' : 'none';
    gearTab.querySelector('span').textContent = gearOpen ? '^' : 'v';
  });

  /* ═══ BOOT ════════════════════════════════════════════════ */
  wsConnect();

})();
</script>
</body>
</html>
)HTML";

#endif /* INDEX_H */
