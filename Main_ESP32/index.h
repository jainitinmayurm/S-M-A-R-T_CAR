/*
 * =============================================================
 *  index.h — Jai RC Rover Web Control Interface
 *  Project by Jai Nitin
 * =============================================================
 *  PROGMEM HTML/CSS/JS served by the Main ESP32 SoftAP.
 *
 *  Features:
 *    - Dual touch-compatible joysticks (Drive Y-axis, Steer X-axis)
 *    - MJPEG camera feed from ESP32-CAM (10.10.10.11:81)
 *    - Real-time BAT / SPEED telemetry via WebSocket
 *    - RTH toggle button (sends 'M' / 'm')
 *    - Speed selector (sends '1'-'4')
 *    - 100 ms heartbeat ('S') with auto-reconnect
 * =============================================================
 */

#ifndef INDEX_H
#define INDEX_H

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
  <meta name="description" content="Jai RC Rover — Autonomous Return-to-Home Control Panel">
  <title>Jai RC Rover</title>
  <style>
    *, *::before, *::after { margin:0; padding:0; box-sizing:border-box; }

    :root {
      --bg:         #0b0d13;
      --bg-card:    rgba(255,255,255,0.04);
      --border:     rgba(255,255,255,0.08);
      --border-acc: rgba(0,229,255,0.25);
      --accent:     #00e5ff;
      --accent-dim: rgba(0,229,255,0.12);
      --accent-glo: rgba(0,229,255,0.35);
      --danger:     #ff3d00;
      --danger-dim: rgba(255,61,0,0.14);
      --success:    #00e676;
      --txt:        #ffffff;
      --txt2:       rgba(255,255,255,0.55);
      --txt3:       rgba(255,255,255,0.28);
      --rad:        14px;
      --rad-sm:     8px;
    }

    html, body {
      height:100%; width:100%;
      font-family: -apple-system,'Segoe UI',system-ui,Roboto,Helvetica,sans-serif;
      background: var(--bg);
      color: var(--txt);
      overflow: hidden;
      touch-action: none;
      -webkit-user-select:none; user-select:none;
    }

    body {
      display:flex; flex-direction:column;
      background:
        radial-gradient(ellipse at 15% 0%, rgba(0,229,255,0.06) 0%, transparent 55%),
        radial-gradient(ellipse at 85% 100%, rgba(124,77,255,0.04) 0%, transparent 55%),
        var(--bg);
    }

    /* ── Status Bar ─────────────────────────────── */
    .status-bar {
      display:flex; align-items:center; justify-content:space-between;
      padding:6px 14px; font-size:11px; color:var(--txt2);
      letter-spacing:.4px; flex-shrink:0;
    }
    .status-left { display:flex; align-items:center; gap:7px; }
    .status-dot {
      width:7px; height:7px; border-radius:50%;
      background:var(--danger); transition:background .3s, box-shadow .3s;
    }
    .status-dot.on {
      background:var(--success);
      box-shadow:0 0 8px rgba(0,230,118,0.55);
    }
    .mode-badge {
      padding:2px 10px; border-radius:20px;
      font-size:10px; font-weight:700;
      text-transform:uppercase; letter-spacing:1.2px;
      background:var(--accent-dim); color:var(--accent);
      border:1px solid rgba(0,229,255,0.2);
      transition:all .3s;
    }
    .mode-badge.rth {
      background:var(--danger-dim); color:var(--danger);
      border-color:rgba(255,61,0,0.3);
      animation:pulseRth 1.4s infinite;
    }
    @keyframes pulseRth { 0%,100%{opacity:1} 50%{opacity:.55} }

    /* ── Camera Section ─────────────────────────── */
    .cam-section {
      flex:1 1 0; min-height:0;
      padding:0 10px; display:flex;
      align-items:center; justify-content:center;
    }
    .cam-wrap {
      position:relative; width:100%; max-height:100%;
      border-radius:var(--rad); overflow:hidden;
      border:1px solid var(--border-acc);
      background:#000;
      box-shadow:0 0 40px rgba(0,229,255,0.04);
    }
    .cam-wrap img {
      width:100%; display:block; object-fit:contain;
      background:#000;
    }
    .cam-ph {
      position:absolute; inset:0;
      display:flex; flex-direction:column;
      align-items:center; justify-content:center;
      gap:6px; color:var(--txt3); font-size:12px;
      pointer-events:none;
    }
    .cam-ph svg { width:36px; height:36px; opacity:.25; }
    .live-tag {
      position:absolute; top:8px; left:8px;
      display:flex; align-items:center; gap:5px;
      padding:2px 9px; background:rgba(255,61,0,.85);
      color:#fff; font-size:9px; font-weight:700;
      letter-spacing:1.5px; border-radius:4px;
      text-transform:uppercase; opacity:0;
      transition:opacity .4s;
    }
    .live-tag.show { opacity:1; }
    .live-tag::before {
      content:''; width:5px; height:5px;
      background:#fff; border-radius:50%;
      animation:blink 1.1s infinite;
    }
    @keyframes blink { 0%,100%{opacity:1} 50%{opacity:.25} }

    /* ── Telemetry Bar ──────────────────────────── */
    .telem-bar {
      display:flex; gap:6px;
      padding:6px 10px; flex-shrink:0;
    }
    .telem-card {
      flex:1; display:flex; align-items:center; gap:10px;
      padding:8px 12px;
      background:var(--bg-card);
      border:1px solid var(--border);
      border-radius:var(--rad-sm);
      backdrop-filter:blur(12px);
    }
    .telem-icon { font-size:18px; line-height:1; }
    .telem-info { display:flex; flex-direction:column; }
    .telem-lbl {
      font-size:9px; color:var(--txt3);
      text-transform:uppercase; letter-spacing:1px;
    }
    .telem-val {
      font-size:20px; font-weight:700;
      font-variant-numeric:tabular-nums;
      color:var(--accent);
    }
    .telem-val .u {
      font-size:11px; font-weight:400;
      color:var(--txt2); margin-left:1px;
    }

    /* ── Controls Row ───────────────────────────── */
    .ctrl-row {
      display:flex; align-items:center; gap:6px;
      padding:2px 10px 6px; flex-shrink:0;
    }
    .spd-group { display:flex; gap:3px; }
    .spd-btn {
      width:38px; height:34px;
      border:1px solid var(--border);
      border-radius:var(--rad-sm);
      background:var(--bg-card);
      color:var(--txt2);
      font-size:13px; font-weight:600;
      cursor:pointer; transition:all .2s;
      -webkit-tap-highlight-color:transparent;
    }
    .spd-btn.on {
      background:var(--accent-dim); color:var(--accent);
      border-color:rgba(0,229,255,0.3);
      box-shadow:0 0 10px rgba(0,229,255,0.12);
    }
    .rth-btn {
      margin-left:auto;
      padding:7px 22px;
      border:1px solid rgba(255,61,0,0.3);
      border-radius:var(--rad-sm);
      background:var(--danger-dim);
      color:var(--danger);
      font-size:13px; font-weight:700;
      letter-spacing:1px; cursor:pointer;
      transition:all .3s;
      -webkit-tap-highlight-color:transparent;
    }
    .rth-btn.on {
      background:var(--danger); color:#fff;
      box-shadow:0 0 22px rgba(255,61,0,0.3);
      animation:pulseRth 1.4s infinite;
    }

    /* ── Joystick Area ──────────────────────────── */
    .joy-area {
      display:flex; justify-content:space-around;
      align-items:center;
      padding:4px 12px 14px; flex-shrink:0;
    }
    .joy-wrap {
      display:flex; flex-direction:column;
      align-items:center; gap:5px;
    }
    .joy-wrap canvas { touch-action:none; }
    .joy-lbl {
      font-size:9px; color:var(--txt3);
      text-transform:uppercase; letter-spacing:2px;
    }

    /* ── Command HUD ────────────────────────────── */
    .cmd-hud {
      position:fixed; bottom:6px; left:50%;
      transform:translateX(-50%);
      padding:1px 10px;
      background:rgba(0,0,0,.55);
      border:1px solid var(--border);
      border-radius:10px;
      font-size:10px; color:var(--txt3);
      font-family:'Courier New',monospace;
      pointer-events:none;
      transition:color .15s;
    }
    .cmd-hud.active { color:var(--accent); }

    /* ── Toast Notification ─────────────────────── */
    .toast {
      position:fixed; top:40px; left:50%;
      transform:translateX(-50%) translateY(-80px);
      padding:10px 24px;
      background:rgba(0,230,118,0.9);
      color:#000; font-size:13px; font-weight:700;
      border-radius:8px; letter-spacing:.5px;
      opacity:0; transition:all .5s cubic-bezier(.4,0,.2,1);
      pointer-events:none; z-index:100;
    }
    .toast.show {
      transform:translateX(-50%) translateY(0);
      opacity:1;
    }
  </style>
