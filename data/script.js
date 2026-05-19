// ── RELOJ EN TIEMPO REAL ──
function tick() {
  const now = new Date();
  document.getElementById('clock').textContent =
    now.toLocaleTimeString('es-AR', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}
tick();
setInterval(tick, 1000);

// ── SIMULACIÓN DE SENSORES (reemplazar con fetch a ESP32) ──
setInterval(() => {
  const t = (24 + Math.random() * 2 - 1).toFixed(1);
  const h = Math.round(58 + Math.random() * 6 - 2);
  document.getElementById('tempVal').innerHTML = `${t} <span class="sensor-unit">°C</span>`;
  document.getElementById('humVal').innerHTML  = `${h} <span class="sensor-unit">%</span>`;
}, 5000);

// ── NAVEGACIÓN SIDEBAR ──
function setNav(el) {
  document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
  el.classList.add('active');
}

// ── TOAST NOTIFICATION ──
let toastTimer;
function toast(msg) {
  const t = document.getElementById('toast');
  document.getElementById('toast-msg').textContent = msg;
  t.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove('show'), 2800);
}

// ── ZONAS DE RIEGO ──
const zones = [
  { number: 1, time: '09:00', duration: 5 },
  { number: 2, time: '12:00', duration: 5 },
  { number: 3, time: '12:00', duration: 5 },
];
const badgeColors = ['z1', 'z2', 'z3'];

function renderZones() {
  const tbody = document.getElementById('zonesBody');
  tbody.innerHTML = '';

  zones.forEach((zone, index) => {
    const color = badgeColors[index % badgeColors.length];
    const row = document.createElement('tr');
    row.innerHTML = `
      <td><span class="zone-badge ${color}">ZONE ${zone.number}</span></td>
      <td style="font-family:var(--mono);font-size:12px">${zone.time}</td>
      <td style="font-family:var(--mono);font-size:12px">${zone.duration} min</td>
      <td><div class="action-btns">
        <button class="icon-btn" title="Activar" onclick="toast('ZONE ${zone.number} activada')">▶</button>
        <button class="icon-btn danger" title="Eliminar" onclick="deleteZone(${index})">✕</button>
      </div></td>
    `;
    tbody.appendChild(row);
  });
}

function addZone() {
  const number   = document.getElementById('newZone').value.trim();
  const time     = document.getElementById('newTime').value;
  const duration = document.getElementById('newDuration').value;

  if (!number || !time || !duration) {
    toast('⚠ Completá todos los campos');
    return;
  }

  zones.push({ number: parseInt(number), time, duration: parseInt(duration) });
  renderZones();

  document.getElementById('newZone').value    = '';
  document.getElementById('newTime').value    = '';
  document.getElementById('newDuration').value = '';

  toast(`ZONE ${number} agregada correctamente`);
}

function deleteZone(index) {
  const num = zones[index].number;
  zones.splice(index, 1);
  renderZones();
  toast(`ZONE ${num} eliminada`);
}

// ── INIT ──
renderZones();
