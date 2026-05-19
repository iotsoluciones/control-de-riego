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
  document.getElementById('temperatura').innerHTML = `${t} <span class="sensor-unit">°C</span>`;
  document.getElementById('humedad').innerHTML  = `${h} <span class="sensor-unit">%</span>`;
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

// ─────────────────────────────────────────────
//  ACTUALIZAR DATOS DINÁMICAMENTE
// ─────────────────────────────────────────────


async function actualizarDatos() {
    try {
        const response = await fetch('/estado');
        if (!response.ok) throw new Error('Error en respuesta');
        
        const data = await response.json();
        
        // Actualizar Humedad
        const humedadValue = parseFloat(data.humedad) || 0;
        document.getElementById('humedad').textContent = humedadValue.toFixed(1) + ' %';
        document.getElementById('humedadBar').style.width = humedadValue + '%';
        
        // Actualizar Temperatura
        const tempValue = parseFloat(data.temperatura) || 0;
        document.getElementById('temperatura').textContent = tempValue.toFixed(1) + ' °C';
        // Limitar barra de temperatura a 50°C
        document.getElementById('temperaturaBar').style.width = Math.min(tempValue / 50 * 100, 100) + '%';
        
        // Actualizar WiFi RSSI
        const rssiValue = parseInt(data.rssi) || 0;
        document.getElementById('rssi').textContent = rssiValue + ' dBm';
        
        // Actualizar indicador WiFi
        const wifiStatus = document.getElementById('wifiStatus');
        if (rssiValue > -70) {
            wifiStatus.classList.add('connected');
            wifiStatus.innerHTML = '<span class="dot"></span> Excelente';
        } else if (rssiValue > -80) {
            wifiStatus.classList.add('connected');
            wifiStatus.innerHTML = '<span class="dot"></span> Bueno';
        } else if (rssiValue > -90) {
            wifiStatus.classList.remove('connected');
            wifiStatus.innerHTML = '<span class="dot"></span> Regular';
        } else {
            wifiStatus.classList.remove('connected');
            wifiStatus.innerHTML = '<span class="dot"></span> Débil';
        }
        
        // Actualizar hora
        const now = new Date();
        document.getElementById('lastUpdate').textContent = 
            now.getHours().toString().padStart(2, '0') + ':' +
            now.getMinutes().toString().padStart(2, '0') + ':' +
            now.getSeconds().toString().padStart(2, '0');
            
    } catch (error) {
        console.error('Error actualizando datos:', error);
        document.getElementById('humedad').textContent = 'ERROR';
        document.getElementById('temperatura').textContent = 'ERROR';
    }
}

// ─────────────────────────────────────────────
//  REFRESCAR DATOS MANUALMENTE
// ─────────────────────────────────────────────

function refrescarDatos() {
    actualizarDatos();
}

// ─────────────────────────────────────────────
//  INICIALIZACIÓN
// ─────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', function() {
    // Actualizar datos inmediatamente
    actualizarDatos();
    
    // Actualizar cada 5 segundos
    setInterval(actualizarDatos, 5000);
});