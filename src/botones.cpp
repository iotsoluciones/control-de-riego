#include "botones.h"
#include "variables.h"
#include "historial.h"
#include <telegram.h>
#include "reles.h"
void controlarBotonAUX(){
  
  static bool estadoAnterior = LOW;
  static unsigned long debounce = 0;

  bool estadoActual = digitalRead(BottAUX);

  // detectar flanco REAL (cuando apretás)
  if(estadoAnterior == LOW && estadoActual == HIGH){
     Serial.println(digitalRead(BottAUX));
    if(millis() - debounce > 1000){

      debounce = millis();

    static unsigned long ultimoAviso = 0;

if(bloqueoFisicoActivo){

    if(millis() - ultimoAviso > 5000){

        enviarTelegram(
            CHAT_ID,
            "⛔ Sistema bloqueado físicamente\n"
            "No se puede ejecutar la acción"
        );

        ultimoAviso = millis();
    }

    return;
}


      int r = 5;

      bool estadoAntes = reles[r].encendido;

      ejecutarRele(r, "botón físico", "LOCAL", CHAT_ID);

      if(estadoAntes){
        guardarEvento(reles[r].nombre + " apagado desde boton fisico");
      }else{
        guardarEvento(reles[r].nombre + " encendido desde boton fisico");
      }
    }
  }

  estadoAnterior = estadoActual;
}

void actualizarHora()
{

struct tm timeinfo;
if(!getLocalTime(&timeinfo)) return;

static int minutoPrevio = -1;

if(timeinfo.tm_min != minutoPrevio){

char buffer[6];

sprintf(buffer,"%02d:%02d",
timeinfo.tm_hour,
timeinfo.tm_min);
minutoPrevio = timeinfo.tm_min;

}

}
