#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <ElegantOTA.h>

#define WEB_PORT 80
#define UPLOAD_CHUNK_SIZE 8192

extern WebServer server;

void iniciarServidorWeb();
void loopServidorWeb();
bool inicializarLittleFS();
void handleFileRequest(String path);
void handleEstado();


#endif