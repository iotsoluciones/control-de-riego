#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <DHT.h>
#define DHTTYPE DHT22
#define MAX_HORARIOS 5
#define MAX_USERS 5
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define MAX_EVENTOS 100

String eventos[MAX_EVENTOS];
int indiceEvento = 0;

int BottAUX =34 ;  // para encender rele 6 manuelamente desde panel fisico
int BottBloqueo =35 ; // para deshabilitar los rele en automatico por alguna modificacion o reparacion 
int Bottreset=25; //  pin entrada reset para red wifi
int BottOFF=27;   // pin entrada boton para apagar todos los rele 
#define LED_R 4  // pin salida LED rojo
#define LED_G 2  // pin salida LED verde
#define LED_B 5  // PIN salida LED azul 
#define SENSOR_TANQUE 18  // pin entrada señal de sensor tanque
#define DHTPIN 15         // entrada dato de sensor temp-hum
int relePin[8]={13,23,14,22,26,21,33,32};   // salida de pines rele

String ciudad = "Buenos Aires";
float probabilidadLluvia = 0;
unsigned long timerClima = 0;
bool bloqueoActivoGlobal = false;
unsigned long bloqueoBombaManual = 0;
String usuarios[MAX_USERS];
int cantidadUsuarios = 0;
float lat = -34.60;
float lon = -58.38;
String usuariosID[MAX_USERS];
String usuariosNombre[MAX_USERS];
unsigned long timerReporte = 0;
DHT dht(DHTPIN, DHTTYPE);
unsigned long bloqueoArranque=0;  
WiFiManager wm;

WiFiManagerParameter param_token("token","Token Telegram","",60);
WiFiManagerParameter param_chatid("chatid","Chat ID","",20);

Preferences prefs;

WiFiClientSecure client;
String BOTtoken;
String CHAT_ID;

UniversalTelegramBot *bot;

bool avisoHumedadEnviado=false;
int segundosPrevios = -1;
bool wifiEstadoAnterior=true;
bool modoTanqueAutomatico = true;
struct Rele{
String nombre;
bool habilitado;
bool encendido;
byte hora;
byte minuto;
int duracion;
unsigned long inicio;
int ultimoMinuto;
};

Rele reles[8];

struct Horario{
byte hora;
byte minuto;
int duracion;
};

Horario horarios[8][MAX_HORARIOS];
int cantidadHorarios[8]={0};

float temperatura=0;
float humedad=0;
float humedadLimite=90;
bool bloqueoHumedad=true;
unsigned long bloqueoManualTiempo = 0;
const char* ntpServer="pool.ntp.org";
const long gmtOffset_sec=-10800;

String horaActual="";
String ultimoEvento="Sistema iniciado";

