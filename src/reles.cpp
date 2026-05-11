# include "reles.h"
# include "variables.h"
# include "telegram.h"

void controlarBomba(){

    static unsigned long tiempoCambioBomba = 0;

    bool necesitaBomba = false;

    // rele 1 al 6
    for(int i=0;i<6;i++){

        if(reles[i].encendido){
            necesitaBomba = true;
            break;
        }
    }
    // rele7 manual/programado
    if(!modoTanqueAutomatico && reles[6].encendido){
         necesitaBomba = true;
        }

   // tanque automático
if(tanqueNecesitaAgua){

    necesitaBomba = true;

    // detectar cambio a modo tanque
    if(!ultimoEstadoTanque){

        Serial.println("CAMBIO A MODO TANQUE");

        bombaEncendida = false;
        estadoAnteriorBomba = false;

        ultimoEstadoTanque = true;
    }

    // activar rele7
    if(!reles[6].encendido){

        digitalWrite(relePin[6], LOW);

        reles[6].encendido = true;

        Serial.println("RELE7 AUTOMATICO ON");
    }
}
else{

    ultimoEstadoTanque = false;

    // apagar rele7 automático
    if(modoTanqueAutomatico && reles[6].encendido){

        digitalWrite(relePin[6], HIGH);

        reles[6].encendido = false;

        Serial.println("RELE7 AUTOMATICO OFF");
    }
}

    // detectar nueva demanda
if(necesitaBomba && !estadoAnteriorBomba){

    tiempoCambioBomba = millis();

    estadoAnteriorBomba = true;
}

// esperar 1 segundo antes de prender bomba
if(necesitaBomba && !bombaEncendida){

    if(millis() - tiempoCambioBomba >= 1000){

        digitalWrite(relePin[7], LOW);

        reles[7].encendido = true;

        bombaEncendida = true;
    }
}

// reset estado
if(!necesitaBomba){

    estadoAnteriorBomba = false;
}

    // APAGAR BOMBA
    if(!necesitaBomba && bombaEncendida){

        digitalWrite(relePin[7], HIGH);

        reles[7].encendido = false;

        bombaEncendida = false;

        tiempoCambioBomba = millis();
    }
}
void controlarTanque(){

    if(bloqueoActivoGlobal){
        tanqueNecesitaAgua = false;
        return;
    }

    if(!modoTanqueAutomatico){

        tanqueNecesitaAgua = false;
        return;
    }

    
static unsigned long timerFlotante = 0;

bool lecturaFlotante = digitalRead(SENSOR_TANQUE) == HIGH;

bool pedidoAgua = false;

// filtro anti rebote SOLO tanque
if(lecturaFlotante){

    if(timerFlotante == 0){

        timerFlotante = millis();
    }

    // medio segundo estable
    if(millis() - timerFlotante >= 500){

        pedidoAgua = true;
    }

}else{

    timerFlotante = 0;
}

    bool riegoActivo = false;

    for(int i=0;i<6;i++){

        if(reles[i].encendido){

            riegoActivo = true;
            break;
        }
    }

    // si hay riego → tanque espera
    if(riegoActivo){

    // si tanque estaba activo
    if(reles[6].encendido){

        // apagar bomba primero
        if(reles[7].encendido){

            digitalWrite(relePin[7], HIGH);
            reles[7].encendido = false;

            delay(300);
        }

        // cerrar tanque
        digitalWrite(relePin[6], HIGH);
        reles[6].encendido = false;

        delay(300);

        // permitir rearme limpio
        bombaEncendida = false;
        estadoAnteriorBomba = false;
    }

    tanqueNecesitaAgua = false;

    return;
}
    Serial.print("Tanque | Pedido: ");
Serial.print(pedidoAgua);

Serial.print(" | RiegoActivo: ");
Serial.print(riegoActivo);

Serial.print(" | NecesitaAgua: ");
Serial.println(tanqueNecesitaAgua);

    // tanque pide agua
    tanqueNecesitaAgua = pedidoAgua;

}

void apagarTodosReles(){


    for(int r=0;r<8;r++){
      digitalWrite(relePin[r],HIGH);
      reles[r].encendido=false;
    }
    guardarEvento("⛔ Todos los Rele ACTIVOS apagados desde boton físico (LOCAL)");
    enviarTelegram(CHAT_ID,"⛔ Todos los Rele ACTIVOS apagados desde boton físico (LOCAL)");

  }

void ejecutarRele(int r, String origen, String usuario, String chat_id){
 
  bloqueoManualTiempo = millis();

  if(reles[r].encendido){

 bool quedaAlgoActivo = false;

// revisar relés 1-7
for(int i=0;i<7;i++){

    // ignorar el relé que estoy apagando
    if(i == r){
        continue;
    }

    if(reles[i].encendido){

        quedaAlgoActivo = true;
        break;
    }
}

// SOLO apagar bomba si no queda nada activo
if(!quedaAlgoActivo){

    if(reles[7].encendido){

        digitalWrite(relePin[7], HIGH);
        reles[7].encendido = false;

        delay(300);
    }
}

// apagar rele
digitalWrite(relePin[r], HIGH);
reles[r].encendido = false;

enviarATodos(
"🔴 " + reles[r].nombre + " apagado desde " + origen + " ("+usuario+")"
);
}
  else{

    digitalWrite(relePin[r],LOW);
    reles[r].encendido = true;

    reles[r].duracion = 0;
    reles[r].inicio = millis();

enviarATodos(
"🟢 " + reles[r].nombre + " encendido desde " + origen + " ("+usuario+")"
);

  }
}

void iniciarReles()
{

prefs.begin("reles",true);


for(int i=0;i<8;i++){

pinMode(relePin[i],OUTPUT);
digitalWrite(relePin[i],HIGH);

/* cargar nombre guardado */
String nombre = prefs.getString(("nombre"+String(i)).c_str(),"Rele "+String(i+1));

reles[i].nombre = nombre;

/* cargar habilitado guardado */
reles[i].habilitado = prefs.getBool(("hab"+String(i)).c_str(), true);

reles[i].encendido=false;
reles[i].ultimoMinuto=-1;

}

prefs.end();

}