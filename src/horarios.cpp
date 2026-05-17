#include "horarios.h"
#include "variables.h"
#include "telegram.h"
#include "wifi.h"
#include "historial.h"
#include "seguridad.h"
#include "display.h"
#include "reles.h"


void controlMultiplesHorarios(int hora,int minuto){

  if(bloqueoFisicoActivo){
    return;
  }

  if(millis() - bloqueoManualTiempo < 5000){
    return;
  }

  static bool lluviaActivaAntes = false;
  bool lluviaActual = probabilidadLluvia > 60;

  // activar bloqueo por lluvia
  if(lluviaActual){
    lluviaBloqueada = true;
    bloqueoLluviaTiempo = millis();
  }

 

  bool lluviaActiva = lluviaActual;

  if(lluviaActiva && !lluviaActivaAntes){
    enviarATodos("🌧️ Riego bloqueado por lluvia (" + String(probabilidadLluvia,1) + "%)");
    guardarEvento("Riego bloqueado por lluvia " + String(probabilidadLluvia) + "%");
  }

  if(!lluviaActiva && lluviaActivaAntes){
    enviarATodos("✅ Lluvia normal, riego habilitado");
    guardarEvento("Riego habilitado por clima normal");
  }

  lluviaActivaAntes = lluviaActiva;

  int limite = modoTanqueAutomatico ? 6 : 7;

  // ⏱ CORTE POR TIEMPO
  for(int r=0; r<limite; r++){

    if(reles[r].encendido && reles[r].duracion > 0){

      if(millis() - reles[r].inicio >= reles[r].duracion*1000){

        digitalWrite(relePin[r], HIGH);
        reles[r].encendido = false;
        reles[r].ultimoMinuto = minuto;

        bool quedaActivo = false;

        for(int i=0;i<6;i++){
          if(reles[i].encendido){
            quedaActivo = true;
            break;
          }
        }

        if(!quedaActivo){
          digitalWrite(relePin[7], HIGH);
          reles[7].encendido = false;
        }
        Serial.println("Riego finalizado automaticamente: " + reles[r].nombre);
        enviarATodos("✅ "+reles[r].nombre+" finalizado con exito!!");
      }
    }
  }

  // ⏰ CONTROL POR MINUTO
  static int ultimoMinutoGlobal = -1;

  if(minuto != ultimoMinutoGlobal){

    ultimoMinutoGlobal = minuto;

    for(int r=0; r<limite; r++){

      if(!reles[r].habilitado) continue;

      for(int h=0; h<cantidadHorarios[r]; h++){

        if(!reles[r].encendido &&
           hora == horarios[r][h].hora &&
           minuto == horarios[r][h].minuto &&
           reles[r].ultimoMinuto != minuto){

          // 🌧 BLOQUEO POR LLUVIA
          if(lluviaActiva){
            reles[r].ultimoMinuto = minuto;
            continue;
          }

          // 🌫 BLOQUEO HUMEDAD AMBIENTE
          if(bloqueoHumedad && humedad > humedadLimite){

            if(!avisoHumedadEnviado){
              enviarATodos("⚠️ Riego cancelado\nHumedad alta: "+String(humedad,1)+"%");
              guardarEvento("Riego cancelado por humedad " + String(humedad) + "%");
              avisoHumedadEnviado = true;
            }

            reles[r].ultimoMinuto = minuto;
            continue;
          }

          // 🌱 BLOQUEO HUMEDAD SUELO
          if(sensorSueloActivo && bloqueoSuelo && humedadSuelo > humedadSueloLimite){

            enviarATodos("🌱 Riego cancelado\nSuelo HUMEDO");
            guardarEvento("Riego cancelado por suelo humedo (" + String(humedadSuelo) + ")");

            reles[r].ultimoMinuto = minuto;
            continue;
          }

          // ✅ ACTIVACIÓN NORMAL
          reles[r].duracion = horarios[r][h].duracion;

          digitalWrite(relePin[r], LOW);
          reles[r].encendido = true;

          esperaBomba = true;
          timerBomba = millis();

          reles[r].inicio = millis();
          reles[r].ultimoMinuto = minuto;
          Serial.println("Riego iniciado automaticamente: " + reles[r].nombre);
          enviarATodos("⚡ Activacion automatica: " + reles[r].nombre);
        }
      } 
    }
  }
}

void guardarHorarios()
{

prefs.begin("horarios", false);

for(int r=0;r<8;r++){

prefs.putInt(("cant"+String(r)).c_str(),cantidadHorarios[r]);

for(int h=0;h<cantidadHorarios[r];h++){

prefs.putInt(("h"+String(r)+String(h)).c_str(),horarios[r][h].hora);
prefs.putInt(("m"+String(r)+String(h)).c_str(),horarios[r][h].minuto);
prefs.putInt(("d"+String(r)+String(h)).c_str(),horarios[r][h].duracion);

}

}

prefs.end();

}

void cargarHorarios()
{

prefs.begin("horarios", true);

for(int r=0;r<8;r++){

cantidadHorarios[r]=prefs.getInt(("cant"+String(r)).c_str(),0);

for(int h=0;h<cantidadHorarios[r];h++){

horarios[r][h].hora=prefs.getInt(("h"+String(r)+String(h)).c_str(),0);
horarios[r][h].minuto=prefs.getInt(("m"+String(r)+String(h)).c_str(),0);
horarios[r][h].duracion=prefs.getInt(("d"+String(r)+String(h)).c_str(),10);

}

}

prefs.end();

}