</head>
<body>

  <!-- Status Bar -->
  <div class="status-bar">
    <div class="status-left">
      <div class="status-dot" id="wsDot"></div>
      <span id="wsLabel">Disconnected</span>
    </div>
    <div class="mode-badge" id="modeBadge">MANUAL</div>
  </div>

  <!-- Camera Feed -->
  <div class="cam-section">
    <div class="cam-wrap">
      <img id="camStream" src="http://10.10.10.11:81/stream" alt="Camera Feed">
      <div class="cam-ph" id="camPh">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
          <path d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z"/>
        </svg>
        <span>Waiting for camera…</span>
      </div>
      <div class="live-tag" id="liveTag">LIVE</div>
    </div>
  </div>

  <!-- Telemetry -->
  <div class="telem-bar">
    <div class="telem-card">
      <div class="telem-icon">&#9889;</div>
      <div class="telem-info">
        <span class="telem-lbl">Battery</span>
        <span class="telem-val" id="batVal">--<span class="u">%</span></span>
      </div>
    </div>
    <div class="telem-card">
      <div class="telem-icon">&#127950;</div>
      <div class="telem-info">
        <span class="telem-lbl">Speed</span>
        <span class="telem-val" id="spdVal">--<span class="u">%</span></span>
      </div>
    </div>
  </div>

  <!-- Controls -->
  <div class="ctrl-row">
    <div class="spd-group">
      <button class="spd-btn" id="spd1" data-s="1">1</button>
      <button class="spd-btn" id="spd2" data-s="2">2</button>
      <button class="spd-btn on" id="spd3" data-s="3">3</button>
      <button class="spd-btn" id="spd4" data-s="4">4</button>
    </div>
    <button class="rth-btn" id="rthBtn">&#127968; RTH</button>
  </div>

  <!-- Joysticks -->
  <div class="joy-area">
    <div class="joy-wrap">
      <canvas id="joyLeft" width="140" height="140"></canvas>
      <span class="joy-lbl">Drive</span>
    </div>
    <div class="joy-wrap">
      <canvas id="joyRight" width="140" height="140"></canvas>
      <span class="joy-lbl">Steer</span>
    </div>
  </div>

  <!-- Command HUD -->
  <div class="cmd-hud" id="cmdHud">CMD: S</div>

  <!-- Toast -->
  <div class="toast" id="toast"></div>