unsigned long timerBlink =0;
bool estadoBlink = false ;
unsigned long timerApagadoRele = 0;
bool esperandoApagadoRele = false;
int relePendienteApagado = -1;
unsigned long timerBomba = 0;
bool esperaBomba = false;
unsigned long timerReinicio = 0;
bool reinicioPendiente = false;
unsigned long debounceOFF = 0;
unsigned long timerTelegram=0;
unsigned long timerClock=0;
unsigned long timerSensor=0;

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
bool bloqueoFisicoActivo(){
  return digitalRead(BottBloqueo) == LOW; // pulsado activado
}
void guardarEvento(String texto){

  struct tm timeinfo;
  String hora = "";

  if(getLocalTime(&timeinfo)){
    char buffer[10];
    sprintf(buffer,"%02d:%02d:%02d",
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec);
    hora = String(buffer);
  }

  String evento = "🕒 " + hora + " | " + texto;

  eventos[indiceEvento] = evento;

  prefs.begin("eventos", false);
  prefs.putString(("e"+String(indiceEvento)).c_str(), evento);
  prefs.putInt("indice", indiceEvento);
  prefs.end();

  indiceEvento++;

  if(indiceEvento >= MAX_EVENTOS){
    indiceEvento = 0; // 🔥 circular
  }
}
void setColor(bool r, bool g, bool b){

  digitalWrite(LED_R, r);
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);

}
void actualizarLED(){

  // 🔴 SIN WIFI
  if(WiFi.status() != WL_CONNECTED){
    setColor(1,0,0);
    return;
  }

  // 🟣 LLUVIA O HUMEDAD ALTA
  if(probabilidadLluvia > 60 || humedad > humedadLimite){
    setColor(1,0,1);
    return;
  }

  // 🟡 RELÉ ACTIVO → PARPADEO
  bool hayReleActivo = false;

  for(int i=0;i<7;i++){
    if(reles[i].encendido){
      hayReleActivo = true;
      break;
    }
  }

  if(hayReleActivo){

    // parpadeo cada 1 segundo
    if(millis() - timerBlink >= 500){
      timerBlink = millis();
      estadoBlink = !estadoBlink;
    }

    if(estadoBlink){
      setColor(1,1,0); // amarillo ON
    }else{
      setColor(0,0,0); // apagado
    }

    return;
  }

  // 🟢 TODO OK
  setColor(0,1,0);
}
void consultarClima(){

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("❌ Sin WiFi");
    return;
  }

  
  client.setInsecure();

  HTTPClient https;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" 
               + String(lat,6) + 
               "&longitude=" + String(lon,6) + 
               "&hourly=precipitation_probability&timezone=auto";

  Serial.println("🌍 Consultando clima...");
  Serial.println(url);

  if(!https.begin(client, url)){
    Serial.println("❌ begin() falló");
    return;
  }

  int httpCode = https.GET();

  Serial.println("HTTP CODE: " + String(httpCode));

  if(httpCode > 0){

    String payload = https.getString();
    Serial.println("JSON recibido:");
    Serial.println(payload);
    DynamicJsonDocument doc(12288);

    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.print("❌ Error JSON: ");
      Serial.println(error.c_str());
      return;
    }

    struct tm timeinfo;
    int horaActual = 0;

    if(getLocalTime(&timeinfo)){
      horaActual = timeinfo.tm_hour;
    } else{
      Serial.println("Hora no disponible");
    horaActual = 0 ;
  }

    JsonArray lluvia = doc["hourly"]["precipitation_probability"];

    if(!lluvia.isNull() && horaActual < lluvia.size()){
      probabilidadLluvia = lluvia[horaActual];
    }else{
      Serial.println("⚠️ No se pudo leer lluvia");
      probabilidadLluvia = 0;
    }

    Serial.println("🕒 Hora: " + String(horaActual));
    Serial.println("🌧️ Prob lluvia: " + String(probabilidadLluvia));

  }else{
    Serial.println("❌ Error HTTP");
  }
  
  
  https.end();
}
bool obtenerCiudadPorCoordenadas(float lat, float lon){

  client.setInsecure();
  HTTPClient https;

  String url = "https://nominatim.openstreetmap.org/reverse?lat=" 
               + String(lat,6) + 
               "&lon=" + String(lon,6) + 
               "&format=json";

  Serial.println("🌍 Reverse geocoding...");
  Serial.println(url);

  if(!https.begin(client, url)){
    Serial.println("❌ begin() falló");
    return false;
  }

  https.addHeader("User-Agent", "ESP32");

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

    DynamicJsonDocument doc(8192);
    if(deserializeJson(doc, payload)){
      Serial.println("❌ Error JSON");
      https.end();
      return false;
    }

    if(doc["display_name"]){

      ciudad = doc["display_name"].as<String>();

      prefs.begin("config", false);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ Ciudad detectada:");
      Serial.println(ciudad);

      https.end();
      return true;

    }

  }

  https.end();
  return false;
}
bool buscarCoordenadasOSM(String ciudadBusqueda){

  client.setInsecure();
  HTTPClient https;

  ciudadBusqueda.trim();
  ciudadBusqueda.replace(" ", "%20");

  String url = "https://nominatim.openstreetmap.org/search?q=" 
               + ciudadBusqueda + 
               "&format=json&limit=1";

  Serial.println("🌍 OSM fallback...");
  Serial.println(url);

  if(!https.begin(client, url)){
    Serial.println("❌ begin() OSM falló");
    return false;
  }

  https.addHeader("User-Agent", "ESP32");

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.println("❌ JSON OSM error");
      https.end();
      return false;
    }

    if(doc.size() > 0){

      lat = String((const char*)doc[0]["lat"]).toFloat();
      lon = String((const char*)doc[0]["lon"]).toFloat();

      ciudad = doc[0]["display_name"].as<String>();

      prefs.begin("config", false);
      prefs.putFloat("lat", lat);
      prefs.putFloat("lon", lon);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ OSM encontró:");
      Serial.println(ciudad);

      consultarClima();

      https.end();
      return true;   // ENCONTRÓ

    }else{
      Serial.println("❌ OSM no encontró nada");
    }

  }else{
    Serial.println("❌ HTTP OSM error");
  }

  https.end();
  return false;   // ❌ NO encontró
}
bool buscarCoordenadas(String ciudadBusqueda){

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("❌ Sin WiFi");
    return false;
  }

  client.setInsecure();
  HTTPClient https;

  ciudadBusqueda.trim();
  ciudadBusqueda.replace(" ", "%20");

  String url = "https://geocoding-api.open-meteo.com/v1/search?name=" 
               + ciudadBusqueda + 
               "&count=3&language=es&format=json";

  Serial.println("🌍 Buscando ciudad...");
  Serial.println(url);

  if(!https.begin(client, url)){
    Serial.println("❌ begin() falló");
    return false;
  }

  int httpCode = https.GET();

  if(httpCode > 0){

    String payload = https.getString();

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if(error){
      Serial.println("❌ Error JSON");
      https.end();
      return false;
    }

    if(doc["results"].size() > 0){

      lat = doc["results"][0]["latitude"];
      lon = doc["results"][0]["longitude"];

      String nombre = doc["results"][0]["name"].as<String>();
      String pais = doc["results"][0]["country"].as<String>();

      ciudad = nombre + ", " + pais;

      prefs.begin("config", false);
      prefs.putFloat("lat", lat);
      prefs.putFloat("lon", lon);
      prefs.putString("ciudad", ciudad);
      prefs.end();

      Serial.println("✅ Ciudad encontrada:");
      Serial.println(ciudad);

      consultarClima();

      https.end();
      return true;   // ENCONTRÓ

    }else{

      Serial.println("⚠️ Open-Meteo falló → probando OSM");
      https.end();

      return buscarCoordenadasOSM(ciudadBusqueda);  // si falla
    }

  }else{
    Serial.println("❌ HTTP error");
  }

  https.end();
  return false;
}
bool esAdmin(String id){
  return id == CHAT_ID;
}
 void enviarATodos(String mensaje){

  // admin primero
  bot->sendMessage(CHAT_ID, mensaje, "");

  for(int i=0;i<cantidadUsuarios;i++){

    bool repetido = false;

    for(int j=0;j<i;j++){
      if(usuariosID[i] == usuariosID[j]){
        repetido = true;
        break;
      }
    }

    if(repetido) continue;

    // evito enviar al admin mensaje duplicado
    if(usuariosID[i] == CHAT_ID) continue;

    bot->sendMessage(usuariosID[i], mensaje, "");
  }
 }
 void controlMultiplesHorarios(int hora,int minuto){

  if(bloqueoFisicoActivo()){
  return;
}

  // pequeño bloqueo después de manual
  if(millis() - bloqueoManualTiempo < 5000){
    return;
  }

  int limite = modoTanqueAutomatico ? 6 : 7;

  //  CORTE EN TIEMPO REAL
  
  for(int r=0;r<limite;r++){

    if(reles[r].encendido && reles[r].duracion > 0){

      if(millis() - reles[r].inicio >= reles[r].duracion*1000){

        digitalWrite(relePin[r],HIGH);
        reles[r].encendido=false;

        reles[r].ultimoMinuto = minuto;

        // verifica si queda algún rele activo
        bool quedaActivo = false;

        for(int i=0;i<6;i++){
          if(reles[i].encendido){
            quedaActivo = true;
            break;
          }
        }


        // apagar bomba si no queda ninguno ACTIVO

        if(!quedaActivo){
          digitalWrite(relePin[7],HIGH);
          reles[7].encendido=false;
        }

        enviarATodos("✅ "+reles[r].nombre+" finalizado con exito!!");
      }
    }
  }

  static int ultimoMinutoGlobal = -1;

if(minuto != ultimoMinutoGlobal){

  ultimoMinutoGlobal = minuto;

  // ACTIVACION AUTOMATICA SOLO UNA VEZ POR MINUTO
 for(int r=0;r<limite;r++){

  if(!reles[r].habilitado) continue;

  for(int h=0;h<cantidadHorarios[r];h++){

    
  // BLOQUEO POR LLUVIA
if(probabilidadLluvia > 60){

  // 
  static int ultimoAvisoLluvia = -1;

  if(minuto != ultimoAvisoLluvia){
    ultimoAvisoLluvia = minuto;

    enviarATodos("🌧️ Riego cancelado por lluvia (" + String(probabilidadLluvia,1) + "%)");
    guardarEvento("Riego cancelado por lluvia " + String(probabilidadLluvia) + "%");
  }

  continue;
}
    if(!reles[r].encendido &&
       hora == horarios[r][h].hora &&
       minuto == horarios[r][h].minuto &&
       reles[r].ultimoMinuto != minuto){

      if(bloqueoHumedad && humedad > humedadLimite){

        if(!avisoHumedadEnviado){
          enviarATodos("⚠️ Riego cancelado\nHumedad alta: "+String(humedad,1)+"%");
          guardarEvento("Riego cancelado por humedad alta " + String(humedad) + "%");
          avisoHumedadEnviado=true;
        }

        continue;
      }

      reles[r].duracion = horarios[r][h].duracion;

      digitalWrite(relePin[r],LOW);
      reles[r].encendido=true;

      esperaBomba = true;
      timerBomba = millis();

      reles[r].inicio=millis();
      reles[r].ultimoMinuto=minuto;  // 👈 CLAVE
     
      enviarATodos("⚡ Activacion automatica: "+reles[r].nombre);
    }
  }
}
}

 }
