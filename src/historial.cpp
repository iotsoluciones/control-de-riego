#include "historial.h"
#include "variables.h"


void guardarEvento(String texto){
  struct tm timeinfo;
  String hora = "";

  if(getLocalTime(&timeinfo)){
    char buffer[10];

    sprintf(buffer,"%02d:%02d:%02d",
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec);

    hora = String(buffer);
  }

  String evento = "🕒 " + hora + " | " + texto;

  eventos[indiceEvento] = evento;

  prefs.begin("eventos", false);

  prefs.putString(("e"+String(indiceEvento)).c_str(), evento);

  indiceEvento++;

  if(indiceEvento >= MAX_EVENTOS){
    indiceEvento = 0;
  }

  prefs.putInt("indice", indiceEvento);

  int total = prefs.getInt("total", 0);

  if(total < MAX_EVENTOS){
    total++;
  }

  prefs.putInt("total", total);

  prefs.end();

  yield();
  delay(50);
}


void borrarHistorial() {

  prefs.begin("eventos", false); 
  prefs.clear();
  prefs.end();

  // limpiar RAM también
  for(int i=0;i<MAX_EVENTOS;i++){
    eventos[i] = "";
  }

  indiceEvento = 0;
}