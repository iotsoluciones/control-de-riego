# 📁 LittleFS - Estructura de Archivos Web

## Archivos Creados

```
data/
├── index.html      ← Página principal (HTML puro)
├── styles.css      ← Estilos CSS modernos
└── script.js       ← JavaScript para actualización dinámica
```

## Cómo Funciona

### 1️⃣ **Flash a ESP32**
PlatformIO subirá automáticamente los archivos de `data/` a LittleFS:
```bash
# Terminal: "Build File System Image" en PlatformIO
# Luego: "Upload File System Image"
```

### 2️⃣ **En el Navegador**
- Accede a: `http://<IP-ESP32>/`
- Se sirve `/index.html` automáticamente
- Los estilos se cargan desde `/styles.css`
- El script se carga desde `/script.js`

### 3️⃣ **Datos Dinámicos**
El JavaScript hace peticiones JSON a `/estado`:
```javascript
// script.js hace cada 5 segundos:
fetch('/estado')  →  web_server.cpp responde con JSON
```

## API Disponible

### GET `/` 
Devuelve `/index.html` (página principal)

### GET `/styles.css`
Devuelve los estilos CSS

### GET `/script.js`
Devuelve el código JavaScript

### GET `/estado`
Devuelve JSON con datos en tiempo real:
```json
{
    "humedad": 45.2,
    "temperatura": 28.5,
    "rssi": -65
}
```

## Modificar Web

### ✏️ HTML
Editar `data/index.html` → Cambiar estructura

### 🎨 CSS
Editar `data/styles.css` → Cambiar colores, layout, etc

### ⚙️ JavaScript
Editar `data/script.js` → Cambiar lógica de actualización

## Upload del Filesystem

Con cada cambio en `/data/`, necesitas:
1. En VS Code → PlatformIO → project → Platform → Upload File System Image
2. O por terminal: `pio run -t uploadfs`

## Ventajas

✅ Archivos completamente separados (HTML, CSS, JS)
✅ Interfaz moderna y responsive
✅ Actualización en tiempo real sin refrescar página
✅ Fácil de mantener y editar
✅ Bajo consumo de memoria del ESP32

