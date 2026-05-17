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
