/*
 * =============================================================
 *  index.h — Jai RC Rover Web Control Interface
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
  <title>Universal Rover</title>
  <style>
    :root {
      --black: #000000;
      --mint: #00ff9f;
      --mint-bright: #00ffcc;
      --mint-glow: rgba(0, 255, 159, 0.55);
      --mint-soft: rgba(0, 255, 159, 0.18);
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

    .rotate-hint { display: none; position: fixed; inset: 0; z-index: 9000; align-items: center; justify-content: center; background: rgba(0,0,0,0.72); backdrop-filter: blur(6px); pointer-events: none; }
    .rotate-hint span { font-size: 0.85rem; letter-spacing: 0.28em; text-transform: uppercase; color: var(--mint); text-shadow: 0 0 24px var(--mint-glow); }
    @media (orientation: portrait) { .rotate-hint { display: flex; } }

    .app { position: relative; display: flex; flex-direction: column; width: 100%; height: 100%; padding: max(8px, env(safe-area-inset-top)) max(10px, env(safe-area-inset-right)) max(6px, env(safe-area-inset-bottom)) max(10px, env(safe-area-inset-left)); }
    .stage { flex: 1; display: grid; grid-template-columns: minmax(0, 1fr) minmax(0, 1.15fr) minmax(0, 1fr); grid-template-rows: auto 1fr; gap: 12px; }

    .top-left { grid-column: 1; grid-row: 1; display: flex; align-items: center; gap: 10px; }
    .top-center { grid-column: 2; grid-row: 1; display: flex; justify-content: center; }
    .top-right { grid-column: 3; grid-row: 1; display: flex; justify-content: flex-end; gap: 10px; }

    .side { grid-row: 2; display: flex; align-items: center; justify-content: center; position: relative; }
    .side-left { grid-column: 1; }
    .side-right { grid-column: 3; }

    .icon-btn, .round-btn, .pill-btn { height: var(--btn); border: 1px solid var(--glass-edge); background: var(--glass); color: var(--text); display: inline-flex; align-items: center; justify-content: center; cursor: pointer; transition: 0.15s; }
    .icon-btn { width: var(--btn); border-radius: 16px; }
    .round-btn { width: var(--btn); border-radius: 50%; position: relative; }
    .pill-btn { padding: 0 14px; border-radius: 999px; gap: 6px; }
    .icon-btn:active, .round-btn:active, .pill-btn:active { transform: scale(0.94); }
    .active { border-color: var(--mint); box-shadow: 0 0 16px var(--mint-glow); }

    svg { width: 22px; height: 22px; stroke: currentColor; fill: none; stroke-width: 1.75; stroke-linecap: round; stroke-linejoin: round; }

    .pill-btn .sync-s { font-size: 0.8rem; font-weight: 700; color: var(--danger); font-family: var(--mono); padding: 2px 6px; border-radius: 999px; background: rgba(255,59,74,0.2); border: 1px solid rgba(255,59,74,0.45); }
    .pill-btn.live .sync-s { color: var(--mint); background: rgba(0,255,159,0.12); border-color: rgba(0,255,159,0.4); }

    .telemetry { display: flex; gap: 32px; padding: 10px 28px; border-radius: 999px; background: var(--glass); border: 1px solid var(--glass-edge); backdrop-filter: blur(12px); }
    .metric { text-align: center; }
    .metric-label { font-size: 0.6rem; letter-spacing: 0.18em; color: var(--muted); margin-bottom: 2px; }
    .metric-val { font-family: var(--mono); font-size: 1.45rem; color: var(--mint); text-shadow: 0 0 14px var(--mint-glow); }

    .joy-zone { position: relative; width: var(--joy); height: var(--joy); }
    .joystick { width: 100%; height: 100%; border-radius: 50%; background: radial-gradient(circle at 50% 50%, rgba(8,14,12,0.92), rgba(0,0,0,0.98)); border: 2px solid rgba(0,255,159,0.38); box-shadow: inset 0 0 40px rgba(0,0,0,0.88); position: relative; }
    .joystick.active { border-color: var(--mint-bright); filter: drop-shadow(0 0 14px var(--mint-glow)); }
    .joystick.disabled { opacity: 0.32; pointer-events: none; filter: grayscale(0.5); }

    .joy-knob { position: absolute; width: 44px; height: 44px; margin: -22px 0 0 -22px; left: 50%; top: 50%; border-radius: 50%; background: radial-gradient(circle, rgba(0,255,159,0.38), rgba(0,40,28,0.55)); border: 1px solid rgba(0,255,159,0.5); box-shadow: 0 0 18px var(--mint-glow); pointer-events: none; opacity: 0.35; transition: 0.12s; }
    .joystick.active .joy-knob { opacity: 0.9; }

    .aux-btn { position: absolute; width: var(--aux); height: var(--aux); border-radius: 50%; border: 1px solid var(--glass-edge); background: var(--glass); color: var(--text); display: flex; align-items: center; justify-content: center; cursor: pointer; z-index: 2; backdrop-filter: blur(8px); transition: 0.1s; }
    .aux-btn:active { transform: scale(0.94); }
    .aux-btn.active { border-color: var(--mint); box-shadow: 0 0 14px var(--mint-glow); }
    .aux-btn.rth-active { border-color: var(--danger); box-shadow: 0 0 16px rgba(255,59,74,0.5); }
    .aux-btn.rth-active svg { stroke: var(--danger); }
    .aux-btn.rth-active svg path { fill: var(--danger); }

    .side-left .aux-auto { top: 0%; right: -22%; }
    .side-left .aux-rth  { bottom: 0%; right: -22%; }

    .side-right .aux-horn { top: 0%; left: -22%; }
    .side-right .aux-gyro { bottom: 0%; left: -22%; }

    .gear-dock { display: flex; flex-direction: column; align-items: center; width: min(96%, 520px); margin: 0 auto; }
    .gear-tab { width: 72px; height: 24px; border: 1px solid var(--glass-edge); border-bottom: none; border-radius: 12px 12px 0 0; background: var(--glass); color: var(--text); cursor: pointer; }
    .gear-panel { width: 100%; display: flex; justify-content: space-evenly; padding: 14px 28px; border-radius: 20px; background: var(--glass); border: 1px solid var(--glass-edge); backdrop-filter: blur(14px); }
    .gear-btn { width: 50px; height: 44px; border: none; background: transparent; color: rgba(255,255,255,0.55); font-family: var(--mono); font-size: 1.1rem; cursor: pointer; }
    .gear-btn.active { color: var(--mint); text-shadow: 0 0 16px var(--mint-glow); }

    /* Center cam panel */
    .cam-panel { grid-column: 2; grid-row: 2; display: flex; align-items: center; justify-content: center; }
    .cam-wrap { width: 100%; max-height: 100%; border-radius: 14px; overflow: hidden; border: 1px solid rgba(0,255,159,0.15); background: #000; position: relative; }
    .cam-wrap img { width: 100%; display: block; object-fit: contain; background: #0a0a0a; }
    .cam-ph { position: absolute; inset: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 6px; color: var(--muted); font-size: 0.7rem; letter-spacing: 0.1em; }
    .cam-ph svg { width: 32px; height: 32px; opacity: 0.25; }
    .cam-live { position: absolute; top: 6px; left: 8px; padding: 1px 8px; background: rgba(255,59,74,0.8); color: #fff; font-size: 0.55rem; font-weight: 700; letter-spacing: 1.5px; border-radius: 3px; opacity: 0; transition: opacity 0.4s; }
    .cam-live.on { opacity: 1; }
  </style>
</head>
<body>
  <div class="rotate-hint" aria-hidden="true"><span>Rotate to landscape</span></div>
  <div class="app">
    <div class="stage">

      <!-- Top-Left: Park, Hazard, Sync -->
      <div class="top-left">
        <button type="button" class="icon-btn" id="parkBtn"><span style="font-weight:800;font-size:1.2rem;">P</span></button>
        <button type="button" class="icon-btn" id="hazBtn"><svg viewBox="0 0 24 24"><path d="M12 3l9 16H3z"/><path d="M12 9v4"/><circle cx="12" cy="17" r="0.75" fill="currentColor" stroke="none"/></svg></button>
        <button type="button" class="pill-btn" id="syncPill">
          <svg viewBox="0 0 24 24"><path d="M7 7h5v5M17 17h-5v-5"/><path d="M7 12a5 5 0 0 1 5-5M17 12a5 5 0 0 1-5 5"/></svg>
          <span class="sync-s" id="syncS">S</span>
        </button>
      </div>

      <!-- Top-Center: Telemetry -->
      <div class="top-center">
        <div class="telemetry">
          <div class="metric"><div class="metric-label">BAT</div><div class="metric-val" id="batVal">&mdash;</div></div>
          <div class="metric"><div class="metric-label">SPD</div><div class="metric-val" id="spdVal">&mdash;</div></div>
          <div class="metric"><div class="metric-label">TILT</div><div class="metric-val" id="tiltVal">&mdash;</div></div>
        </div>
      </div>

      <!-- Top-Right: Headlight, Fullscreen -->
      <div class="top-right">
        <button type="button" class="round-btn" id="headlightBtn"><svg viewBox="0 0 24 24"><path d="M9 18h6"/><path d="M10 18V9a2 2 0 0 1 4 0v9"/><path d="M8 9c0-2.2 1.8-4 4-4s4 1.8 4 4"/><path d="M7 14l-2 1M17 14l2 1"/></svg></button>
        <button type="button" class="round-btn" id="fsBtn"><svg viewBox="0 0 24 24"><path d="M4 9V4h5M15 4h5v5M20 15v5h-5M9 20H4v-5"/></svg></button>
      </div>

      <!-- Left Side: Drive Joystick + Aux -->
      <div class="side side-left">
        <div class="joy-zone">
          <button type="button" class="aux-btn aux-auto" id="autoBtn"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"/><path d="M12 7V4M12 20v-3M7 12H4M20 12h-3"/><path d="M8 8l-1-2M16 16l1 2M16 8l1-2M8 16l-1 2"/></svg></button>
          <button type="button" class="aux-btn aux-rth" id="rthBtn">
            <svg viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z" fill="currentColor" stroke="none"/></svg>
          </button>
          <div class="joystick" id="driveJoy"><div class="joy-knob" id="driveKnob"></div></div>
        </div>
      </div>

      <!-- Center: Camera Feed -->
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

      <!-- Right Side: Steer Joystick + Aux -->
      <div class="side side-right">
        <div class="joy-zone">
          <button type="button" class="aux-btn aux-horn" id="hornBtn"><svg viewBox="0 0 24 24"><path d="M6 10h4l5-4v12l-5-4H6z"/><path d="M4 14v-2"/></svg></button>
          <button type="button" class="aux-btn aux-gyro" id="gyroBtn"><svg viewBox="0 0 24 24"><path d="M12 3a9 9 0 1 0 9 9"/><path d="M12 3v4M12 12h4"/></svg></button>
          <div class="joystick" id="steerJoy"><div class="joy-knob" id="steerKnob"></div></div>
        </div>
      </div>

    </div>

    <!-- Gear Dock -->
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

<script>
(function () {
  'use strict';

  /* ═══ CONFIG ══════════════════════════════════════════════ */
  var WS_URL = 'ws://' + location.hostname + ':81';
  var HB_MS  = 100;

  /* ═══ STATE ═══════════════════════════════════════════════ */
  var ws         = null;
  var connected  = false;
  var currentCmd = 'S';
  var rthActive  = false;
  var activeGear = '1';
  var gearOpen   = true;

  /* ═══ DOM ═════════════════════════════════════════════════ */
  var $ = function(id) { return document.getElementById(id); };
  var syncPill = $('syncPill'), syncS = $('syncS');
  var batVal = $('batVal'), spdVal = $('spdVal'), tiltVal = $('tiltVal');
  var rthBtn = $('rthBtn'), parkBtn = $('parkBtn'), fsBtn = $('fsBtn');
  var gearTab = $('gearTab'), gearPanel = $('gearPanel');
  var camImg = $('camStream'), camPh = $('camPh'), camLive = $('camLive');
  var driveJoyEl = $('driveJoy'), steerJoyEl = $('steerJoy');
  var gearBtns = document.querySelectorAll('.gear-btn');

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

      /* Telemetry: BAT:100,SPEED:59 */
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
        rthActive = true;
        rthBtn.classList.add('rth-active');
        driveJoyEl.classList.add('disabled');
        steerJoyEl.classList.add('disabled');
        return;
      }
      if (d === 'MODE:MANUAL') {
        rthActive = false;
        rthBtn.classList.remove('rth-active');
        driveJoyEl.classList.remove('disabled');
        steerJoyEl.classList.remove('disabled');
        return;
      }

      /* CAM telemetry — show TILT if present */
      if (d.indexOf('CAM:') === 0) {
        var camData = d.substring(4);
        if (camData.indexOf('TILT:') !== -1) {
          var m = camData.match(/TILT:([^\s,]+)/);
          if (m) tiltVal.textContent = m[1];
        }
      }
    };
  }

  function wsSend(c) { if (ws && ws.readyState === 1) ws.send(c); }

  /* ═══ HEARTBEAT ═══════════════════════════════════════════ */
  setInterval(function () { wsSend(currentCmd); }, HB_MS);

  /* ═══ CAMERA ══════════════════════════════════════════════ */
  camImg.onload = function () {
    camPh.style.display = 'none';
    camLive.classList.add('on');
  };
  camImg.onerror = function () {
    camPh.style.display = 'flex';
    camLive.classList.remove('on');
    setTimeout(function () {
      camImg.src = 'http://10.10.10.11:81/stream?t=' + Date.now();
    }, 4000);
  };

  /* ═══ JOYSTICK ════════════════════════════════════════════ */
  function Joystick(joyId, knobId, axis) {
    this.joy  = $(joyId);
    this.knob = $(knobId);
    this.axis = axis;
    this.on   = false;
    this.tid  = null;
    this.val  = 0;
    this.cx = 0; this.cy = 0; this.maxR = 0;
    this.rect = null;
    var self = this;

    /* ── Touch ── */
    this.joy.addEventListener('touchstart', function (e) {
      e.preventDefault();
      if (self.on) return;
      self._measure();
      var t = e.changedTouches[0];
      self.tid = t.identifier;
      self.on  = true;
      self.joy.classList.add('active');
      self._move(t.clientX, t.clientY);
    }, { passive: false });

    this.joy.addEventListener('touchmove', function (e) {
      e.preventDefault();
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === self.tid) {
          self._move(e.changedTouches[i].clientX, e.changedTouches[i].clientY);
          break;
        }
      }
    }, { passive: false });

    var endFn = function (e) {
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === self.tid) {
          self._release(); break;
        }
      }
    };
    this.joy.addEventListener('touchend', endFn);
    this.joy.addEventListener('touchcancel', endFn);

    /* ── Mouse (desktop) ── */
    this.joy.addEventListener('mousedown', function (e) {
      self._measure(); self.on = true;
      self.joy.classList.add('active');
      self._move(e.clientX, e.clientY);
    });
    document.addEventListener('mousemove', function (e) {
      if (self.on) self._move(e.clientX, e.clientY);
    });
    document.addEventListener('mouseup', function () {
      if (self.on) self._release();
    });
  }

  Joystick.prototype._measure = function () {
    this.rect = this.joy.getBoundingClientRect();
    this.cx   = this.rect.left + this.rect.width / 2;
    this.cy   = this.rect.top  + this.rect.height / 2;
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
    this.knob.style.left = '50%';
    this.knob.style.top  = '50%';
  };

  var joyDrive = new Joystick('driveJoy', 'driveKnob', 'y');
  var joySteer = new Joystick('steerJoy', 'steerKnob', 'x');

  /* ═══ COMMAND COMPUTATION ═════════════════════════════════ */
  function computeCmd() {
    var ly = joyDrive.val, rx = joySteer.val;
    var d = 'N', s = 'N';
    if (ly > 0) d = 'F'; else if (ly < 0) d = 'B';
    if (rx > 0) s = 'R'; else if (rx < 0) s = 'L';
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

  setInterval(function () { currentCmd = computeCmd(); }, 50);

  /* ═══ RTH BUTTON ══════════════════════════════════════════ */
  rthBtn.addEventListener('click', function () {
    if (!rthActive) { wsSend('M'); }
    else            { wsSend('m'); }
  });

  /* ═══ PARK (STOP) BUTTON ══════════════════════════════════ */
  parkBtn.addEventListener('click', function () {
    parkBtn.classList.toggle('active');
    if (parkBtn.classList.contains('active')) {
      driveJoyEl.classList.add('disabled');
      steerJoyEl.classList.add('disabled');
      joyDrive._release();
      joySteer._release();
      currentCmd = 'S';
      wsSend('S');
    } else {
      if (!rthActive) {
        driveJoyEl.classList.remove('disabled');
        steerJoyEl.classList.remove('disabled');
      }
    }
  });

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

  /* ═══ FULLSCREEN ══════════════════════════════════════════ */
  fsBtn.addEventListener('click', function () {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(function () {});
    } else {
      document.exitFullscreen();
    }
  });

  /* ═══ COSMETIC TOGGLE BUTTONS ═════════════════════════════ */
  ['headlightBtn', 'hazBtn', 'autoBtn', 'hornBtn', 'gyroBtn'].forEach(function (id) {
    var btn = $(id);
    if (btn) btn.addEventListener('click', function () { btn.classList.toggle('active'); });
  });

  /* ═══ BOOT ════════════════════════════════════════════════ */
  wsConnect();

})();
</script>
</body>
</html>
)HTML";

#endif /* INDEX_H */
