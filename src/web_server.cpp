#include "web_server.h"
#include "variables.h"
#include "telegram.h"

#include <WiFi.h>
#include <LittleFS.h>

WebServer server(80);

// ─────────────────────────────────────────────
//  INICIALIZAR LITTLEFS
// ─────────────────────────────────────────────

bool inicializarLittleFS() {
    if (!LittleFS.begin()) {
        Serial.println("❌ Error montando LittleFS");
        return false;
    }
    Serial.println("✅ LittleFS montado correctamente");
    return true;
}

// ─────────────────────────────────────────────
//  HANDLER: Servir archivos estáticos
// ─────────────────────────────────────────────

void handleFileRequest(String path) {
    Serial.print("📄 Solicitado: ");
    Serial.println(path);
    
    // Ruta por defecto
    if (path == "/") {
        path = "/index.html";
    }
    
    // Determinar tipo de contenido
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".json")) contentType = "application/json";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".jpg")) contentType = "image/jpeg";
    else if (path.endsWith(".svg")) contentType = "image/svg+xml";
    
    // Intentar abrir archivo
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        Serial.print("✅ Archivo enviado: ");
        Serial.println(path);
    } else {
        Serial.print("❌ Archivo no encontrado: ");
        Serial.println(path);
        server.send(404, "text/plain", "Archivo no encontrado");
    }
}

// ─────────────────────────────────────────────
//  HANDLER: API /estado (JSON)
// ─────────────────────────────────────────────

void handleEstado() {
    String json = R"rawliteral({
    "humedad": )rawliteral";
    
    json += String(humedad, 1);
    json += R"rawliteral(,
    "temperatura": )rawliteral";
    
    json += String(temperatura, 1);
    json += R"rawliteral(,
    "rssi": )rawliteral";
    
    json += String(WiFi.RSSI());
    json += R"rawliteral(
})rawliteral";
    
    server.send(200, "application/json", json);
}

// ─────────────────────────────────────────────
//  INIT DEL SERVIDOR WEB
// ─────────────────────────────────────────────

void iniciarServidorWeb() {
    
    // Inicializar LittleFS
    if (!inicializarLittleFS()) {
        Serial.println("⚠️  Continuando sin LittleFS");
    }
    
    // --- Rutas API ---
    server.on("/estado", HTTP_GET, handleEstado);
    
    // --- Servidor de archivos estáticos (catch-all) ---
    server.onNotFound([]() {
        handleFileRequest(server.uri());
    });
    
    // --- Configurar OTA ---
    ElegantOTA.begin(&server);
    
    // --- Callbacks OTA ---
    ElegantOTA.onStart([]() {
        OTAEnCurso = true;
        Serial.println("📡 OTA iniciada");
        enviarATodos("🔄 Actualización OTA iniciada");
    });

    ElegantOTA.onEnd([](bool success) {
        OTAEnCurso = false;
        if (success) {
            Serial.println("✅ OTA completada correctamente");
        } else {
            Serial.println("❌ Error durante OTA");
        }
    });
    
    // --- Iniciar servidor ---
    server.begin();
    Serial.println("✅ Servidor web iniciado en puerto 80");
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────

void loopServidorWeb() {
    server.handleClient();
}
