#include "helpers.h"
#include "variables.h"
#include <WiFi.h>


String sugerirComando(String txt){

 
  if(txt.indexOf("borra")>=0) return "/borrarhorarios";
  if(txt.indexOf("est")>=0) return "/estado";
  if(txt.indexOf("men")>=0) return "/menurapido";
  if(txt.indexOf("rel")>=0) return "/rele1";
  if(txt.indexOf("ho")>=0) return "/horarios";
  if(txt.indexOf("se")>=0) return "/sensores";
  if(txt.indexOf("tan")>=0) return "/tanquesi";
  if(txt.indexOf("ha")>=0) return "/habilitartodos";
  if(txt.indexOf("de")>=0) return "/deshabilitartodos";
  if(txt.indexOf("bo")>=0) return "/borrarnombres";
  if(txt.indexOf("to")>=0) return "/todooff";
  if(txt.indexOf("re")>=0) return "/reiniciar";
  if(txt.indexOf("us")>=0) return "/usuarios";
  if(txt.indexOf("pa")>=0) return "/panel";
  if(txt.indexOf("sen")>=0) return "/sensorsi";
  if(txt.indexOf("ut")>=0) return "/autorizar ID Nombre";
  if(txt.indexOf("el")>=0) return "/eliminar ID";
  if(txt.indexOf("bo")>=0) return "/borrarhistorial";
  if(txt.indexOf("hi")>=0) return "/historial";
  if(txt.indexOf("us")>=0) return "/usuarios";
  if(txt.indexOf("cl")>=0) return "/clima";
  if(txt.indexOf("no")>=0) return "/nombrerele N Nombre"; 
  return "";

}

int obtenerCalidadWiFi()
{
  if(WiFi.status()!=WL_CONNECTED) return 0;
  int rssi = WiFi.RSSI();
  int calidad = map(rssi,-100,-50,0,100);
  if(calidad<0) calidad=0;
  if(calidad>100) calidad=100;
  return calidad;
}

String textoWiFiEstado()
{
  if(WiFi.status()!=WL_CONNECTED) return "SIN WIFI";
  return String(obtenerCalidadWiFi())+"% ("+String(WiFi.RSSI())+" dBm)";
}

String obtenerMotivoBloqueo(){

  // 🌧 LLUVIA
  bool lluvia = probabilidadLluvia > 60;

  if(lluviaBloqueada){
    if(millis() - bloqueoLluviaTiempo < 10800000){
      lluvia = true;
    }else{
      lluviaBloqueada = false;
    }
  }

  if(lluvia){
    return "LLUVIA";
  }

  // 💧 HUMEDAD AMBIENTE
  if(bloqueoHumedad && humedad > humedadLimite){
    return "HUMEDAD";
  }

 // 🌱 SUELO
if(sensorSueloActivo){
  if(bloqueoSuelo && humedadSuelo > humedadSueloLimite){
    return "SUELO";
  }
}
  return ""; // sin bloqueo
}
