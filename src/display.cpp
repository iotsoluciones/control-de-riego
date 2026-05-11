#include <Arduino.h>
#include <WiFi.h>

#include "display.h"
#include "variables.h"
#include "helpers.h"


void mostrarTexto(String txt){
  display.clearDisplay();
  display.setCursor(0,10);
  display.println(txt);
  display.display();
}

void actualizarDisplay(){

  if (modoreset) {
    return; // no actualiza display durante reset wifi
  } 

  display.clearDisplay();

  // TITULO
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("SolucionesIOT");

  // WIFI
  display.setCursor(0,12);
  if(WiFi.status() == WL_CONNECTED){
    display.print("WiFi: ");
    display.print(obtenerCalidadWiFi());
    display.print("%");
  }else{
    display.print("WiFi: OFF");
  }

  // IP
  display.setCursor(0,24);
  display.print("IP: ");
  display.println(WiFi.localIP());
 
  display.display();
}

void mostrarDisplayBloqueado(){

    static unsigned long ultimoDisplay = 0;

    if(millis() - ultimoDisplay < 500){
        return;
    }

    ultimoDisplay = millis();

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(1,4);
    display.println("SISTEMA");

    display.setCursor(9,20);
    display.println("BLOQUEADO!!");

    display.display();
}