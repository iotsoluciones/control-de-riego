#include <WiFi.h>
#include "seguridad.h"
#include "telegram.h"
#include "funciones.h"
#include "variables.h"
#include "sensores.h"
#include "display.h"
#include "botones.h"
#include "clima.h"
#include "horarios.h"
#include "reles.h"
#include "conexion_wifi.h"

void iniciarSistema(){

Serial.begin(115200);
Serial.println("TEST SERIAL");

prefs.begin("config", true);
lat = prefs.getFloat("lat", -34.60);
lon = prefs.getFloat("lon", -58.38);
prefs.end();

prefs.begin("users", true);

cantidadUsuarios = prefs.getInt("cant", 0);

for(int i=0;i<cantidadUsuarios;i++){
  usuariosID[i] = prefs.getString(("id"+String(i)).c_str(), "");
  usuariosNombre[i] = prefs.getString(("nom"+String(i)).c_str(), "");
}

prefs.end();

pinMode(BottBloqueo,INPUT);
pinMode(Sensor_Suelo,INPUT);
pinMode(BottAUX,INPUT);
pinMode(BOTON_RESET,INPUT);
pinMode(BottOFF,INPUT);
pinMode(SENSOR_TANQUE,INPUT);

prefs.begin("eventos", true);

indiceEvento = prefs.getInt("indice", 0);
int totalEventos = prefs.getInt("total", 0);  

for(int i=0;i<MAX_EVENTOS;i++){
  eventos[i] = prefs.getString(("e"+String(i)).c_str(), "");
}

prefs.end();

  Wire.begin(SDA_PIN, SCL_PIN);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error display");
    while(true);
  }

display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);

// pantalla inicial
display.setCursor(0,0);
display.println("SolucionesIOT");

display.setCursor(0,20);
display.println("Configurar WiFi...");

display.display();

/* SENSOR */
dht.begin();

/* RELES */
iniciarReles();

/* WIFI */
iniciarWifi();
WiFi.setSleep(false);
clientTelegram.setInsecure();
clientTelegram.setTimeout(500);
myBot.setUpdateTime(100);
myBot.setTelegramToken(BOTtoken.c_str());
myBot.begin();


WiFi.setTxPower(WIFI_POWER_19_5dBm);

// 👇 MOSTRAR WIFI OK
display.clearDisplay();
display.setCursor(0,0);
display.println("SolucionesIOT");

display.setCursor(0,20);
display.println("WiFi OK - Espere...");

display.display();

delay(50);

if(WiFi.status()==WL_CONNECTED){
}

ArduinoOTA.setHostname("SolucioinesIOT");  
ArduinoOTA.setPassword("1234");    // contraseña para cargar actualizacion OTA 
ArduinoOTA.begin();

/* CONFIGURACION */
prefs.begin("config",true);
humedadLimite = prefs.getFloat("humedadLimite",90);
ciudad = prefs.getString("ciudad", "Buenos Aires");
modoTanqueAutomatico = prefs.getBool("modoTanque", true);
sensorSueloActivo = prefs.getBool("sensorSuelo", true);
prefs.end();

/* SINCRONIZAR HORA */
configTime(gmtOffset_sec,0,ntpServer);

struct tm timeinfo;

int intentos=0;

while(!getLocalTime(&timeinfo) && intentos<5){
delay(50);
intentos++;
}

 // CARGAR DATOS 
cargarHorarios();
leerSensores();

inicio = true;
bloqueoArranque = millis(); 

ArduinoOTA.begin();
 

}