void guardarUsuarios(){

prefs.begin("users", false);

prefs.putInt("cant", cantidadUsuarios);

for(int i=0;i<cantidadUsuarios;i++){
  prefs.putString(("id"+String(i)).c_str(), usuariosID[i]);
  prefs.putString(("nom"+String(i)).c_str(), usuariosNombre[i]);
}

prefs.end();
}
void ejecutarRele(int r, String origen, String usuario, String chat_id){

  bloqueoManualTiempo = millis();

  if(reles[r].encendido){

  digitalWrite(relePin[r], HIGH);
  reles[r].encendido = false;

  // 👇 IMPORTANTE: si no queda ningún rele activo → apagar bomba
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

  bot->sendMessage(chat_id,
  "🔴 " + reles[r].nombre + " apagado desde " + origen + " ("+usuario+")",
  "");
}else{

    digitalWrite(relePin[r],LOW);
    reles[r].encendido = true;

    
    esperaBomba = true;
    timerBomba = millis();

    reles[r].duracion = 0;
    reles[r].inicio = millis();

    bot->sendMessage(chat_id,
"🟢 " + reles[r].nombre + " encendido desde " + origen + " ("+usuario+")",
"");

  }
}
String sugerirComando(String txt){

  if(txt.indexOf("est")>=0) return "/estado";
  if(txt.indexOf("men")>=0) return "/menu_rapido";
  if(txt.indexOf("rel")>=0) return "/rele1";
  if(txt.indexOf("ho")>=0) return "/horarios";
  if(txt.indexOf("se")>=0) return "/sensores";
  if(txt.indexOf("tan")>=0) return "/tanque_si";
  if(txt.indexOf("ha")>=0) return "/habilitar_todos";
  if(txt.indexOf("de")>=0) return "/deshabilitar_todos";
  if(txt.indexOf("bo")>=0) return "/borrarnombres";
  if(txt.indexOf("to")>=0) return "/todo_off";
  if(txt.indexOf("re")>=0) return "/reiniciar";
  if(txt.indexOf("us")>=0) return "/usuarios";
  if(txt.indexOf("pa")>=0) return "/panel";

  return "";

}
void controlarBotonAUX(){

  static bool estadoAnterior = HIGH;
  static unsigned long tiempoPresionado = 0;
  static bool esperando = false;

  bool estadoActual = digitalRead(BottAUX);

  
  if(estadoActual == LOW && estadoAnterior == HIGH){
    tiempoPresionado = millis();
    esperando = true;
  }

  
  if(esperando && (millis() - tiempoPresionado >= 500)){
    esperando = false;

    if(bloqueoFisicoActivo()){
      enviarATodos("⛔ No se puede accionar - BLOQUEO ACTIVO");
      estadoAnterior = estadoActual; 
      return;
    }

    int r = 5;

bool estadoAntes = reles[r].encendido;
ejecutarRele(r, "botón físico", "LOCAL", CHAT_ID);
String nombreRele = reles[r].nombre;

if(estadoAntes){
  guardarEvento(nombreRele + " apagado desde boton fisico");
}else{
  guardarEvento(nombreRele + " encendido desde boton fisico");
}

  estadoAnterior = estadoActual;
  }
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
void reconectarWiFi(){  

if(WiFi.status()!=WL_CONNECTED){

WiFi.reconnect();

unsigned long start=millis();

while(WiFi.status()!=WL_CONNECTED && millis()-start<5000){
delay(500);
}

}

}
String textoWiFiEstado()
{
  if(WiFi.status()!=WL_CONNECTED) return "SIN WIFI";
  return String(obtenerCalidadWiFi())+"% ("+String(WiFi.RSSI())+" dBm)";
}
bool usuarioAutorizado(String id){

  if(id == CHAT_ID) return true;  // 👈 ADMIN dinámico

  for(int i=0;i<cantidadUsuarios;i++){
    if(id == usuariosID[i]) return true;
  }

  return false;
}
String obtenerNombreUsuario(String id){

  if(id == CHAT_ID) return "ADMIN";

  for(int i=0;i<cantidadUsuarios;i++){
    if(usuariosID[i] == id){
      return usuariosNombre[i];
    }
  }

  return "Desconocido";
}
void enviarMenuTelegram(String chat_id){

String menu;

menu += "     🌱 *RIEGO AUTOMÁTICO INTELIGENTE*\n";
menu += "                  🌐SolucionesIOT\n";
menu += "       ━━━━━━━━━━━━━━━━━━\n\n";

menu += "      *SISTEMA*\n";
menu += "📋  /menu\n";
menu += "⚡  /menu_rapido\n";
menu += "🔍  /historial\n";
menu += "🔄  /reiniciar\n\n";

menu += "      *ESTADO Y DATOS*\n";
menu += "📊  /estado - Estado general\n";
menu += "📡  /sensores - Temp / Humedad / WiFi\n";
menu += "🌧  /clima - Probabilidad de lluvia\n";
menu += "📅  /horarios - Ver riegos programados\n\n";

menu += "      *UBICACIÓN*\n";
menu += "🌍  /ciudad Nombre - Buscar ciudad\n";
menu += "📍  /ubicacion latit-long - Manual\n\n";

menu += "      *CONTROL MANUAL*\n";
menu += "🎛  /panel - Panel con botones\n";
menu += "⚡  /rele1 a /rele7 - Encender/Apagar\n";
menu += "⛔  /todo_off - Apagar todo\n\n";

menu += "      *PROGRAMACIÓN*\n";
menu += "⏰  /programar N HH:MM SEG\n";
menu += "🗑  /borrar N I - Borrar horario\n";
menu += "🧹  /borrarhorarios - Borrar todo\n\n";

menu += "      *HUMEDAD*\n";
menu += "💧  /humedad N - Límite de humedad\n\n";

menu += "      *TANQUE*\n";
menu += "🚰  /tanque_si - Automático\n";
menu += "🚫  /tanque_no - Manual\n\n";

menu += "      *RELES*\n";
menu += "✏️  /nombrerele N Nombre\n";
menu += "✅  /habilitar N\n";
menu += "⛔  /deshabilitar N\n";
menu += "🟢  /habilitar_todos\n";
menu += "🔴  /deshabilitar_todos\n";
menu += "🧹  /borrarnombres\n\n";

menu += "      *USUARIOS*\n";
menu += "👥  /usuarios\n";
menu += "➕  /autorizar ID Nombre\n";
menu += "➖  /eliminar ID\n";
menu += "❌ /borrarhistorial\n\n";

menu += "    ✔️ *TIP*\n";
menu += "➖El sistema bloquea riego si hay lluvia 🌧\n";
menu += "➖Mira hasta 100 eventos almacenados en historial \n";
bot->sendMessage(chat_id, menu, "");
}
void enviarMenuRapido(String chat_id){

String teclado =
"["
"[\"/estado\",\"/sensores\"],"
"[\"/horarios\",\"/panel\"],"
"[\"/todo_off\",\"/menu\"],"
"[\"/tanque_si\",\"/tanque_no\"]"
"]";

bot->sendMessageWithReplyKeyboard(
chat_id,
"📲 Accesos rápidos:",
"",
teclado,
true,
false
);

}
void enviarPanelReles(String chat_id)
{

String teclado;

if(modoTanqueAutomatico){

  // ❌ SIN rele 7
  teclado =
  "["
  "[\"" + reles[0].nombre + "\",\"" + reles[1].nombre + "\"],"
  "[\"" + reles[2].nombre + "\",\"" + reles[3].nombre + "\"],"
  "[\"" + reles[4].nombre + "\",\"" + reles[5].nombre + "\"]"
  "]";

}else{

  // ✅ CON rele 7
  teclado =
  "["
  "[\"" + reles[0].nombre + "\",\"" + reles[1].nombre + "\"],"
  "[\"" + reles[2].nombre + "\",\"" + reles[3].nombre + "\"],"
  "[\"" + reles[4].nombre + "\",\"" + reles[5].nombre + "\"],"
  "[\"" + reles[6].nombre + "\"]"
  "]";

}

bot->sendMessageWithReplyKeyboard(
chat_id,
"🎛 PANEL DE CONTROL\nSelecciona un rele:",
"",
teclado,
true,
false
);

}
void controlarWiFi()
{

bool estadoActual = WiFi.status()==WL_CONNECTED;

if(estadoActual != wifiEstadoAnterior){

if(!estadoActual){

bot->sendMessage(CHAT_ID,"⚠️ WiFi desconectado","");

}else{

bot->sendMessage(CHAT_ID,"✅ WiFi reconectado","");

}

wifiEstadoAnterior=estadoActual;

}

}
void controlarTanque(){
if(bloqueoActivoGlobal){
  return;
}

if(!modoTanqueAutomatico){
  return; // NO hace nada si está en modo manual
}



bool pedidoAgua = digitalRead(SENSOR_TANQUE) == LOW; // flotante

/* verificar si hay riego activo (1-6) */
bool riegoActivo=false;

for(int i=0;i<6;i++){
  if(reles[i].encendido){
    riegoActivo=true;
    break;
  }
}

/* SI HAY RIEGO → usar bomba normal (NO tanque) */
if(riegoActivo){

  // 🔴 APAGAR SOLO el tanque (rele 7)
  if(reles[6].encendido){
    digitalWrite(relePin[6],HIGH);
    reles[6].encendido=false;
  }

  // 🟢 ASEGURAR que la bomba esté prendida
  if(!reles[7].encendido){
    digitalWrite(relePin[7],LOW);
    reles[7].encendido=true;
  }

  return;
}

/* SI HAY PEDIDO DE AGUA */
if(pedidoAgua){

/* activar rele 7 (tanque) */
if(!reles[6].encendido){

digitalWrite(relePin[6],LOW);
reles[6].encendido=true;
delay(1000); 
}

/* activar bomba */
if(!reles[7].encendido){

digitalWrite(relePin[7],LOW);
reles[7].encendido=true;

}

}else{

/* apagar bomba primero */
if(reles[7].encendido){

digitalWrite(relePin[7],HIGH);
reles[7].encendido=false;
delay(1000); 
}

/* luego tanque */
if(reles[6].encendido){

digitalWrite(relePin[6],HIGH);
reles[6].encendido=false;

}

}

}
void iniciarWifi()
{

prefs.begin("config",true);

String savedToken=prefs.getString("token","");
String savedChat=prefs.getString("chatid","");

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
void controlarBomba(){
  
  if(bloqueoActivoGlobal){
    return;
  }

  if(millis() - bloqueoBombaManual < 3000){
  return;
  }
 
  if(reles[6].encendido){
    return;
  }

  static unsigned long bloqueoBomba = 0;

  if(millis() - bloqueoBomba < 2000){
    return;
  }

  bool algunReleActivo=false;

  int limite = modoTanqueAutomatico ? 6 : 7;

  for(int i=0;i<limite;i++){
    if(reles[i].encendido){
      algunReleActivo=true;
      break;
    }
  }

  // ENCENDER
  if(algunReleActivo && !reles[7].encendido && !esperandoApagadoRele){
    digitalWrite(relePin[7],LOW);
    reles[7].encendido=true;
  }

  // APAGAR
  if(!algunReleActivo && reles[7].encendido){
    digitalWrite(relePin[7],HIGH);
    reles[7].encendido=false;
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
void borrarHistorial() {

  prefs.begin("eventos", false); 
  prefs.clear();
  prefs.end();

  // limpiar RAM también
  for(int i=0;i<MAX_EVENTOS;i++){
    eventos[i] = "";
  }

  indiceEvento = 0;
}
void leerSensor()
{

temperatura=dht.readTemperature();
humedad=dht.readHumidity();

/* RESET BLOQUEO HUMEDAD */
if(humedad <= humedadLimite && avisoHumedadEnviado){

bot->sendMessage(CHAT_ID,"✅ Humedad normal, riego habilitado","");

avisoHumedadEnviado=false;

}

}
void actualizarHora()
{

struct tm timeinfo;
if(!getLocalTime(&timeinfo)) return;

static int minutoPrevio = -1;

if(timeinfo.tm_min != minutoPrevio){

char buffer[6];

sprintf(buffer,"%02d:%02d",
timeinfo.tm_hour,
timeinfo.tm_min);
minutoPrevio = timeinfo.tm_min;

}

}
void manejarTelegram(){
client.setTimeout(1500);
int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

while(numNewMessages){

for(int i=0;i<numNewMessages;i++){

String text = bot->messages[i].text;
text.trim();

String chat_id = String(bot->messages[i].chat_id);
bool comandoValido = false;

static unsigned long ultimoAviso = 0;

if(bloqueoFisicoActivo()){

  if(millis() - ultimoAviso > 5000){
    
    bot->sendMessage(chat_id,
    "⛔ Sistema bloqueado por seguridad física",
    "");
    ultimoAviso = millis();
  }

  continue;
}

if(millis() - bloqueoArranque < 8000){
continue;
}
if(!usuarioAutorizado(chat_id)){

  bot->sendMessage(chat_id,
  "⛔ Usuario no autorizado\n\n"
  "🆔 ID: " + chat_id + "\n"
  "👉 Solicita acceso al administrador",
  "");

  bot->sendMessage(CHAT_ID,
  "🚫 Intento de acceso\nID: " + chat_id,
  "");

  continue;
}


/* convertir nombre de rele en comando */
for(int r=0;r<7;r++){
  if(text == reles[r].nombre){
    text="/rele"+String(r+1);
    comandoValido = true;
  }
}

if(text.startsWith("/ciudad")){

  comandoValido = true;

  char nueva[60];

  int resultado = sscanf(text.c_str(), "/ciudad %[^\n]", nueva);

  if(resultado == 1){

    String ciudadIngresada = String(nueva);

    bot->sendMessage(chat_id,
    "🌍 Buscando ciudad...\n" + ciudadIngresada,
    "");

    bool ok = buscarCoordenadas(ciudadIngresada);

    if(ok){
       String nombre = obtenerNombreUsuario(chat_id);
       guardarEvento(nombre + "actualizo UBICACION. ");  // almaceno accion y nombre en histrial 
      bot->sendMessage(chat_id,
      "✅ Ciudad encontrada:\n📍 " + ciudad,
      "");

    }else{

      bot->sendMessage(chat_id,
      "❌ No se encontró la ciudad:\n" + ciudadIngresada,
      "");

    }

  }else{

    bot->sendMessage(chat_id,
    "⚠️ Uso: /ciudad (ciudad ubicacion)",
    "");

  }
}

if(text == "/clima"){

comandoValido = true;

consultarClima();  // SEGUNDA ACTUALIZACON

String msg;

msg += "🌧️ *CLIMA*\n";
msg += "━━━━━━━━━━━━━━━━━━\n\n";
msg += "📍 " + ciudad + "\n";
msg += "🌧️ Probabilidad: " + String(probabilidadLluvia,1) + " %\n\n";

Serial.println("CLIMA ACTUAL:");
Serial.println(probabilidadLluvia);

if(probabilidadLluvia > 60){
  msg += "⛔ Riego BLOQUEADO por lluvia";
}else{
  msg += "✅ Riego permitido";
}

bot->sendMessage(chat_id, msg, "Markdown");
}

if(text.startsWith("/ubicacion")){

comandoValido = true;

float nuevaLat, nuevaLon;

int ok = sscanf(text.c_str(), "/ubicacion %f %f", &nuevaLat, &nuevaLon);

if(ok == 2){

lat = nuevaLat;
lon = nuevaLon;

prefs.begin("config", false);
prefs.putFloat("lat", lat);
prefs.putFloat("lon", lon);
prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);
 guardarEvento(nombre + "actualizo UBICACION. ");  // almaceno accion y nombre en histrial 
bot->sendMessage(chat_id,
"📍 Ubicación guardada\nLat: " + String(lat,6) +
"\nLon: " + String(lon,6),
"");

}else{

bot->sendMessage(chat_id,
"⚠️ Uso: /ubicacion -34.60 -58.38",
"");

}
}
// ===== BORRAR HISTORIAL =====

if (text == "/borrarhistorial") {

  comandoValido = true;

  if (!esAdmin(chat_id)) {
    bot->sendMessage(chat_id, "⛔ Solo el admin puede borrar el historial", "");
    return;
  }

  bot->sendMessage(chat_id,
  "⚠️ Confirmar borrado\nEscribí: /confirmarborrado",
  "");

  return;
}

if (text == "/confirmarborrado") {

  comandoValido = true;

  if (!esAdmin(chat_id)) {
    bot->sendMessage(chat_id, "⛔ No autorizado", "");
    return;
  }

  borrarHistorial();

  bot->sendMessage(chat_id,
  "🗑️ Historial borrado correctamente",
  "");

  return;
}

if(text=="/menu"){
enviarMenuTelegram(chat_id);
comandoValido = true;
}

if(text=="/menu_rapido"){
enviarMenuRapido(chat_id);
comandoValido = true;
}

///// panel -///////
if(text=="/panel"){
enviarPanelReles(chat_id);
comandoValido = true;
}

// ---- MODO TANQUE ----

if(text == "/tanque_si"){
  comandoValido = true;

  modoTanqueAutomatico = true;

  // nombre automático
  reles[6].nombre = "TANQUE";

  // guardar nombre
  prefs.begin("reles", false);
  prefs.putString("nombre6", "TANQUE");
  prefs.end();

  // guardar modo
  prefs.begin("config", false);
  prefs.putBool("modoTanque", modoTanqueAutomatico);
  prefs.end();

  //  RESET TOTAL DEL TANQUE
  digitalWrite(relePin[6], HIGH);
  reles[6].encendido = false;

  //  APAGAR BOMBA (se re-evaluará sola)
  digitalWrite(relePin[7], HIGH);
  reles[7].encendido = false;

  //  pequeño bloqueo para evitar rebotes automáticos
  bloqueoBombaManual = millis();
   String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " habilita Modo tanque AUTOMATICO.");  // almaceno accion y nombre en histrial 
  bot->sendMessage(chat_id,
  "⚠️ Tanque AUTOMATICO ACTIVADO\nControl por flotante habilitado",
  "");
}

if(text == "/tanque_no"){
  comandoValido = true;

  modoTanqueAutomatico = false;

  // nombre normal
  String nombreDefault = "Rele 7";
  reles[6].nombre = nombreDefault;

  // guardar config
  prefs.begin("reles", false);
  prefs.putString("nombre6", nombreDefault);
  prefs.end();

  prefs.begin("config", false);
  prefs.putBool("modoTanque", modoTanqueAutomatico);
  prefs.end();

  //  APAGAR TANQUE SI ESTABA ACTIVO
  if(reles[6].encendido){
    digitalWrite(relePin[6], HIGH);
    reles[6].encendido = false;
  }

  // APAGAR BOMBA SI ESTABA POR TANQUE
  if(reles[7].encendido){

    bool hayRiego = false;

    for(int i=0;i<6;i++){
      if(reles[i].encendido){
        hayRiego = true;
        break;
      }
    }

    // solo apagar si NO hay riego activo
    if(!hayRiego){
      digitalWrite(relePin[7], HIGH);
      reles[7].encendido = false;
    }
  }

  //  BLOQUEO CORTO PARA EVITAR REENCENDIDO AUTOMATICO
  bloqueoBombaManual = millis();

  String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " deshabilita Modo tanque AUTOMATICO.");  // almaceno accion y nombre en histrial 
  
  bot->sendMessage(chat_id,
  "⚠️ Tanque AUTOMATICO DESACTIVADO\n",
  "");
}


if(text=="/borrarnombres"){
comandoValido = true;
prefs.begin("reles", false);

for(int i=0;i<8;i++){

/*  SI ES RELE 7 Y ESTA EN MODO TANQUE → NO TOCAR */
if(i==6 && modoTanqueAutomatico){
  continue;
}

String nombreDefault = "Rele " + String(i+1);

/* actualizar en memoria */
reles[i].nombre = nombreDefault;

/* guardar en flash */
prefs.putString(("nombre"+String(i)).c_str(), nombreDefault);

}

prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);
guardarEvento(nombre + " realizo borrado de nombres almacenados.");  // almaceno accion y nombre en histrial 
bot->sendMessage(chat_id,"🗑 Nombres de todos los Rele reiniciados correctamente","");


} 


if(text=="/habilitar_todos"){
comandoValido = true;
prefs.begin("reles",false);

for(int i=0;i<8;i++){
  reles[i].habilitado=true;
  prefs.putBool(("hab"+String(i)).c_str(),true);
}

prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);
 guardarEvento(nombre + " habilito todos los Rele ");  // almaceno accion y nombre en histrial 
bot->sendMessage(chat_id,"✅ Todos los Rele habilitados","");
}


if(text=="/deshabilitar_todos"){
comandoValido = true;
prefs.begin("reles",false);

for(int i=0;i<8;i++){

  if(i==7) continue; // bomba protegida

  //  NO toca tanque si está en automático
  if(i==6 && modoTanqueAutomatico) continue;

  reles[i].habilitado=false;
  prefs.putBool(("hab"+String(i)).c_str(),false);
}

prefs.end();
  String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " deshabilito todos los Rele. ");  // almaceno accion y nombre en histrial
bot->sendMessage(chat_id,"⛔ Todos los Rele deshabilitados","");
}

/////// reiniciar sistema -///////
if(text=="/reiniciar"){
comandoValido = true;
bot->sendMessage(chat_id,"🔄 Reiniciando sistema...","");
 String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " reinicia el sistema. ");  // almaceno accion y nombre en histrial 
reinicioPendiente = true;
timerReinicio = millis();

/* limpiar mensajes pendientes */
bot->last_message_received = bot->messages[i].update_id;

}

