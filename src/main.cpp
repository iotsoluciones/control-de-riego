#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <DHT.h>
#define DHTPIN 15
#define DHTTYPE DHT22
#define MAX_HORARIOS 5
#define SENSOR_TANQUE 27
#define MAX_USERS 5
String ciudad = "Buenos Aires";
float probabilidadLluvia = 0;
unsigned long timerClima = 0;
#include <HTTPClient.h>
#include <ArduinoJson.h>

String usuarios[MAX_USERS];
int cantidadUsuarios = 0;
float lat = -34.60;
float lon = -58.38;
String usuariosID[MAX_USERS];
String usuariosNombre[MAX_USERS];

DHT dht(DHTPIN, DHTTYPE);
unsigned long bloqueoArranque=0;  
WiFiManager wm;

WiFiManagerParameter param_token("token","Token Telegram","",60);
WiFiManagerParameter param_chatid("chatid","Chat ID","",20);

Preferences prefs;

WiFiClientSecure client;
String BOTtoken;
String CHAT_ID;
void pantallaInicio(String texto);
UniversalTelegramBot *bot;
int relePin[8]={13,23,14,22,26,21,33,32};
bool avisoHumedadEnviado=false;
int Bottreset=25;
int BottOFF=4;
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



void consultarClima(){

  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + ciudad + "&appid=TU_API_KEY&units=metric";

  http.begin(url);
  int httpCode = http.GET();

  if(httpCode == 200){

    String payload = http.getString();

    DynamicJsonDocument doc(4096);
    deserializeJson(doc, payload);
    
  if(doc["list"][0]["pop"].is<float>()){
  probabilidadLluvia = doc["list"][0]["pop"].as<float>() * 100.0;
}
  }

  http.end();
}
 void enviarATodos(String mensaje){

  // 👑 admin primero
  bot->sendMessage(CHAT_ID, mensaje, "");

  for(int i=0;i<cantidadUsuarios;i++){

    bool repetido = false;

    // 🔍 verificar duplicados
    for(int j=0;j<i;j++){
      if(usuariosID[i] == usuariosID[j]){
        repetido = true;
        break;
      }
    }

    if(repetido) continue;

    // 🚫 evitar enviar al admin duplicado
    if(usuariosID[i] == CHAT_ID) continue;

    bot->sendMessage(usuariosID[i], mensaje, "");
  }
 }
 void controlMultiplesHorarios(int hora,int minuto){

  // 🔒 pequeño bloqueo después de manual
  if(millis() - bloqueoManualTiempo < 5000){
    return;
  }

  int limite = modoTanqueAutomatico ? 6 : 7;

  // 🔴 =========================
  // 🔴 CORTE EN TIEMPO REAL
  // 🔴 =========================
  for(int r=0;r<limite;r++){

    if(reles[r].encendido && reles[r].duracion > 0){

      if(millis() - reles[r].inicio >= reles[r].duracion*1000){

        digitalWrite(relePin[r],HIGH);
        reles[r].encendido=false;

        reles[r].ultimoMinuto = minuto;

        // 🔍 verificar si queda algún rele activo
        bool quedaActivo = false;

        for(int i=0;i<6;i++){
          if(reles[i].encendido){
            quedaActivo = true;
            break;
          }
        }

        // 🚰 apagar bomba si no queda ninguno
        if(!quedaActivo){
          digitalWrite(relePin[7],HIGH);
          reles[7].encendido=false;
        }

        enviarATodos("✅ "+reles[r].nombre+" finalizado");
      }
    }
  }

  static int ultimoMinutoGlobal = -1;

if(minuto != ultimoMinutoGlobal){

  ultimoMinutoGlobal = minuto;

  // 🟢 ACTIVACION AUTOMATICA SOLO UNA VEZ POR MINUTO
 for(int r=0;r<limite;r++){

  if(!reles[r].habilitado) continue;

  for(int h=0;h<cantidadHorarios[r];h++){

    if(!reles[r].encendido &&
       hora == horarios[r][h].hora &&
       minuto == horarios[r][h].minuto &&
       reles[r].ultimoMinuto != minuto){

      if(bloqueoHumedad && humedad > humedadLimite){

        if(!avisoHumedadEnviado){
          enviarATodos("⚠️ Riego cancelado\nHumedad alta: "+String(humedad,1)+"%");
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
  String menu="🌱 SISTEMA DE RIEGO AUTOMATICO\n";
   menu+="SolucionesIOT 🌐";
  menu+="━━━━━━━━━━━━━━━━━━\n\n";

  menu+="📊 ESTADO \n";
  
  menu+="/menu      - Ver menu\n";
  menu+="/menu_rapido - Accesos rápidos\n";
  menu+="/estado    - Estado de sistema\n";
  menu+="/horarios  - Ver automatizacion\n";
  menu+="/sensores  - Temperatura-humedad-señal WiFi\n";
  menu+="/todo_off - (Apagar todos los rele activos)\n";
  menu+="/panel      - Ver panel de control reles\n"; 
  menu+="/reiniciar - ReiniciaR sistema\n"; 
  menu+="━━━━━━━━━━━━━━━━━━\n\n";

  menu+="🗑 ADMIN NOMBRE - HORARIOS\n";
  menu+="/borrar N I - (Borrar horario seleccionado)\n";
  menu+="/borrarhorarios - (Borrar todos los horarios guardados)\n";
  menu+="/borrarnombres- (Borrar todos los nombres de reles guardados)\n";
  
  menu+="━━━━━━━━━━━━━━━━━━\n\n";

  menu+="🎛 CONTROL MANUAL (TOGGLE)\n";
  menu+="Enviar el comando para cambiar estado\n\n";
  for(int r=0;r<7;r++){
  menu+="⚡ /rele"+String(r+1)+"- ";}
  menu+="━━━━━━━━━━━━━━━━━━\n\n";

  menu+="🔌 CONTROL GENERAL\n";
  menu+="/nombrerele N NOMBRE \n"; 
  menu+="(Cambiar nombre del rele)\n"; 
  menu+="/habilitar N - (Habilitar rele)\n";
  menu+="/deshabilitar N - (Deshabilitar rele)\n";
  menu+="/habilitar_todos - Habilitar todos los rele\n";
  menu+="/deshabilitar_todos - Deshabilitar todos los Rele\n";
  menu+="/programar 1 21:30 60 \n";
  menu+="Ej:(Programamos rele1, hora 21:30, 60 segundos corresponde a 1 minuto de trabajo)\n\n";
  menu+="/tanque_si  - Activar modo tanque automatico\n";
  menu+="/tanque_no  - Usar rele 7 como Normal\n";
  menu+="/humedad N - (Cambiar limite humedad)\n";
  menu+="⛔ Limite actual: ";
  menu+=String(humedadLimite);
  menu+=" %\n";
  menu+="\n━━━━━━━━━━━━━━━━━━\n\n";

  menu+="👥 ADMINISTRAR USUARIOS\n";
  menu+="/autorizar ID Nombre - Agregar usuario\n";
  menu+="/eliminar ID - Eliminar usuario\n";
  menu+="/usuarios - Ver usuarios autorizados\n\n";
  bot->sendMessage(chat_id,menu,"");

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
if(!modoTanqueAutomatico){
  return; // 🔴 NO hace nada si está en modo manual
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
delay(500); // espera 0.5 segundos para asegurar apertura de válvula
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
delay(500); // espera 0.5 segundos para asegurar cierre de válvula
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
static unsigned long bloqueoBomba = 0;

if(millis()- bloqueoBomba<2000){
  return;
}

bool algunReleActivo=false;

/* 👉 si modo tanque manual → incluir rele 7 */
int limite = modoTanqueAutomatico ? 6 : 7;

for(int i=0;i<limite;i++){
  if(reles[i].encendido){
    algunReleActivo=true;
    break;
  }
}

/* ENCENDER BOMBA */
if(algunReleActivo && !reles[7].encendido && !esperandoApagadoRele){

digitalWrite(relePin[7],LOW);
reles[7].encendido=true;

}

/* APAGAR BOMBA */
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

/* 🔐 CONTROL DE ACCESO */

if(millis() - bloqueoArranque < 8000){
continue;
}
if(!usuarioAutorizado(chat_id)){

  // 👉 si NO hay usuarios cargados
  if(cantidadUsuarios == 0){

    bot->sendMessage(chat_id,
    "⚠️ Sistema sin usuarios registrados\n\n"
    "👉 Solo el ADMIN puede usarlo\n"
    "👉 Para agregar usuarios:\n"
    "/autorizar ID Nombre\n\n"
    "📌 Ejemplo:\n"
    "/autorizar 123456789 Juan",
    "");

  }else{

    bot->sendMessage(chat_id,
    "⛔ Usuario no autorizado!!\n\n"
    "👉 Pedir acceso al administrador!!",
    "");
    bot->sendMessage(CHAT_ID,
    "🚫 Intento de acceso\nID: " + chat_id,
    "");
  }

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

char nueva[40];

int ok = sscanf(text.c_str(), "/ciudad %[^\n]", nueva);

if(ok == 1){

ciudad = String(nueva);

prefs.begin("config", false);
prefs.putString("ciudad", ciudad);
prefs.end();

bot->sendMessage(chat_id,
"📍 Ubicación actualizada:\n" + ciudad,
"");

}else{

bot->sendMessage(chat_id,
"⚠️ Uso: /ciudad Buenos Aires",
"");

}
}

if(text == "/lluvia"){

comandoValido = true;

String msg;

msg += "🌧️ Pronóstico de lluvia\n";
msg += "📍 " + ciudad + "\n\n";
msg += "Probabilidad: " + String(probabilidadLluvia,1) + " %";

bot->sendMessage(chat_id, msg, "");
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

/* nombre automatico */
reles[6].nombre = "TANQUE";

/* guardar nombre */
prefs.begin("reles", false);
prefs.putString("nombre6", "TANQUE");
prefs.end();

/* guardar modo */
prefs.begin("config", false);
prefs.putBool("modoTanque", modoTanqueAutomatico);
prefs.end();

bot->sendMessage(chat_id,"⚠️ ATENCION! Tanque AUTOMATICO activado","");

}

if(text == "/tanque_no"){
comandoValido = true;
modoTanqueAutomatico = false;

/* volver a nombre normal */
String nombreDefault = "Rele 7";
reles[6].nombre = nombreDefault;

/* guardar nombre */
prefs.begin("reles", false);
prefs.putString("nombre6", nombreDefault);
prefs.end();

/* guardar modo */
prefs.begin("config", false);
prefs.putBool("modoTanque", modoTanqueAutomatico);
prefs.end();

bot->sendMessage(chat_id,"⚠️ ATENCION! Tanque AUTOMATICO DESHABILITADO, Rele 7 habilitado NORMAL","");

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

bot->sendMessage(chat_id,"✅ Todos los Rele habilitados","");
}





if(text=="/deshabilitar_todos"){
comandoValido = true;
prefs.begin("reles",false);

for(int i=0;i<8;i++){

  if(i==7) continue;

  reles[i].habilitado=false;
  prefs.putBool(("hab"+String(i)).c_str(),false);
}

prefs.end();

bot->sendMessage(chat_id,"⛔ Todos los Rele deshabilitados","");
}




/////// reiniciar sistema -///////
if(text=="/reiniciar"){
comandoValido = true;
bot->sendMessage(chat_id,"🔄 Reiniciando sistema...","");

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

/* 🚰 RELE 8 = BOMBA */
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

/* resto de reles normales */
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

for(int i=0;i<cantidadUsuarios;i++){

if(usuariosID[i] == eliminarID){

for(int j=i;j<cantidadUsuarios-1;j++){
usuariosID[j] = usuariosID[j+1];
usuariosNombre[j] = usuariosNombre[j+1];
}

cantidadUsuarios--;

guardarUsuarios();

bot->sendMessage(chat_id,"🗑 Usuario eliminado","");
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

/* 👉 RELE 8 = BOMBA */
if(r==7){

mensaje+="🚰 *BOMBA*\n";
mensaje+="   ⚙️ Activacion automatica\n";
mensaje+="   Depende de valvulas activas.\n\n";

continue;
}

/* 👉 RELE 7 = TANQUE */
if(r==6){

mensaje+="💧 *"+reles[r].nombre+"*\n";
if(modoTanqueAutomatico){

mensaje+="   ⚙️ Flotante automatico activado\n\n";

}else{

if(cantidadHorarios[r]==0){

mensaje+="   🚫 Sin horarios\n\n";


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
mensaje+="-------------------------------\n";
}

}

continue;
}

/* 👉 RELES 1–6 (TODO IGUAL QUE ANTES) */
mensaje+="💧 *"+reles[r].nombre+"*\n";

if(cantidadHorarios[r]==0){

mensaje+="   🚫 Sin horarios\n\n";
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
mensaje+="-------------------------------\n";

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

humedadLimite = valor;

prefs.begin("config",false);
prefs.putFloat("humedadLimite",humedadLimite);
prefs.end();

bot->sendMessage(chat_id,"⛔ Nuevo limite humedad: "+String(valor)+"%","");

}else{

bot->sendMessage(chat_id,"⚠️ Uso correcto: /humedad 1 a 100","");

}

}

/* ===== COMANDO HUMEDAD ===== */

if(text.startsWith("/humedad")){

int valor;
int ok = sscanf(text.c_str(), "/humedad %d",&valor);

if(ok == 1 && valor>0 && valor<=100){

comandoValido = true;

humedadLimite = valor;

prefs.begin("config",false);
prefs.putFloat("humedadLimite",humedadLimite);
prefs.end();

bot->sendMessage(chat_id,"⛔ Nuevo limite humedad: "+String(valor)+"%","");

}else{

bot->sendMessage(chat_id,"⚠️ Uso correcto: /humedad 1 a 100","");

}

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
bloqueoManualTiempo = millis();

static unsigned long bloqueoBomba = 0;
bloqueoBomba = millis();

if(reles[r].encendido){

  // 🔴 APAGAR INMEDIATO
  digitalWrite(relePin[r], HIGH);
  reles[r].encendido = false;

  // 🔍 verificar si hay otro rele activo
  bool hayOtroActivo = false;

  int limite = modoTanqueAutomatico ? 6 : 7;

  for(int i=0;i<limite;i++){
    if(i != r && reles[i].encendido){
      hayOtroActivo = true;
      break;
    }
  }

  // 👉 apagar bomba solo si no queda ninguno
  if(!hayOtroActivo){
    digitalWrite(relePin[7],HIGH);
    reles[7].encendido=false;
  }

  String nombre = obtenerNombreUsuario(chat_id);
  enviarATodos("✅ "+reles[r].nombre+" - Apagado manual por: "+nombre);
}
else{

digitalWrite(relePin[r],LOW);
reles[r].encendido=true;

// ⏱ esperar antes de prender bomba
esperaBomba = true;
timerBomba = millis();

/* modo manual permanente */
reles[r].duracion=0;
reles[r].inicio=millis();

String nombre = obtenerNombreUsuario(chat_id);
enviarATodos("⚡ "+reles[r].nombre+"- Encendido manual por: "+nombre);

}

}

}

// -------- HABILITAR --------

if(text.startsWith("/habilitar ")){

int numero;
int ok = sscanf(text.c_str(), "/habilitar %d",&numero);

if(ok == 1 && numero>=1 && numero<=8){

comandoValido = true;   // 👈 ACA (NO antes)

numero--;

reles[numero].habilitado=true;

prefs.begin("reles",false);
prefs.putBool(("hab"+String(numero)).c_str(),true);
prefs.end();

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

comandoValido = true;   // 👈 ACA (NO antes)


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
  "⛔ No se puede programar el rele 7 en modo tanque automatico",
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

for(int i=indice;i<cantidadHorarios[rele]-1;i++){

horarios[rele][i]=horarios[rele][i+1];

}

cantidadHorarios[rele]--;

guardarHorarios();

bot->sendMessage(chat_id,"🗑  Horario borrado","");

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
Serial.begin(115200);

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


pinMode(Bottreset,INPUT_PULLUP);
pinMode(BottOFF,INPUT_PULLUP);
pinMode(SENSOR_TANQUE,INPUT_PULLUP);

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

bloqueoArranque = millis();

enviarATodos("✅ Sistema iniciado correctamente");

}
void loop()
{

unsigned long now = millis();

unsigned long timerReporte = 0;

if(millis() - timerReporte > 8000000){ // 
  enviarATodos("📡 Sistema OK\nTemp: "+String(temperatura));
  timerReporte = millis();
}

if(esperaBomba && millis() - timerBomba >= 100){ 

  digitalWrite(relePin[7],LOW);
  reles[7].encendido=true;

  esperaBomba = false;
}

if(millis() - timerClima > 600000){ // cada 10 min
  timerClima = millis();
  consultarClima();
}

if(esperandoApagadoRele && millis() - timerApagadoRele >= 2000){

  digitalWrite(relePin[relePendienteApagado],HIGH);
  reles[relePendienteApagado].encendido=false;

  esperandoApagadoRele = false;
}

controlarWiFi();

/* actualizar reloj cada segundo */
if(now - timerClock > 100){
  timerClock = now;
  actualizarHora();
}

/* leer sensor cada 3 segundos */
if(now - timerSensor > 500){
  timerSensor = now;
  leerSensor();
}

/* revisar telegram */
if(now - timerTelegram > 1000){
  timerTelegram = now;
  manejarTelegram();
}

/* ejecutar horarios de riego */
struct tm timeinfo;
if(getLocalTime(&timeinfo)){
  controlMultiplesHorarios(timeinfo.tm_hour, timeinfo.tm_min);
}
reconectarWiFi();
controlarTanque();


/* Apaga todos los RELE si se mantiene presionado */
if(digitalRead(BottOFF) == LOW && millis() - debounceOFF > 500){

  debounceOFF = millis();

  for(int r=0;r<8;r++){
    digitalWrite(relePin[r],HIGH);
    reles[r].encendido=false;
  }

  bot->sendMessage(CHAT_ID,"⛔ Todos apagados manualmente","");
}

///CONTEO PARA RESET DE LA PLACA//
if(reinicioPendiente && millis() - timerReinicio >= 8000){
  ESP.restart();
}


/* reset RED WiFi si se mantiene presionado */
if (digitalRead(Bottreset) == LOW) {
  delay(4000);
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

}