void loopSistema(){

if(inicio){

    // esperar conexión estable
    if(WiFi.status() == WL_CONNECTED){

        // esperar 10 segundos después del arranque
        if(millis() - bloqueoArranque > 5000){

            if(!avisoInicioEnviado){

                Serial.println("ENVIANDO AVISO INICIO A TODOS");

                enviarATodos(
                    "☑️ Sistema iniciado correctamente"
                );

                avisoInicioEnviado = true;

                inicio = false;
            }
        }
    }
}
  ArduinoOTA.handle();


  ///// BLOQUEO DE SEGURIDAD

    bool lectura = digitalRead(BottBloqueo);

    if(lectura != bloqueoAnterior){

        delay(30);

        lectura = digitalRead(BottBloqueo);

        if(lectura != bloqueoAnterior){

            bloqueoActual = lectura;

            bloqueoSistemaCompleto();

            bloqueoAnterior = lectura;
        }
    }

    // 🚫 BLOQUEO TOTAL
    if(bloqueoActivoGlobal){

        mostrarDisplayBloqueado();

        return;
    }

 manejarTelegram();


static unsigned long ultimoPrint = 0;

if(millis() - ultimoPrint > 2000){

  ultimoPrint = millis();

  Serial.print("Loop OK | Heap: ");
  Serial.print(ESP.getFreeHeap());

  Serial.print(" | RSSI: ");
  Serial.println(WiFi.RSSI());
}

  unsigned long nowe = millis();


  // reset wifi
bool estadoBoton = digitalRead(BOTON_RESET);

// 👉 detecta cuando recién apretás
if(estadoBoton == HIGH && !botonActivo){
  tiempoPresionado = millis();
  botonActivo = true;
  modoreset = true;
}



 // 🔴 MODO RESET TOTAL
if(modoreset){

  bool estadoBoton = digitalRead(BOTON_RESET);

  if(estadoBoton == HIGH){
    resetWifi();   // ✅ se ejecuta TODO el tiempo
  }else{
    enviarTelegram(CHAT_ID,"❌ Reset Red WiFi cancelado - ⛔ NO SE PUDO EJECUTAR LA FUNCION!!");
    botonActivo = false;
    avisoEnviado = false;
    modoreset = false;
  }
  hayComandoTelegram=false;
  return; // ⛔ corta TODO
}


if(millis() - timerDisplay > 500){
  timerDisplay = millis();
  actualizarDisplay();
}


  // tareas siguientes
  
if(hayComandoTelegram){
    procesarTelegram();
}

  controlarBotonAUX();
  controlarTanque();
  controlarBomba();

  if(nowe - timerClock > 2000){
    timerClock = nowe;
    actualizarHora();
  }

   if(nowe - timerSensor > 3000){   
    timerSensor = nowe;
    leerSensores();
  } 
 
  // CONTROL DE RIEGO

  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    controlMultiplesHorarios(timeinfo.tm_hour, timeinfo.tm_min);
  }

  // wifi

  if(nowe - timerWiFi > 5000){
    timerWiFi = nowe;
    reconectarWiFi();
    controlarWiFi();
  }


  // clima

  if(nowe - timerClima > 3600000){   // ⬅️ c/ 
    timerClima = nowe;
    consultarClima();
  }



  if(esperandoApagadoRele && nowe - timerApagadoRele >= 60000){
    digitalWrite(relePin[relePendienteApagado],HIGH);
    reles[relePendienteApagado].encendido=false;
    esperandoApagadoRele = false;
  }

  // reporte conexion ok

  if(nowe - timerReporte > 18000000){
    enviarATodos("📡 Sistema OK\n");
    timerReporte = nowe;
  }

  // boton todos off

  if(digitalRead(BottOFF) == HIGH && nowe - debounceOFF > 10000){
    debounceOFF = nowe;
    apagarTodosReles();

  }

  // reinicio

  if(reinicioPendiente && nowe - timerReinicio >= 80000){
    ESP.restart();
  }



// 👉 cuando soltás el botón
if(estadoBoton == LOW && botonActivo){
  enviarTelegram(CHAT_ID,"No se pudo realizar el reset WiFi desde el botón");
  botonActivo = false;
  avisoEnviado = false;
  modoreset = false;
}

static unsigned long timerTelegram = 0;


}