/* ===== BORRAR TODOS LOS HORARIOS ===== */

if(text=="/borrarhorarios"){
comandoValido = true;
for(int r=0;r<7;r++){

cantidadHorarios[r]=0;

for(int h=0;h<MAX_HORARIOS;h++){

horarios[r][h].hora=0;
horarios[r][h].minuto=0;
horarios[r][h].duracion=0;

}

}

/* guardar cambios en memoria */
guardarHorarios();
 String nombre = obtenerNombreUsuario(chat_id);
 guardarEvento(nombre + " borra todos los horarios almacenados. ");  // almaceno accion y nombre en histrial 
bot->sendMessage(chat_id,"🗑  Todos los horarios fueron borrados. Sistema reiniciado de programacion.","");

}
// -------- ESTADO --------

if(text=="/estado"){
comandoValido = true;
String mensaje;

mensaje+="📊 *ESTADO DEL SISTEMA*\n";
mensaje+="━━━━━━━━━━━━━━━━━━\n\n";

mensaje+="🌡 Temperatura: "+String(temperatura,1)+" °C\n\n";
mensaje+="💧 Humedad: "+String(humedad,1)+" %\n\n";
mensaje+="📶 WiFi: "+textoWiFiEstado()+"\n\n";
mensaje+="🚫 Limite humedad: "+String(humedadLimite)+" %\n\n";
mensaje+="🚰 Modo tanque: ";

if(modoTanqueAutomatico){
mensaje+="🟢 AUTOMATICO \n";
mensaje+="(control por flotante)\n\n";
}
else{
mensaje+="⛔ DESACTIVADO \n";
}
mensaje+="\n━━━━━━━━━━━━━━━━━━\n\n";

mensaje+="🔌 *RELES*\n\n";

for(int r=0;r<8;r++){

  //  RELE 8 = BOMBA
  if(r==7){

    mensaje+="🚰 BOMBA\n";

    mensaje+="Estado: ";
    if(reles[7].encendido)
      mensaje+="🟢 ON\n";
    else
      mensaje+="⚪ OFF\n";

    mensaje+="Modo: ⚙️ AUTOMATICO (Por valvulas activas)\n\n";

    continue;
  }

  // RELE 7 = TANQUE
  if(r==6){

    mensaje+="💧 "+reles[r].nombre+"\n";

    mensaje+="Estado: ";
    if(reles[r].encendido)
      mensaje+="🟢 ON\n";
    else
      mensaje+="⚪ OFF\n";

    mensaje+="Control: ";

    if(modoTanqueAutomatico){
      // SIEMPRE HABILITADO EN AUTOMATICO
      mensaje+="🟢 AUTOMATICO (Flotante)\n";
    }else{
      //  comportamiento normal
      if(reles[r].habilitado)
        mensaje+="🟢 HABILITADO\n";
      else
        mensaje+="🔴 DESHABILITADO\n";
    }

    mensaje+="\n";
    continue;
  }

  // RELES NORMALES
  mensaje+="💧 "+reles[r].nombre+"\n";

  mensaje+="Estado: ";
  if(reles[r].encendido)
    mensaje+="🟢 ON\n";
  else
    mensaje+="⚪ OFF\n";

  mensaje+="Control: ";
  if(reles[r].habilitado)
    mensaje+="🟢 HABILITADO\n";
  else
    mensaje+="🔴 DESHABILITADO\n";

  mensaje+="\n";
}

bot->sendMessage(chat_id,mensaje,"Markdown");


}