<script>
(function(){
  'use strict';

  /* ── DOM refs ─────────────────────────────────── */
  var wsDot    = document.getElementById('wsDot');
  var wsLabel  = document.getElementById('wsLabel');
  var modeBadge= document.getElementById('modeBadge');
  var batVal   = document.getElementById('batVal');
  var spdVal   = document.getElementById('spdVal');
  var rthBtn   = document.getElementById('rthBtn');
  var cmdHud   = document.getElementById('cmdHud');
  var camImg   = document.getElementById('camStream');
  var camPh    = document.getElementById('camPh');
  var liveTag  = document.getElementById('liveTag');
  var toastEl  = document.getElementById('toast');

  /* ── State ────────────────────────────────────── */
  var ws         = null;
  var connected  = false;
  var rthActive  = false;
  var currentCmd = 'S';
  var activeSpd  = '3';
  var toastTimer = null;

  /* ── WebSocket ────────────────────────────────── */
  var WS_URL = 'ws://' + location.hostname + ':81';
  var reconnDelay = 1500;

  function wsConnect() {
    if (ws && (ws.readyState === 0 || ws.readyState === 1)) return;
    try { ws = new WebSocket(WS_URL); } catch(e) {
      setTimeout(wsConnect, reconnDelay); return;
    }

    ws.onopen = function() {
      connected = true;
      wsDot.classList.add('on');
      wsLabel.textContent = 'Connected';
      ws.send(activeSpd);
    };

    ws.onclose = function() {
      connected = false;
      wsDot.classList.remove('on');
      wsLabel.textContent = 'Reconnecting\u2026';
      setTimeout(wsConnect, reconnDelay);
    };

    ws.onerror = function() { ws.close(); };

    ws.onmessage = function(ev) {
      var d = ev.data;

      /* Telemetry: BAT:100,SPEED:59 */
      if (d.indexOf('BAT:') === 0) {
        var parts = d.split(',');
        for (var i = 0; i < parts.length; i++) {
          var kv = parts[i].split(':');
          if (kv[0] === 'BAT')   batVal.innerHTML = kv[1] + '<span class="u">%</span>';
          if (kv[0] === 'SPEED') spdVal.innerHTML = kv[1] + '<span class="u">%</span>';
        }
        return;
      }

      /* Mode changes */
      if (d === 'MODE:RTH') {
        rthActive = true;
        modeBadge.textContent = 'RTH';
        modeBadge.classList.add('rth');
        rthBtn.classList.add('on');
        return;
      }
      if (d === 'MODE:MANUAL') {
        rthActive = false;
        modeBadge.textContent = 'MANUAL';
        modeBadge.classList.remove('rth');
        rthBtn.classList.remove('on');
        return;
      }

      /* RTH complete */
      if (d === 'RTH:COMPLETE') {
        showToast('Home Reached!');
        return;
      }
    };
  }

  function wsSend(c) {
    if (ws && ws.readyState === 1) ws.send(c);
  }

  /* ── Heartbeat (100 ms) ───────────────────────── */
  setInterval(function() { wsSend(currentCmd); }, 100);

  /* ── Toast ────────────────────────────────────── */
  function showToast(msg) {
    toastEl.textContent = msg;
    toastEl.classList.add('show');
    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(function() {
      toastEl.classList.remove('show');
    }, 3000);
  }

  /* ── Camera Load Detection ────────────────────── */
  camImg.onload = function() {
    camPh.style.display = 'none';
    liveTag.classList.add('show');
  };
  camImg.onerror = function() {
    camPh.style.display = 'flex';
    liveTag.classList.remove('show');
    /* Retry stream every 4 s */
    setTimeout(function() {
      camImg.src = 'http://10.10.10.11:81/stream?t=' + Date.now();
    }, 4000);
  };

  /* ── Joystick Class ───────────────────────────── */
  function Joystick(canvasId, axis) {
    this.cvs  = document.getElementById(canvasId);
    this.ctx  = this.cvs.getContext('2d');
    this.axis = axis;                 /* 'y' or 'x' */
    this.W    = this.cvs.width;
    this.H    = this.cvs.height;
    this.cx   = this.W / 2;
    this.cy   = this.H / 2;
    this.outerR = this.W / 2 - 6;
    this.knobR  = 20;
    this.dz     = 0.18;              /* dead-zone fraction */
    this.kx   = this.cx;
    this.ky   = this.cy;
    this.val  = 0;                    /* -1 … +1 */
    this.on   = false;
    this.tid  = null;
    var self  = this;

    /* HiDPI sharpness */
    var dpr = window.devicePixelRatio || 1;
    if (dpr > 1) {
      this.cvs.width  = this.W * dpr;
      this.cvs.height = this.H * dpr;
      this.cvs.style.width  = this.W + 'px';
      this.cvs.style.height = this.H + 'px';
      this.ctx.scale(dpr, dpr);
    }

    /* Touch */
    this.cvs.addEventListener('touchstart', function(e) {
      e.preventDefault();
      if (self.on) return;
      var t = e.changedTouches[0];
      self.tid = t.identifier;
      self.on  = true;
      self._update(self._tpos(t));
    }, {passive:false});

    this.cvs.addEventListener('touchmove', function(e) {
      e.preventDefault();
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === self.tid) {
          self._update(self._tpos(e.changedTouches[i]));
          break;
        }
      }
    }, {passive:false});

    var endFn = function(e) {
      for (var i = 0; i < e.changedTouches.length; i++) {
        if (e.changedTouches[i].identifier === self.tid) {
          self.on = false; self.tid = null;
          self.kx = self.cx; self.ky = self.cy;
          self.val = 0; self._draw(); break;
        }
      }
    };
    this.cvs.addEventListener('touchend', endFn);
    this.cvs.addEventListener('touchcancel', endFn);

    /* Mouse (desktop testing) */
    this.cvs.addEventListener('mousedown', function(e) {
      self.on = true; self._update(self._mpos(e));
    });
    document.addEventListener('mousemove', function(e) {
      if (self.on) self._update(self._mpos(e));
    });
    document.addEventListener('mouseup', function() {
      if (!self.on) return;
      self.on = false;
      self.kx = self.cx; self.ky = self.cy;
      self.val = 0; self._draw();
    });

    this._draw();
  }

  Joystick.prototype._mpos = function(e) {
    var r = this.cvs.getBoundingClientRect();
    return { x: e.clientX - r.left, y: e.clientY - r.top };
  };
  Joystick.prototype._tpos = function(t) {
    var r = this.cvs.getBoundingClientRect();
    return { x: t.clientX - r.left, y: t.clientY - r.top };
  };

  Joystick.prototype._update = function(p) {
    var dx = p.x - this.cx;
    var dy = p.y - this.cy;
    if (this.axis === 'y') dx = 0;
    if (this.axis === 'x') dy = 0;
    var dist = Math.sqrt(dx*dx + dy*dy);
    var maxD = this.outerR - this.knobR;
    if (dist > maxD) { dx = dx*maxD/dist; dy = dy*maxD/dist; }
    this.kx = this.cx + dx;
    this.ky = this.cy + dy;
    this.val = (this.axis === 'y') ? (-dy / maxD) : (dx / maxD);
    if (Math.abs(this.val) < this.dz) this.val = 0;
    this._draw();
  };

  Joystick.prototype._draw = function() {
    var c = this.ctx, w = this.W, h = this.H;
    c.clearRect(0, 0, w, h);

    /* Outer ring */
    c.beginPath(); c.arc(this.cx, this.cy, this.outerR, 0, Math.PI*2);
    c.strokeStyle = 'rgba(255,255,255,0.07)'; c.lineWidth = 2; c.stroke();

    /* Track fill */
    c.beginPath(); c.arc(this.cx, this.cy, this.outerR - 1, 0, Math.PI*2);
    c.fillStyle = 'rgba(255,255,255,0.015)'; c.fill();

    /* Axis guide */
    c.beginPath();
    if (this.axis === 'y') {
      c.moveTo(this.cx, this.cy - this.outerR + 12);
      c.lineTo(this.cx, this.cy + this.outerR - 12);
    } else {
      c.moveTo(this.cx - this.outerR + 12, this.cy);
      c.lineTo(this.cx + this.outerR - 12, this.cy);
    }
    c.strokeStyle = 'rgba(255,255,255,0.045)'; c.lineWidth = 1; c.stroke();

    /* Directional arrows */
    c.save();
    c.strokeStyle = 'rgba(255,255,255,0.08)'; c.lineWidth = 1.2;
    c.lineCap = 'round'; c.lineJoin = 'round';
    if (this.axis === 'y') {
      /* Up arrow */
      var at = this.cy - this.outerR + 16;
      c.beginPath(); c.moveTo(this.cx-5, at+5); c.lineTo(this.cx, at); c.lineTo(this.cx+5, at+5); c.stroke();
      /* Down arrow */
      var ab = this.cy + this.outerR - 16;
      c.beginPath(); c.moveTo(this.cx-5, ab-5); c.lineTo(this.cx, ab); c.lineTo(this.cx+5, ab-5); c.stroke();
    } else {
      /* Left arrow */
      var al = this.cx - this.outerR + 16;
      c.beginPath(); c.moveTo(al+5, this.cy-5); c.lineTo(al, this.cy); c.lineTo(al+5, this.cy+5); c.stroke();
      /* Right arrow */
      var ar = this.cx + this.outerR - 16;
      c.beginPath(); c.moveTo(ar-5, this.cy-5); c.lineTo(ar, this.cy); c.lineTo(ar-5, this.cy+5); c.stroke();
    }
    c.restore();

    /* Glow when active */
    if (this.on) {
      c.beginPath(); c.arc(this.kx, this.ky, this.knobR + 10, 0, Math.PI*2);
      c.fillStyle = 'rgba(0,229,255,0.12)'; c.fill();
    }

    /* Knob */
    var g = c.createRadialGradient(this.kx - 2, this.ky - 2, 0, this.kx, this.ky, this.knobR);
    if (this.on) {
      g.addColorStop(0, 'rgba(0,229,255,0.95)');
      g.addColorStop(1, 'rgba(0,180,220,0.65)');
    } else {
      g.addColorStop(0, 'rgba(190,195,210,0.55)');
      g.addColorStop(1, 'rgba(140,145,160,0.35)');
    }
    c.beginPath(); c.arc(this.kx, this.ky, this.knobR, 0, Math.PI*2);
    c.fillStyle = g; c.fill();
    c.strokeStyle = this.on ? 'rgba(0,229,255,0.55)' : 'rgba(255,255,255,0.12)';
    c.lineWidth = 1.5; c.stroke();
  };

  /* ── Instantiate Joysticks ────────────────────── */
  var joyL = new Joystick('joyLeft',  'y');
  var joyR = new Joystick('joyRight', 'x');

  /* ── Command Computation ──────────────────────── */
  function computeCmd() {
    var ly = joyL.val;   /* positive = forward  */
    var rx = joyR.val;   /* positive = right    */
    var d = 'N', s = 'N';
    if (ly > 0) d = 'F'; else if (ly < 0) d = 'B';
    if (rx > 0) s = 'R'; else if (rx < 0) s = 'L';

    if (d==='F' && s==='L') return 'G';
    if (d==='F' && s==='R') return 'H';
    if (d==='B' && s==='L') return 'I';
    if (d==='B' && s==='R') return 'J';
    if (d==='F') return 'F';
    if (d==='B') return 'B';
    if (s==='L') return 'L';
    if (s==='R') return 'R';
    return 'S';
  }

  /* 50 ms poll to update currentCmd & HUD */
  setInterval(function() {
    currentCmd = computeCmd();
    cmdHud.textContent = 'CMD: ' + currentCmd;
    cmdHud.classList.toggle('active', currentCmd !== 'S');
  }, 50);

  /* ── Speed Buttons ────────────────────────────── */
  var spdBtns = document.querySelectorAll('.spd-btn');
  for (var i = 0; i < spdBtns.length; i++) {
    spdBtns[i].addEventListener('click', (function(btn) {
      return function() {
        activeSpd = btn.getAttribute('data-s');
        for (var j = 0; j < spdBtns.length; j++)
          spdBtns[j].classList.toggle('on', spdBtns[j] === btn);
        wsSend(activeSpd);
      };
    })(spdBtns[i]));
  }

  /* ── RTH Button ───────────────────────────────── */
  rthBtn.addEventListener('click', function() {
    if (!rthActive) {
      wsSend('M');
      rthActive = true;
      rthBtn.classList.add('on');
      modeBadge.textContent = 'RTH';
      modeBadge.classList.add('rth');
    } else {
      wsSend('m');
      rthActive = false;
      rthBtn.classList.remove('on');
      modeBadge.textContent = 'MANUAL';
      modeBadge.classList.remove('rth');
    }
  });

  /* ── Bootstrap ────────────────────────────────── */
  wsConnect();

})();
</script>
</body>
</html>
)rawliteral";

#endif /* INDEX_H */
