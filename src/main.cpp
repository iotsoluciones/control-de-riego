#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "variables.h"
#include "funciones.h"
#include "display.h"
#include "historial.h"
#include "sensores.h"
#include "reles.h"
#include "clima.h"
#include "telegram.h"
#include "telegram_menu.h"
#include "seguridad.h"
#include "usuarios.h"
#include "horarios.h"
#include "helpers.h"
#include "botones.h"
#include "conexion_wifi.h"

void setup(){
  iniciarSistema();
}

void loop(){
  loopSistema();
} 