if(text.startsWith("/autorizar")){
comandoValido = true;
if(chat_id != CHAT_ID){
  bot->sendMessage(chat_id,"⛔ Solo admin","");
  return;
}

char id[20];
char nombre[30];

int ok = sscanf(text.c_str(), "/autorizar %s %[^\n]", id, nombre);

if(ok >= 2){

String nuevoID = String(id);
String nuevoNombre = String(nombre);

/* evitar duplicados */
for(int i=0;i<cantidadUsuarios;i++){
  if(usuariosID[i] == nuevoID){
    bot->sendMessage(chat_id,"⚠️ Usuario ya existe","");
    return;
  }
}

if(cantidadUsuarios < MAX_USERS){

usuariosID[cantidadUsuarios] = nuevoID;
usuariosNombre[cantidadUsuarios] = nuevoNombre;

cantidadUsuarios++;

guardarUsuarios();

 String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento( nombre + " autorizo usuario " + nuevoNombre + " (" + nuevoID + ")");  // almaceno accion y nombre en histrial 

bot->sendMessage(chat_id,
"✅ Usuario autorizado\n👤 "+nuevoNombre+"\n🆔 "+nuevoID,
"");

}else{
bot->sendMessage(chat_id,"⛔ Maximo usuarios alcanzado","");
}

}else{
bot->sendMessage(chat_id,"⚠️ Uso:\n/autorizar ID Nombre","");
}

}

