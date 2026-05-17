#pragma once

#include <WebServer.h>
#include <ElegantOTA.h>

// ─────────────────────────────────────────────
//  INSTANCIA GLOBAL DEL SERVIDOR
// ─────────────────────────────────────────────

extern WebServer server;

// ─────────────────────────────────────────────
//  FUNCIONES PÚBLICAS
// ─────────────────────────────────────────────

/**
 * @brief Inicializa el servidor web con LittleFS
 * Configura rutas, OTA, callbacks y comienza a escuchar
 */
void iniciarServidorWeb();

/**
 * @brief Loop del servidor web
 * Debe llamarse en cada iteración del loop principal
 * Maneja las peticiones HTTP y OTA
 */
void loopServidorWeb();

// ─────────────────────────────────────────────
//  FUNCIONES INTERNAS (helper)
// ─────────────────────────────────────────────

/**
 * @brief Inicializa el filesystem LittleFS
 * @return true si se montó correctamente, false en caso contrario
 */
bool inicializarLittleFS();

/**
 * @brief Maneja peticiones de archivos estáticos
 * @param path Ruta del archivo solicitado (ej: "/index.html", "/styles.css")
 */
void handleFileRequest(String path);

/**
 * @brief API que devuelve datos en tiempo real (JSON)
 * Responde en la ruta GET /estado con datos de sensores
 */
void handleEstado();

// ─────────────────────────────────────────────
//  CONSTANTES
// ─────────────────────────────────────────────

#define WEB_PORT 80
#define UPLOAD_CHUNK_SIZE 8192
