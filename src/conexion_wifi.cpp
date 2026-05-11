#include "conexion_wifi.h"
#include "variables.h"
#include "telegram.h"
#include "historial.h"

void iniciarWifi()
{

prefs.begin("config",true);

String savedToken=prefs.getString("token");
String savedChat=prefs.getString("chatid");

prefs.end();

param_token.setValue(savedToken.c_str(),60);
param_chatid.setValue(savedChat.c_str(),20);

wm.addParameter(&param_token);
wm.addParameter(&param_chatid);

wm.autoConnect("SolucionesIOT");

BOTtoken=param_token.getValue();
CHAT_ID=param_chatid.getValue();

prefs.begin("config",false);

prefs.putString("token",BOTtoken);
prefs.putString("chatid",CHAT_ID);

prefs.end();

}

void reconectarWiFi(){  


if(WiFi.status()!=WL_CONNECTED){

WiFi.reconnect();
guardarEvento("📡 WiFi DESCONECTADO");
unsigned long start=millis();

while(WiFi.status()!=WL_CONNECTED && millis()-start<5000){
delay(50);

guardarEvento(
"📶 WiFi RECONECTADO | IP: " + WiFi.localIP().toString()
);
}

}

}

void controlarWiFi()
{

bool estadoActual = WiFi.status()==WL_CONNECTED;

if(estadoActual != wifiEstadoAnterior){

if(!estadoActual){

enviarTelegram(CHAT_ID,"⚠️ WiFi desconectado");

}else{

enviarTelegram(CHAT_ID,"✅ WiFi reconectado");

}

wifiEstadoAnterior=estadoActual;

}

}
