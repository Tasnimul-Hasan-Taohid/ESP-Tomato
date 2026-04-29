/* ================================================================
   ESP32 Tomato — Frontend App
   WebSocket live stats, WiFi scan, relay API calls, toast system
   ================================================================ */

'use strict';

// ── WebSocket live stats ─────────────────────────────────────
let ws = null;
let wsRetries = 0;
const WS_MAX_RETRY = 10;
const WS_RETRY_DELAY = 3000;

function connectWS() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(`${proto}://${location.host}/ws`);

  ws.onopen = () => {
    wsRetries = 0;
    console.log('[WS] Connected');
    setLiveIndicator(true);
    // Keep-alive ping
    setInterval(() => { if (ws.readyState === WebSocket.OPEN) ws.send('ping'); }, 25000);
  };

  ws.onclose = () => {
    setLiveIndicator(false);
    if (wsRetries < WS_MAX_RETRY) {
      wsRetries++;
      console.log(`[WS] Reconnecting in ${WS_RETRY_DELAY}ms (attempt ${wsRetries})`);
      setTimeout(connectWS, WS_RETRY_DELAY);
    }
  };

  ws.onerror = () => {
    console.warn('[WS] Error — will retry');
    ws.close();
  };

  ws.onmessage = (evt) => {
    try {
      const msg = JSON.parse(evt.data);
      if (msg.type === 'stats') handleStatsUpdate(msg);
    } catch (e) {
      console.warn('[WS] Bad JSON:', e);
    }
  };
}

function setLiveIndicator(live) {
  const dot = document.querySelector('.live-dot');
  if (!dot) return;
  dot.style.background = live ? 'var(--green)' : 'var(--tomato)';
}

function handleStatsUpdate(msg) {
  // Update any element with data-stat="fieldName"
  const d = msg.data || {};
  const updates = {
    'heap':      d.freeHeap ? `${Math.round(d.freeHeap / 1024)} KB` : null,
    'heapPct':   d.heapUsedPct != null ? `${d.heapUsedPct}%` : null,
    'temp':      d.cpuTempC != null ? `${d.cpuTempC.toFixed(1)} °C` : null,
    'rssi':      msg.rssi != null ? `${msg.rssi} dBm` : null,
    'ping':      d.pingMs != null && d.pingMs >= 0 ? `${d.pingMs.toFixed(1)} ms` : '—',
    'inet':      msg.internet != null ? (msg.internet ? 'Online' : 'Offline') : null,
  };
  for (const [id, val] of Object.entries(updates)) {
    if (val !== null) updateEl(id, val);
  }

  // Progress bar for heap
  const pb = document.querySelector('.heap-bar .progress-fill');
  if (pb && d.heapUsedPct != null) {
    pb.style.width = d.heapUsedPct + '%';
    pb.style.background = d.heapUsedPct > 80 ? 'var(--tomato)' :
                          d.heapUsedPct > 60 ? 'var(--yellow)' : 'var(--green)';
  }
}

function updateEl(id, val) {
  const el = document.getElementById(id);
  if (el) el.textContent = val;
}

// ── WiFi Scan ────────────────────────────────────────────────
async function scanWifi() {
  const container = document.getElementById('scan-results');
  if (!container) return;
  container.innerHTML = '<p class="dim">Scanning... (takes ~5s)</p>';

  try {
    const res = await fetch('/api/scan');
    if (!res.ok) throw new Error('Scan failed');
    const networks = await res.json();

    if (networks.length === 0) {
      container.innerHTML = '<p class="dim">No networks found.</p>';
      return;
    }

    // Sort by RSSI descending
    networks.sort((a, b) => b.rssi - a.rssi);

    const list = document.createElement('div');
    list.className = 'scan-list';

    networks.forEach(net => {
      const item = document.createElement('div');
      item.className = 'scan-item';
      item.innerHTML = `
        <span class="ssid">${escapeHTML(net.ssid)}</span>
        <span class="meta">${net.rssi} dBm ${net.enc ? '🔒' : '🔓'}</span>
      `;
      item.addEventListener('click', () => {
        const ssidInput = document.querySelector('input[name="ssid"]');
        if (ssidInput) ssidInput.value = net.ssid;
        showToast(`Selected: ${net.ssid}`);
      });
      list.appendChild(item);
    });

    container.innerHTML = '';
    container.appendChild(list);
  } catch (err) {
    container.innerHTML = `<p class="err">Scan failed: ${err.message}</p>`;
  }
}