if(text=="/usuarios"){
comandoValido = true;

if(cantidadUsuarios == 0){

  bot->sendMessage(chat_id,
  "⚠️ No hay usuarios cargados\n\n"
  "👉 El sistema aún no tiene usuarios autorizados\n"
  "👉 Usá: /autorizar ID Nombre",
  "");

  return;
}

String lista = "👥 Usuarios autorizados:\n\n";

for(int i=0;i<cantidadUsuarios;i++){
  lista += "👤 "+usuariosNombre[i]+" → "+usuariosID[i]+"\n";
}

bot->sendMessage(chat_id, lista, "");
}


if(text.startsWith("/eliminar")){
comandoValido = true;

if(chat_id != CHAT_ID){
  bot->sendMessage(chat_id,"⛔ Solo admin","");
  return;
}

char id[20];
int ok = sscanf(text.c_str(), "/eliminar %s", id);

if(ok == 1){

String eliminarID = String(id);
String nombreAdmin = obtenerNombreUsuario(chat_id);

for(int i=0;i<cantidadUsuarios;i++){

if(usuariosID[i] == eliminarID){

String nombreEliminado = usuariosNombre[i];  // guardo nombre antes

for(int j=i;j<cantidadUsuarios-1;j++){
usuariosID[j] = usuariosID[j+1];
usuariosNombre[j] = usuariosNombre[j+1];
}

cantidadUsuarios--;

guardarUsuarios();

bot->sendMessage(chat_id,"🗑 Usuario eliminado","");

guardarEvento( nombreAdmin + " elimino a " + nombreEliminado);  // almaceno accion y nombre en histrial

return;
}

}

bot->sendMessage(chat_id,"⚠️ Usuario no encontrado","");
}
}


/* ===== NUEVO COMANDO /HORARIOS ===== */
if(text=="/horarios"){
comandoValido = true;
String mensaje;

mensaje+="🌱 *PROGRAMACION DE RIEGO*\n";
mensaje+="━━━━━━━━━━━━━━━━━━\n\n";

for(int r=0;r<8;r++){

/* RELE 8 = BOMBA */
if(r==7){

mensaje+="🚰 *BOMBA*\n";
mensaje+="   ⚙️ Activacion automatica\n";
mensaje+="   Depende de valvulas activas.\n\n";

continue;
}

/* RELE 7 = TANQUE */
if(r==6){

mensaje+="💧 *"+reles[r].nombre+"*\n";
if(modoTanqueAutomatico){

mensaje+="   ⚙️ Flotante automatico activado\n";
mensaje+="---------------------------------------\n";
}else{

if(cantidadHorarios[r]==0){

mensaje+="   🚫 Sin horarios\n";
mensaje+="--------------------------------------\n";

}else{

for(int h=0;h<cantidadHorarios[r];h++){

mensaje+="   ⏰ ";

if(horarios[r][h].hora<10) mensaje+="0";
mensaje+=String(horarios[r][h].hora);

mensaje+=":";

if(horarios[r][h].minuto<10) mensaje+="0";
mensaje+=String(horarios[r][h].minuto);

mensaje+="  //  ⏱ ";
mensaje+=String(horarios[r][h].duracion);
mensaje+=" seg\n";
}
mensaje+="--------------------------------------\n";
}

}

continue;
}

/* RELES 1–6 (TODO IGUAL QUE ANTES) */

mensaje+="💧 *"+reles[r].nombre+"*\n";

if(cantidadHorarios[r]==0){

mensaje+="   🚫 Sin horarios\n";
mensaje+="--------------------------------------\n";
continue;

}

for(int h=0;h<cantidadHorarios[r];h++){

mensaje+="   ⏰ ";

if(horarios[r][h].hora<10) mensaje+="0";
mensaje+=String(horarios[r][h].hora);

mensaje+=":";

if(horarios[r][h].minuto<10) mensaje+="0";
mensaje+=String(horarios[r][h].minuto);

mensaje+="  //  ⏱";
mensaje+=String(horarios[r][h].duracion);
mensaje+=" seg\n";

}
mensaje+="--------------------------------------\n";

}

mensaje+="━━━━━━━━━━━━━━━━━━";

bot->sendMessage(chat_id,mensaje,"Markdown");

}


