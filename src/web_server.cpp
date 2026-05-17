#include <WiFi.h>
#include <LittleFS.h>

#include "web_server.h"
#include "variables.h"
#include "telegram.h"



WebServer server(80);

// ─────────────────────────────────────────────
//  INICIALIZAR LITTLEFS
// ─────────────────────────────────────────────

bool inicializarLittleFS() {
    if (!LittleFS.begin(true)) {
        Serial.println("❌ Error montando LittleFS");
        return false;
    }
    Serial.println("✅ LittleFS montado correctamente");
    return true;
}

// ─────────────────────────────────────────────
//  HANDLER: Servir archivos estáticos
// ─────────────────────────────────────────────

void handleFileRequest(String path){

    Serial.print("📄 Solicitado: ");
    Serial.println(path);

    if(path == "/"){
        path = "/index.html";
    }

    String contentType="text/plain";

    if(path.endsWith(".html")) contentType="text/html";
    else if(path.endsWith(".css")) contentType="text/css";
    else if(path.endsWith(".js")) contentType="application/javascript";
    else if(path.endsWith(".json")) contentType="application/json";
    else if(path.endsWith(".png")) contentType="image/png";

    if(LittleFS.exists(path)){

        File file=LittleFS.open(path,"r");
        server.streamFile(file,contentType);
        file.close();

        Serial.println("✅ Archivo enviado");
    }
    else{

        Serial.print("❌ Archivo no encontrado: ");
        Serial.println(path);

        server.send(404,"text/plain","Archivo no encontrado");
    }
}
void handleEstado() {

    String json = "{";

    json += "\"humedad\":" + String(humedad,1);
    json += ",\"temperatura\":" + String(temperatura,1);
    json += ",\"rssi\":" + String(WiFi.RSSI());

    json += ",\"ip\":\"";
    json += WiFi.localIP().toString();
    json += "\"";

    json += ",\"heap\":";
    json += String(ESP.getFreeHeap());

    json += ",\"tanque\":";
    json += (tanqueNecesitaAgua ? "true" : "false");

    json += "}";

    server.send(200,"application/json",json);
}
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