// ── Relay control (AJAX — no page reload) ────────────────────
async function setRelay(num, state) {
  try {
    const form = new FormData();
    form.append('state', state ? 'on' : 'off');
    const res = await fetch(`/api/relay/${num}`, { method: 'POST', body: form });
    if (!res.ok) throw new Error('Request failed');
    const data = await res.json();
    showToast(`Relay ${num} → ${state ? 'ON' : 'OFF'}`);

    // Update state indicators on page
    const indicator = document.querySelector(`[data-relay="${num}"]`);
    if (indicator) {
      indicator.textContent = state ? 'ON' : 'OFF';
      indicator.className   = state ? 'ok' : 'off';
    }
  } catch (err) {
    showToast(`Error: ${err.message}`, true);
  }
}

// ── Load logs ────────────────────────────────────────────────
async function loadLogs() {
  const container = document.getElementById('log-entries');
  if (!container) return;

  try {
    const res  = await fetch('/api/logs');
    const logs = await res.json();
    container.innerHTML = '';
    if (logs.length === 0) {
      container.innerHTML = '<p class="dim">No log entries yet.</p>';
      return;
    }
    // Newest first
    [...logs].reverse().forEach(entry => {
      const div = document.createElement('div');
      div.className = 'log-entry';
      div.textContent = entry;
      container.appendChild(div);
    });
  } catch (err) {
    if (container) container.innerHTML = `<p class="err">Failed to load logs: ${err.message}</p>`;
  }
}

// ── Toast notifications ───────────────────────────────────────
let toastTimer = null;

function showToast(msg, isError = false) {
  let toast = document.querySelector('.toast');
  if (!toast) {
    toast = document.createElement('div');
    toast.className = 'toast';
    document.body.appendChild(toast);
  }
  toast.textContent = msg;
  toast.style.borderColor = isError ? 'var(--tomato)' : 'var(--green)';
  toast.classList.add('show');

  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove('show'), 3000);
}

// ── Helpers ───────────────────────────────────────────────────
function escapeHTML(str) {
  return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

function formatUptime(seconds) {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m ${s}s`;
  return `${m}m ${s}s`;
}

// ── Confirm before destructive actions ────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  // Wire up any async relay buttons
  document.querySelectorAll('[data-relay-btn]').forEach(btn => {
    btn.addEventListener('click', () => {
      const num   = btn.dataset.relayBtn;
      const state = btn.dataset.state === 'on';
      setRelay(num, state);
    });
  });

  // Start WebSocket only on dashboard (where live updates matter)
  if (document.querySelector('.grid')) {
    connectWS();
  }

  // Add live dot to nav on pages with WebSocket
  const brand = document.querySelector('.brand');
  if (brand && document.querySelector('.grid')) {
    const dot = document.createElement('span');
    dot.className = 'live-dot';
    brand.prepend(dot);
  }

  // Auto-load logs on logs page
  if (document.getElementById('log-entries')) {
    loadLogs();
    setInterval(loadLogs, 10000);
  }

  // Add uptime countdown updater
  setInterval(() => {
    const el = document.querySelector('[data-uptime]');
    if (el) {
      const cur = parseInt(el.dataset.uptime) + 1;
      el.dataset.uptime = cur;
      el.textContent = formatUptime(cur);
    }
  }, 1000);
});