/* ===== COMANDO HUMEDAD ===== */

if(text.startsWith("/humedad")){

int valor;
int ok = sscanf(text.c_str(), "/humedad %d",&valor);

if(ok == 1 && valor>0 && valor<=100){

comandoValido = true;

float anterior = humedadLimite;

humedadLimite = valor;

prefs.begin("config",false);
prefs.putFloat("humedadLimite",humedadLimite);
prefs.end();

String nombre = obtenerNombreUsuario(chat_id);
guardarEvento(
  nombre + " cambio humedad de " + String(anterior) + "% a " + String(valor) + "%");

bot->sendMessage(chat_id,"⛔ Nuevo limite humedad: "+String(valor)+"%","");

}else{

bot->sendMessage(chat_id,"⚠️ Uso correcto: /humedad 1 a 100","");

}

}

if(text=="/historial"){
comandoValido = true;

String msg = "📜 *ULTIMOS HISTORIAL DE EVENTOS*\n";
msg += "━━━━━━━━━━━━━━━━━━\n\n";

int contador = 0;

// mostrar desde el más nuevo hacia atrás
for(int i=0;i<MAX_EVENTOS;i++){

  int idx = (indiceEvento - 1 - i + MAX_EVENTOS) % MAX_EVENTOS;

  if(eventos[idx] != ""){
    msg += eventos[idx] + "\n";
    contador++;
  }
}

if(contador == 0){
  msg += "⚠️ Sin eventos registrados";
}

bot->sendMessage(chat_id, msg, "Markdown");
}


/* ===== CAMBIAR NOMBRE RELE ===== */

if(text.startsWith("/nombrerele")){
comandoValido = true;
int numero;
char nombre[30];

sscanf(text.c_str(), "/nombrerele %d %[^\n]", &numero, nombre);

numero--;

/* 🚫 bloquear rele 7 en modo tanque */
if(numero==6 && modoTanqueAutomatico){

bot->sendMessage(chat_id,
"⛔ No se puede cambiar nombre del rele 7 en modo tanque automatico",
"");

return;
}

if(numero>=0 && numero<7){

reles[numero].nombre = String(nombre);

prefs.begin("reles",false);
prefs.putString(("nombre"+String(numero)).c_str(),reles[numero].nombre);
prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
 guardarEvento(nombre + " cambio nombre de Rele " + String(numero+1) + " a " + reles[numero].nombre);  // almaceno accion y nombre en histrial 
bot->sendMessage(
chat_id,
"✅ Nombre actualizado: Rele "+String(numero+1)+" → "+reles[numero].nombre,
""
);

}

}

/* ===== COMANDO SENSORES ===== */

if(text=="/sensores"){
comandoValido = true;
String msg;

msg+="📡 *SENSORES*\n";
msg+="━━━━━━━━━━━━━━━━━━\n\n";

msg+="🌡 Temperatura\n";
msg+="→ "+String(temperatura,1)+" °C\n\n";

msg+="💧 Humedad\n";
msg+="→ "+String(humedad,1)+" %\n\n";

msg+="📶 WiFi\n";
msg+="→ "+textoWiFiEstado()+"\n";

msg+="━━━━━━━━━━━━━━━━━━";

bot->sendMessage(chat_id,msg,"Markdown");

}
// -------- CONTROL MANUAL TOGGLE --------
for(int r=0;r<7;r++){

  String cmd="/rele"+String(r+1);

  if(text==cmd){
    comandoValido = true;

    String nombre = obtenerNombreUsuario(chat_id);
    ejecutarRele(r, "Telegram", nombre,chat_id);
  }
}

// -------- HABILITAR --------

if(text.startsWith("/habilitar ")){

int numero;
int ok = sscanf(text.c_str(), "/habilitar %d",&numero);

if(ok == 1 && numero>=1 && numero<=8){

comandoValido = true;  

numero--;

reles[numero].habilitado=true;

prefs.begin("reles",false);
prefs.putBool(("hab"+String(numero)).c_str(),true);
prefs.end();

String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(nombre + " habilita Rele" + String(numero+1));   /// almacenamos la info en el historial

bot->sendMessage(chat_id,"✅ Rele "+String(numero+1)+" habilitado","");

}else{

bot->sendMessage(chat_id,"⚠️ Uso correcto: /habilitar 1 a 8","");

}

}

// -------- DESHABILITAR --------

if(text.startsWith("/deshabilitar ")){

int numero;
int ok = sscanf(text.c_str(), "/deshabilitar %d",&numero);

if(ok == 1 && numero>=1 && numero<=8){

comandoValido = true;  


numero--;

if(numero==7){
bot->sendMessage(chat_id,"⛔ Rele 8 protegido","");
return;
}

reles[numero].habilitado=false;

prefs.begin("reles",false);
prefs.putBool(("hab"+String(numero)).c_str(),false);
prefs.end();

bot->sendMessage(chat_id,"⛔ Rele "+String(numero+1)+" deshabilitado","");
String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(nombre + " deshabilita Rele" + String(numero+1));   /// almacenamos la info en el historial

}else{

bot->sendMessage(chat_id,"⚠️ Uso correcto: /deshabilitar 1 a 8","");

}

}

//-------------APAGAR TODOS LOS RELES ---------

if(text=="/todo_off"){
comandoValido = true;
String msg;


for(int r=0;r<8;r++){

digitalWrite(relePin[r],HIGH);
reles[r].encendido=false;}
delay((500));
String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(nombre + " apago todos los Rele ");   /// almacenamos la info en el historial
bot->sendMessage(chat_id,"⛔ Todos los reles apagados manualmente","");
bot->last_message_received = bot->messages[i].update_id;
}




// -------- PROGRAMAR --------

if(text.startsWith("/programar")){
comandoValido = true;
int numero,hora,minuto,duracion;

sscanf(text.c_str(), "/programar %d %d:%d %d",&numero,&hora,&minuto,&duracion);

numero--;

// 🚫 bloquear rele 7 en modo tanque automatico
if(numero == 6 && modoTanqueAutomatico){

  bot->sendMessage(chat_id,
  "⛔ No se puede programar Rele 7 en modo tanque automatico",
  "");

  return;
} 


if(numero>=0 && numero<7){

if(cantidadHorarios[numero]<MAX_HORARIOS){

horarios[numero][cantidadHorarios[numero]].hora=hora;
horarios[numero][cantidadHorarios[numero]].minuto=minuto;
horarios[numero][cantidadHorarios[numero]].duracion=duracion;

cantidadHorarios[numero]++;

guardarHorarios();

String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(
  nombre + " programo" " (" + reles[numero].nombre + ")" +         ///////////almacenamos datos para el historial
  " a las " + String(hora) + ":" + (minuto<10?"0":"") + String(minuto) +
  " por " + String(duracion) + " seg"
);
bot->sendMessage(chat_id,"✅ Horario agregado correctamente","");

}else{

bot->sendMessage(chat_id,"⛔ Maximo 5 horarios correctamente","");

}

}

}

// -------- BORRAR --------

