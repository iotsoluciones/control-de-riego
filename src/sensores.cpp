
#include "sensores.h"
#include "variables.h"
#include "telegram.h"

void leerSensores()
{

static unsigned long tReinicioDHT=0;

if(millis()-tReinicioDHT > 1800000){

    Serial.println("♻ Reinicio preventivo DHT");

    dht.begin();

    tReinicioDHT=millis();

    delay(100);
}

yield();

float t=dht.readTemperature();

delay(50);

float h=dht.readHumidity();

if(isnan(t) || isnan(h)){

    Serial.println("⚠ FALLO DHT");

    dht.begin();

    temperatura=ultimaTempOK;
    humedad=ultimaHumOK;

    return;
}

ultimaTempOK=t;
ultimaHumOK=h;

temperatura=t;
humedad=h;

  // 🌱 SENSOR HUMEDAD DE SUELO
  int valorSuelo = analogRead(35);

  // mapear (AJUSTAR después con calibración real)
  humedadSuelo = map(valorSuelo, 3000, 1500, 0, 100);

  // limitar rango
  if(humedadSuelo < 0) humedadSuelo = 0;
  if(humedadSuelo > 100) humedadSuelo = 100;

  // DEBUG (muy recomendable)
static unsigned long debugSuelo = 0;

if(millis() - debugSuelo > 10000){

  debugSuelo = millis();

  Serial.print("Suelo RAW: ");
  Serial.print(valorSuelo);
  Serial.print(" | Suelo %: ");
  Serial.println(humedadSuelo);
}

  /* RESET BLOQUEO HUMEDAD AMBIENTE */
  if(humedad <= humedadLimite && avisoHumedadEnviado){
    enviarTelegram(CHAT_ID,"✅ Humedad normal, riego habilitado");
    avisoHumedadEnviado = false;
  }
}

