#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "variables.h"
#include "clima.h"

bool obtenerCiudadPorCoordenadas(float lat, float lon){

WiFiClientSecure clientHTTP;
clientHTTP.setInsecure();

HTTPClient https;


  String url = "https://nominatim.openstreetmap.org/reverse?lat=" 
               + String(lat,6) + 
               "&lon=" + String(lon,6) + 
               "&format=json";

  Serial.println("🌍 Reverse geocoding...");
  Serial.println(url);

  if(!https.begin(url)){
    Serial.println("❌ begin() falló");
    return false;
  }

  https.addHeader("User-Agent", "ESP32");

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

      JsonDocument doc;
    if(deserializeJson(doc, payload)){
      Serial.println("❌ Error JSON");
      https.end();
      return false;
    }

    if(doc["display_name"]){

      ciudad = doc["display_name"].as<String>();

      prefs.begin("config", false);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ Ciudad detectada:");
      Serial.println(ciudad);

      https.end();
      return true;

    }

  }

  https.end();
  return false;
}

void consultarClima(){


  if(WiFi.status() != WL_CONNECTED){
    Serial.println("❌ Sin WiFi");
    return;
  }

  
WiFiClientSecure clientHTTP;
clientHTTP.setInsecure();

HTTPClient https;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" 
               + String(lat,6) + 
               "&longitude=" + String(lon,6) + 
               "&hourly=precipitation_probability&timezone=auto";

  Serial.println("🌍 Consultando clima...");
  Serial.println(url);

  if(!https.begin(url)){
    Serial.println("❌ begin() falló");
    return;
  }

  int httpCode = https.GET();

  Serial.println("HTTP CODE: " + String(httpCode));

  if(httpCode > 0){

    String payload = https.getString();
    Serial.println("JSON recibido:");
    Serial.println(payload);
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.print("❌ Error JSON: ");
      Serial.println(error.c_str());
      return;
    }

    struct tm timeinfo;
    int horaActual = 0;

    if(getLocalTime(&timeinfo)){
      horaActual = timeinfo.tm_hour;
    } else{
      Serial.println("Hora no disponible");
    horaActual = 0 ;
  }

    JsonArray lluvia = doc["hourly"]["precipitation_probability"];

    if(!lluvia.isNull() && horaActual < lluvia.size()){

      float nuevaLluvia = lluvia[horaActual];

// suavizado (mezcla valor anterior + nuevo)
probabilidadLluviaSuavizada = (probabilidadLluviaSuavizada * 0.7) + (nuevaLluvia * 0.3);

// valor final que usa el sistema
probabilidadLluvia = probabilidadLluviaSuavizada;

    }else{
      Serial.println("⚠️ No se pudo leer lluvia");
      probabilidadLluvia = 0;
    }

    Serial.println("🕒 Hora: " + String(horaActual));
    Serial.println("🌧️ Prob lluvia: " + String(probabilidadLluvia));

  }else{
    Serial.println("❌ Error HTTP");
  }
  
  Serial.println("Heap libre: " + String(ESP.getFreeHeap()));

  https.end();
}

bool buscarCoordenadasOSM(String ciudadBusqueda){
WiFiClientSecure clientHTTP;
clientHTTP.setInsecure();

HTTPClient https;

  ciudadBusqueda.trim();
  ciudadBusqueda.replace(" ", "%20");

  String url = "https://nominatim.openstreetmap.org/search?q=" 
               + ciudadBusqueda + 
               "&format=json&limit=1";

  Serial.println("🌍 OSM fallback...");
  Serial.println(url);

  if(!https.begin(url)){
    Serial.println("❌ begin() OSM falló");
    return false;
  }

  https.addHeader("User-Agent", "ESP32");

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

     JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.println("❌ JSON OSM error");
      https.end();
      return false;
    }

    if(doc.size() > 0){

      lat = String((const char*)doc[0]["lat"]).toFloat();
      lon = String((const char*)doc[0]["lon"]).toFloat();

      ciudad = doc[0]["display_name"].as<String>();

      prefs.begin("config", false);
      prefs.putFloat("lat", lat);
      prefs.putFloat("lon", lon);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ OSM encontró:");
      Serial.println(ciudad);

      consultarClima();

      https.end();
      return true;   // ENCONTRÓ

    }else{
      Serial.println("❌ OSM no encontró nada");
    }

  }else{
    Serial.println("❌ HTTP OSM error");
  }

  https.end();
  return false;   // ❌ NO encontró
}

bool buscarCoordenadas(String ciudadBusqueda){

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("❌ Sin WiFi");
    return false;
  }
WiFiClientSecure clientHTTP;
clientHTTP.setInsecure();

HTTPClient https;

  ciudadBusqueda.trim();
  ciudadBusqueda.replace(" ", "%20");

  String url = "https://geocoding-api.open-meteo.com/v1/search?name=" 
               + ciudadBusqueda + 
               "&count=3&language=es&format=json";

  Serial.println("🌍 Buscando ciudad...");
  Serial.println(url);

  if(!https.begin(url)){
    Serial.println("❌ begin() falló");
    return false;
  }

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

      JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.println("❌ Error JSON");
      https.end();
      return false;
    }

    if(doc["results"].size() > 0){

      lat = doc["results"][0]["latitude"];
      lon = doc["results"][0]["longitude"];

      String nombre = doc["results"][0]["name"].as<String>();
      String pais = doc["results"][0]["country"].as<String>();

      ciudad = nombre + ", " + pais;

      prefs.begin("config", false);
      prefs.putFloat("lat", lat);
      prefs.putFloat("lon", lon);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ Ciudad encontrada:");
      Serial.println(ciudad);

      consultarClima();

      https.end();
      return true;   // ENCONTRÓ

    }else{

      Serial.println("⚠️ Open-Meteo falló → probando OSM");
      https.end();

      return buscarCoordenadasOSM(ciudadBusqueda);  // si falla
    }

  }else{
    Serial.println("❌ HTTP error");
  }

  https.end();
  return false;
}
