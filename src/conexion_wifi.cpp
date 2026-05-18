#include "conexion_wifi.h"
#include "variables.h"
#include "telegram.h"
#include "historial.h"

void iniciarWifi()
{
    Serial.println("Iniciando WiFiManager...");

    // ===== LEER CONFIG GUARDADA =====
    prefs.begin("config", true);

    String savedToken = prefs.getString("token", "");
    String savedChat  = prefs.getString("chatid", "");
    String savedClave = prefs.getString("clave", "1234");

    prefs.end();

    // guardar en RAM
    claveAdmin = savedClave;

    Serial.println("Clave cargada:");
    Serial.println(claveAdmin);

    // ===== PARAMETROS PERSONALIZADOS =====

    param_token.setValue(
        savedToken.c_str(),
        60
    );

    param_chatid.setValue(
        savedChat.c_str(),
        20
    );

    WiFiManagerParameter param_clave(
        "clave",
        "Clave admin",
        savedClave.c_str(),
        20
    );

    wm.addParameter(&param_token);
    wm.addParameter(&param_chatid);
    wm.addParameter(&param_clave);

    
/////// ---- Iniciamos WiFiManager y portal de configuracion ---- ///////

wm.setTitle("🌱 Sistema de Riego IOT");

wm.setCustomHeadElement(R"(

<style>

body{
    background:#0f0f17;
    color:#fff;
    font-family:Arial,sans-serif;
}

.wrap{

    max-width:420px;

    background:#1a1a28;

    margin:auto;

    margin-top:25px;

    padding:25px;

    border-radius:22px;

    box-shadow:
    0 0 25px rgba(130,0,255,.25);

}

h1,h2,h3{

    color:#b366ff;

    text-align:center;

}

hr{
    border:1px solid #302050;
}

label{

    color:#c9a0ff;

    font-size:14px;
}

input{

    background:#242438;

    color:white;

    border:1px solid #8b3dff;

    border-radius:14px;

    padding:14px;

    width:100%;

    margin-top:8px;

    margin-bottom:10px;

}

input:focus{

outline:none;

border-color:#c86fff;

box-shadow:
0 0 12px #a100ff;

}

button,
input[type=submit]{

background:
linear-gradient(
90deg,
#6500ff,
#ae00ff
);

border:none;

color:white;

padding:14px;

width:100%;

border-radius:14px;

font-size:16px;

margin-top:15px;

}

button:hover{

transform:scale(1.02);

}

a{
color:#bb86fc;
}

.info{

background:#232336;

padding:10px;

border-radius:12px;

margin-top:10px;

}

</style>

)");


    // ===== CONECTAR =====

    wm.autoConnect("SolucionesIOT");

    Serial.println("WiFi conectado");
    Serial.println(WiFi.SSID());

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // ===== LEER DATOS DEL PORTAL =====

    BOTtoken = param_token.getValue();

    CHAT_ID = param_chatid.getValue();

    claveAdmin = param_clave.getValue();

    Serial.println("Clave actual:");
    Serial.println(claveAdmin);

    // ===== GUARDAR =====

    prefs.begin("config", false);

    prefs.putString("token", BOTtoken);

    prefs.putString("chatid", CHAT_ID);

    prefs.putString("clave", claveAdmin);

    prefs.end();

    Serial.println("Configuración guardada");
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

void controlarWiFi(){

bool wifiActual = WiFi.status() == WL_CONNECTED;

// SE CORTO
if(!wifiActual && wifiAnterior){

    guardarEvento("📡 WiFi DESCONECTADO");

    Serial.println("WIFI DESCONECTADO");

    wifiAnterior = false;
}

// VOLVIO
if(wifiActual && !wifiAnterior){

    delay(200);
    guardarEvento("📡 WiFi RECONECTADO");

    enviarATodos(
    "📡 WiFi RECONECTADO\n\n"
    "🌐 IP: " + WiFi.localIP().toString()
    );

    Serial.println("WIFI RECONECTADO");

    wifiAnterior = true;
}
}