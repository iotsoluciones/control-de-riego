#include <WiFi.h>
#include "seguridad.h"
#include "variables.h"
#include "telegram.h"
#include "historial.h"
#include "display.h"

void resetWifi(){


  static unsigned long refresh = 0;

  if(millis() - refresh < 200) return;
  refresh = millis();

  unsigned long tiempo = millis() - tiempoPresionado;
  int segundos = tiempo / 1000;

  display.clearDisplay();
  display.setCursor(0,5);
  display.println("Mantener boton...");
  display.setCursor(0,25);
  display.print("Reset wifi en: ");

  int restante = 5 - segundos;
  if(restante < 0) restante = 0;

  display.print(restante);
  display.println("s");
  display.display();

  if(tiempo >= 5000){
    mostrarTexto("Reseteando WiFi...");

    if(WiFi.status() == WL_CONNECTED){
      guardarEvento("Reset wifi por boton físico (LOCAL)");
      enviarTelegram(CHAT_ID,"✔️ Reset WiFi Finalizado con exito!! - VUELVE A PROGRAMAR DESDE SU MOVIL CONECTANDO A RED: SolucionesIOT 🛜");
      yield(); // para actualizar display antes de reiniciar
       delay(2000); // pequeño delay para que el mensaje se alcance a mostrar 
    }
    
    wm.resetSettings();
    ESP.restart();
  }
}

void apagarReleSeguro(int r){

  // apago rele primero
  if(reles[7].encendido){
    digitalWrite(relePin[7], HIGH);
    reles[7].encendido = false;
  }

  // prog.apagado valvula
  esperandoApagadoRele = true;
  timerApagadoRele = millis();
  relePendienteApagado = r;
}

void bloqueoSistemaCompleto(){

    if(bloqueoActual){

        bloqueoActivoGlobal = true;

        // APAGAR TODO
        for(int r=0; r<8; r++){

            digitalWrite(relePin[r], HIGH);

            reles[r].encendido = false;

        }

        esperandoApagadoRele = false;

        esperaBomba = false;

        bloqueoBombaManual = millis();

        guardarEvento(
        "⛔ BLOQUEO FISICO ACTIVADO");

        enviarATodos(
        "⛔ BLOQUEO FISICO ACTIVADO\nSistema detenido");

    }else{

        bloqueoActivoGlobal = false;

        bloqueoBombaManual = millis();

        guardarEvento(
        "✅ BLOQUEO FISICO DESACTIVADO");

        enviarATodos(
        "✅ BLOQUEO DESACTIVADO\nSistema reanudado");
    }
}