if(text.startsWith("/borrar")){
comandoValido = true;
int rele,indice;

sscanf(text.c_str(), "/borrar %d %d",&rele,&indice);

rele--;
indice--;

if(rele>=0 && rele<7 && indice<cantidadHorarios[rele]){

  
int h = horarios[rele][indice].hora;
int m = horarios[rele][indice].minuto;
int d = horarios[rele][indice].duracion;


for(int i=indice;i<cantidadHorarios[rele]-1;i++){

horarios[rele][i]=horarios[rele][i+1];

}
cantidadHorarios[rele]--;

guardarHorarios();

String nombre = obtenerNombreUsuario(chat_id);

guardarEvento(nombre + " borro horario de " " (" + reles[rele].nombre + ")" +
  " a las " + String(h) + ":" + (m<10?"0":"") + String(m) +
  " por " + String(d) + " seg");

String msg;

msg += "🗑 *HORARIO BORRADO*\n";
msg += "━━━━━━━━━━━━━━━━━━\n\n";

msg += "💧 " + reles[rele].nombre + "\n";

msg += "⏰ ";
if(h < 10) msg += "0";
msg += String(h);
msg += ":";

if(m < 10) msg += "0";
msg += String(m);

msg += "\n";

msg += "⏱ " + String(d) + " seg\n";

bot->sendMessage(chat_id, msg, "Markdown");

}

}


if(!comandoValido){

String sugerencia = sugerirComando(text);

if(sugerencia != ""){
bot->sendMessage(chat_id,
"❓ Comando no reconocido\n👉 Quizás quisiste decir: "+sugerencia,
"");
}else{
bot->sendMessage(chat_id,
"❓ Comando no reconocido\n👉 Escribí /menu",
"");
}

}


}

numNewMessages = bot->getUpdates(bot->last_message_received + 1);

}
}
void setup()
{
Serial.begin(9600);
Serial.println("TEST SERIAL");
setColor(0,0,0);
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
pinMode(LED_R, OUTPUT);
pinMode(LED_G, OUTPUT);
pinMode(LED_B, OUTPUT);
pinMode(BottAUX,INPUT);
pinMode(Bottreset,INPUT_PULLUP);
pinMode(BottOFF,INPUT_PULLUP);
pinMode(SENSOR_TANQUE,INPUT_PULLUP);

prefs.begin("eventos", true);

indiceEvento = prefs.getInt("indice", 0);

for(int i=0;i<MAX_EVENTOS;i++){
  eventos[i] = prefs.getString(("e"+String(i)).c_str(), "");
}

prefs.end();

/* SENSOR */
dht.begin();

/* RELES */
iniciarReles();

/* WIFI */
iniciarWifi();

if(WiFi.status()==WL_CONNECTED){
}
/* TELEGRAM */
client.setInsecure();
client.setTimeout(2000);

bot=new UniversalTelegramBot(BOTtoken,client);

/* CONFIGURACION */
prefs.begin("config",true);
humedadLimite = prefs.getFloat("humedadLimite",90);
modoTanqueAutomatico = prefs.getBool("modoTanque", true);
prefs.end();

prefs.begin("config", true);
ciudad = prefs.getString("ciudad", "Buenos Aires");
prefs.end();

/* SINCRONIZAR HORA */
configTime(gmtOffset_sec,0,ntpServer);

struct tm timeinfo;

int intentos=0;

while(!getLocalTime(&timeinfo) && intentos<10){
delay(500);
intentos++;
}

/* CARGAR DATOS */
cargarHorarios();
leerSensor();
consultarClima();

bloqueoArranque = millis();

enviarATodos("✅ Sistema iniciado correctamente");
  actualizarLED();


}
void loop()
{

  unsigned long now = millis();

  // BLOQUEO FISICO
  static bool bloqueoAnterior = false;
  bool bloqueoActual = bloqueoFisicoActivo();

  if(bloqueoActual != bloqueoAnterior){

    if(bloqueoActual){

      bloqueoActivoGlobal = true;

      for(int r=0;r<8;r++){
        digitalWrite(relePin[r], HIGH);
        reles[r].encendido = false;
      }

      esperandoApagadoRele = false;
      esperaBomba = false;
      bloqueoBombaManual = millis();
      guardarEvento("⛔ BLOQUEO FISICO ACTIVADO\nSistema detenido ");
      enviarATodos("⛔ BLOQUEO FISICO ACTIVADO\nSistema detenido");

    }else{

      bloqueoActivoGlobal = false;
      bloqueoBombaManual = millis();
      guardarEvento("✅ BLOQUEO DESACTIVADO\nSistema reanudado");
      enviarATodos("✅ BLOQUEO DESACTIVADO\nSistema reanudado");
    }

    bloqueoAnterior = bloqueoActual;
  }

  // parpadeo LED
  if(bloqueoActual){

    static unsigned long t = 0;
    static bool estado = false;

    if(millis() - t >= 300){
      t = millis();
      estado = !estado;
    }

    setColor(estado,0,0);

  }else{
    actualizarLED();
  }

  // tareas siguientes

  controlarBotonAUX();
  controlarTanque();
  controlarBomba();


  if(now - timerClock > 1000){
    timerClock = now;
    actualizarHora();
  }

  if(now - timerSensor > 2000){   
    timerSensor = now;
    leerSensor();
  }

  // CONTROL DE RIEGO

  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    controlMultiplesHorarios(timeinfo.tm_hour, timeinfo.tm_min);
  }

  // wifi

  static unsigned long timerWiFi = 0;

  if(now - timerWiFi > 3000){
    timerWiFi = now;
    reconectarWiFi();
    controlarWiFi();
  }

  // telegram

  if(now - timerTelegram > 5000){   
    timerTelegram = now;

    if(WiFi.status() == WL_CONNECTED){
      manejarTelegram();
    }
  }

  // clima

  if(now - timerClima > 900000){   // ⬅️ 15 minutos
    timerClima = now;
    consultarClima();
  }

  // retardo bomba 

  if(esperaBomba && now - timerBomba >= 500){  
    digitalWrite(relePin[7],LOW);
    reles[7].encendido=true;
    esperaBomba = false;
  }

  

  if(esperandoApagadoRele && now - timerApagadoRele >= 1000){
    digitalWrite(relePin[relePendienteApagado],HIGH);
    reles[relePendienteApagado].encendido=false;
    esperandoApagadoRele = false;
  }

  // reporte conexion ok

  if(now - timerReporte > 7200000){
    enviarATodos("📡 Sistema OK\n");
    timerReporte = now;
  }

  // boton todos off

  if(digitalRead(BottOFF) == LOW && now - debounceOFF > 300){

    debounceOFF = now;

    for(int r=0;r<8;r++){
      digitalWrite(relePin[r],HIGH);
      reles[r].encendido=false;
    }
    guardarEvento("⛔ Todos los Rele ACTIVOS apagados desde boton físico (LOCAL)");
    bot->sendMessage(CHAT_ID,"⛔ Todos los Rele ACTIVOS apagados desde boton físico (LOCAL)","");
  }

  // reinicio

  if(reinicioPendiente && now - timerReinicio >= 8000){
    ESP.restart();
  }

  // reset wifi

  static unsigned long inicioReset = 0;

  if(digitalRead(Bottreset) == LOW){

    if(inicioReset == 0){
      inicioReset = now;
    }

    if(now - inicioReset > 4000){
      WiFiManager wm;
      guardarEvento("Reseteo de REDWIFI");
      wm.resetSettings();
      ESP.restart();
    }

  } else {
    inicioReset = 0;
  }
}