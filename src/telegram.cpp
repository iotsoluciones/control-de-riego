#include <WiFi.h>
#include "telegram.h"
#include "reles.h"
#include "variables.h"
#include "historial.h"
#include "usuarios.h"
#include "horarios.h"
#include "seguridad.h"
#include "clima.h"


#include "telegram_menu.h"
#include "helpers.h"

TBMessage msg;
bool hayComandoTelegram = false;
String text = "";
String chat_id = "";
String nombreUsuario = "";
String ultimoMensajeID = "";
String idSalirPendiente = "";

bool enviarTelegram(String chatid, String texto){

    if(WiFi.status()!=WL_CONNECTED){
        Serial.println("❌ Sin WiFi");
        return false;
    }

    Serial.print("Telegram -> ");
    Serial.println(chatid);

    TBMessage msg;

    int64_t idTelegram = atoll(chatid.c_str());

    msg.chatId = idTelegram;

    myBot.sendMessage(msg, texto);

    Serial.println("✅ Enviado");

    return true;
}

void manejarTelegram(){

    if(hayComandoTelegram){
        return;
    }

    if(myBot.getNewMessage(msg)){

       String idActual =
       String(msg.chatId) + "_" + String(msg.messageID);

         if(idActual == ultimoMensajeID){
            return;
            }

           ultimoMensajeID = idActual;

        comandoPendiente = msg.text;
        chatPendiente = String(msg.chatId);

        hayComandoTelegram = true;
    }
}

void procesarTelegram(){

    if(!hayComandoTelegram){
        return;
    }

    text = comandoPendiente;
    chat_id = chatPendiente;

    Serial.println("CHAT RECIBIDO:");
    Serial.println(chat_id);

    Serial.println("CHAT ADMIN:");
    Serial.println(CHAT_ID);

    bool comandoValido = false;

  // ==========================
       // INGRESAMOS A COMANDOS 


static unsigned long ultimoAviso = 0;

if(bloqueoFisicoActivo){

  if(millis() - ultimoAviso > 5000){
    
    enviarTelegram(chat_id,
    "⛔ Sistema bloqueado por seguridad física");
    ultimoAviso = millis();
  }
  
  return;
}

if(millis() - bloqueoArranque < 8000){
  hayComandoTelegram=false;
return;
}
if(!usuarioAutorizado(chat_id)){

  enviarTelegram(chat_id,
  "⛔ Usuario no autorizado\n\n"
  "🆔 ID: " + chat_id + "\n"
  "👉 Solicita acceso al administrador");

  enviarTelegram(CHAT_ID,
  "🚫 Intento de acceso\nID: " + chat_id);
  hayComandoTelegram = false;
  return;
}

// convertir nombre de rele en comando 
for(int r=0;r<7;r++){
  if(text == reles[r].nombre){
    text="/rele"+String(r+1);
    comandoValido = true;
  }
}

////   salir de sistema autoeliminacio 

if(text == "/salir"){

    esperandoConfirmacionSalir = true;
    idSalirPendiente = chat_id;

    enviarTelegram(
        chat_id,
        "⚠️ Vas a eliminar tu acceso del sistema.\n\n"
        "Acciona para confirmar:\n"
        "/confirmarsalir"
    );

    comandoValido = true;
}


if(text == "/confirmarsalir"){

    if(esperandoConfirmacionSalir &&
       idSalirPendiente == chat_id){

         if(chat_id == CHAT_ID){       /////////// EDMIN NO SE PUEDE AUTOELIMINAR 

    enviarTelegram(
        chat_id,
        "⛔ El administrador principal no puede eliminarse."
    );

    return;
     } 

        for(int i=0;i<cantidadUsuarios;i++){

            if(String(usuariosID[i]) == chat_id){

                for(int j=i;j<cantidadUsuarios-1;j++){
                    usuariosID[j] = usuariosID[j+1];
                }

                cantidadUsuarios--;

                guardarUsuarios();

                enviarTelegram(
                    chat_id,
                    "❌ Tu ID fue eliminado del sistema.\n"
                    "Ya no tenés acceso."
                );

                enviarTelegram(
                    CHAT_ID,
                    "⚠️ Un usuario se eliminó del sistema:\n" +
                    chat_id
                );

                break;
            }
        }

        esperandoConfirmacionSalir = false;
        idSalirPendiente = "";

        comandoValido = true;
    }
}

// ACTIVAR SENSOR
if(text == "/sensorsi"){
  comandoValido = true;

  sensorSueloActivo = true;

  prefs.begin("config", false);
  prefs.putBool("sensorSuelo", true);
  prefs.end();

  String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " activo sensor de suelo");

  enviarTelegram(chat_id,
  "🌱 Sensor de suelo ACTIVADO\nEl riego usará la humedad del suelo");
}

// DESACTIVAR SENSOR
if(text == "/sensorno"){
  comandoValido = true;

  sensorSueloActivo = false;

  prefs.begin("config", false);
  prefs.putBool("sensorSuelo", false);
  prefs.end();

  String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " desactivo sensor de suelo");

  enviarTelegram(chat_id,
  "🚫 Sensor de suelo DESACTIVADO\nEl riego no tendrá en cuenta el suelo");
}

if(text.startsWith("/ciudad")){

  comandoValido = true;

  char nueva[60];

  int resultado = sscanf(text.c_str(), "/ciudad %[^\n]", nueva);

  if(resultado == 1){

    String ciudadIngresada = String(nueva);

    enviarTelegram(chat_id,
    "🌍 Buscando ciudad...\n" + ciudadIngresada);

    bool ok = buscarCoordenadas(ciudadIngresada);

    if(ok){
       String nombre = obtenerNombreUsuario(chat_id);
       guardarEvento(nombre + "actualizo UBICACION. ");  // almaceno accion y nombre en histrial 
      enviarTelegram(chat_id,
      "✅ Ciudad encontrada:\n📍 " + ciudad);

    }else{

      enviarTelegram(chat_id,
      "❌ No se encontró la ciudad:\n" + ciudadIngresada);

    }

  }else{

    enviarTelegram(chat_id,
    "⚠️ Uso: /ciudad (ciudad ubicacion)");

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

enviarTelegram(chat_id, msg);
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
enviarTelegram(chat_id,
"📍 Ubicación guardada\nLat: " + String(lat,6) +
"\nLon: " + String(lon,6));

}else{

enviarTelegram(chat_id,
"⚠️ Uso: /ubicacion -34.60 -58.38");

}
}
// ===== BORRAR HISTORIAL =====

if (text == "/borrarhistorial") {

  comandoValido = true;

  if (!esAdmin(chat_id)) {
    enviarTelegram(chat_id, "⛔ Solo el admin puede borrar el historial");
    hayComandoTelegram=false;
    return;
  }

  enviarTelegram(chat_id,
  "⚠️ Confirmar borrado\nEscribí: /confirmarborrado");
     hayComandoTelegram=false;
  return;
}

if (text == "/confirmarborrado") {

  comandoValido = true;

  if (!esAdmin(chat_id)) {
    enviarTelegram(chat_id, "⛔ No autorizado");
    hayComandoTelegram=false;
    return;
  }

  borrarHistorial();

  enviarTelegram(chat_id,
  "🗑️ Historial borrado correctamente");
   hayComandoTelegram=false;
  return;
}

if(text=="/menu" || text=="📋 Menu"){
    enviarMenuTelegram(chat_id);
    comandoValido = true;
}

if(text=="/menurapido" || text=="⚡ Menurapido"){
enviarMenuRapido(chat_id);
comandoValido = true;
}

///// panel -///////
if(text=="/panel" || text=="🎛 Panel"){
    enviarPanelReles(chat_id);
    comandoValido = true;
}

// ---- MODO TANQUE ----

if(text=="/tanquesi" || text=="🚰 Tanque SI"){
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


  //  pequeño bloqueo para evitar rebotes automáticos
  bloqueoBombaManual = millis();
   String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " habilita Modo tanque AUTOMATICO.");  // almaceno accion y nombre en histrial 
  enviarTelegram(chat_id,
  "⚠️ Tanque AUTOMATICO ACTIVADO\nControl por flotante habilitado");
}


if(text=="/tanqueno" || text=="🚫 Tanque NO"){
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
  
  enviarTelegram(chat_id,
  "⚠️ Tanque AUTOMATICO DESACTIVADO\n");
}

if(text=="/borrarnombres"){
comandoValido = true;
prefs.begin("reles", false);

for(int i=0;i<8;i++){

// SI ES RELE 7 Y ESTA EN MODO TANQUE → NO TOCAR 
if(i==6 && modoTanqueAutomatico){
  continue;
}

String nombreDefault = "Rele " + String(i+1);

// actualizar en memoria 
reles[i].nombre = nombreDefault;

// guardar en flash 
prefs.putString(("nombre"+String(i)).c_str(), nombreDefault);

}

prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);
guardarEvento(nombre + " realizo borrado de nombres almacenados.");  // almaceno accion y nombre en histrial 
enviarTelegram(chat_id,"🗑 Nombres de todos los Rele reiniciados correctamente");

} 

if(text=="/habilitartodos"){
comandoValido = true;
prefs.begin("reles",false);

for(int i=0;i<8;i++){
  reles[i].habilitado=true;
  prefs.putBool(("hab"+String(i)).c_str(),true);
}

prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);
 guardarEvento(nombre + " habilito todos los Rele ");  // almaceno accion y nombre en histrial 
enviarTelegram(chat_id,"✅ Todos los Rele habilitados");
}

if(text=="/deshabilitartodos"){
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
enviarTelegram(chat_id,"⛔ Todos los Rele deshabilitados");
}

/////// reiniciar sistema -///////
if(text=="/reiniciar"){
comandoValido = true;
enviarTelegram(chat_id,"🔄 Reiniciando sistema...");
 String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento(nombre + " reinicia el sistema. ");  // almaceno accion y nombre en histrial 
   display.clearDisplay();
   display.setTextSize(1);
  display.setCursor(18,8);
  display.println("Reiniciando...");
  display.setCursor(10,22);
  display.println("Por favor espere!");
  display.display();
  yield(); // para actualizar display antes de reiniciar
  delay(2000); // pequeño delay para que el mensaje se alcance a mostrar
reinicioPendiente = true;
hayComandoTelegram = false;
timerReinicio = millis();

}

// ===== BORRAR TODOS LOS HORARIOS ===== 

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

// guardar cambios en memoria 
guardarHorarios();
 String nombre = obtenerNombreUsuario(chat_id);
 guardarEvento(nombre + " borra todos los horarios almacenados. ");  // almaceno accion y nombre en histrial 
enviarTelegram(chat_id,"🗑  Todos los horarios fueron borrados. Sistema reiniciado de programacion.");

}
// -------- ESTADO --------

if(text=="/estado" || text=="📊 Estado"){
comandoValido = true;
String mensaje;

mensaje+="📊 *ESTADO DEL SISTEMA*\n";
mensaje+="━━━━━━━━━━━━━━━━━━\n\n";

mensaje+="🌡 Temperatura: "+String(temperatura,1)+" °C\n\n";
mensaje+="💧 Humedad: "+String(humedad,1)+" %\n\n";
mensaje+="📶 WiFi: "+textoWiFiEstado()+"\n\n";
mensaje+= "🌐 IP: ";
mensaje+= WiFi.localIP().toString();
mensaje+= "\n\n";
mensaje+="🚫 Limite humedad: "+String(humedadLimite)+" %\n\n";
mensaje+="🌧 *CLIMA* - ";
mensaje+="💧 Probabilidad: " + String(probabilidadLluvia,1) + " %\n";

// usar MISMA lógica que el sistema
bool lluviaActual = probabilidadLluvia > 60;

if(lluviaBloqueada){
  if(millis() - bloqueoLluviaTiempo < 10800000){
    lluviaActual = true;
  }
}

// estado final
if(lluviaActual){
  mensaje+="⛔ Riego BLOQUEADO por lluvia\n\n";
}else{
  mensaje+="✅ Riego PERMITIDO por lluvia \n\n";
}

mensaje+="🚰 Modo tanque: ";

if(modoTanqueAutomatico){
mensaje+="🟢 AUTOMATICO \n";
mensaje+="(control por flotante)\n";
}
else{
mensaje+="⛔ DESACTIVADO \n";
}

// 🌱 SENSOR DE SUELO

mensaje += "\n🌱 *SUELO*\n";

if(!sensorSueloActivo){

  mensaje += "Sensor: 🔴 DESACTIVADO\n\n";

}else{

  mensaje += "Sensor: 🟢 ACTIVO\n\n";
}
//////////////////////////////////

String motivo = obtenerMotivoBloqueo();

if(motivo != ""){
  mensaje += "🚫 BLOQUEO: " + motivo + "\n";
}else{
  mensaje += "✅ SIN BLOQUEOS\n";
}

mensaje+="\n━━━━━━━━━━━━━━━━━━\n\n";

//////////////////////////////////////////////////

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

enviarTelegram(chat_id,mensaje);

}

if(text.startsWith("/autorizar")){
comandoValido = true;
if(chat_id != CHAT_ID){
  enviarTelegram(chat_id,"⛔ Solo admin");
  hayComandoTelegram=false;
  return;
}

char id[20];
char nombre[30];

int ok = sscanf(text.c_str(), "/autorizar %s %[^\n]", id, nombre);

if(ok >= 2){

String nuevoID = String(id);
String nuevoNombre = String(nombre);

// evitar duplicados 
for(int i=0;i<cantidadUsuarios;i++){
  if(usuariosID[i] == nuevoID){
    enviarTelegram(chat_id,"⚠️ Usuario ya existe");
    hayComandoTelegram=false;
    return;
  }
}

if(cantidadUsuarios < MAX_USERS){

usuariosID[cantidadUsuarios] = nuevoID;
usuariosNombre[cantidadUsuarios] = nuevoNombre;

cantidadUsuarios++;
 enviarTelegram(
    String(nuevoID),
    "✅ Has sido autorizado en el sistema de RIEGO SolucionesIOT\n"
    "Ya podés utilizar los comandos habilitados!! Envia /menu para continuar... "
); 

guardarUsuarios();

 String nombre = obtenerNombreUsuario(chat_id);
  guardarEvento( nombre + " autorizo usuario " + nuevoNombre + " (" + nuevoID + ")");  // almaceno accion y nombre en histrial 

enviarTelegram(chat_id,
"✅ Usuario autorizado\n👤 "+nuevoNombre+"\n🆔 "+nuevoID);

}else{
enviarTelegram(chat_id,"⛔ Maximo usuarios alcanzado");
}

}else{
enviarTelegram(chat_id,"⚠️ Uso:\n/autorizar ID Nombre");
}

}

if(text=="/usuarios"){
comandoValido = true;

if(cantidadUsuarios == 0){

  enviarTelegram(chat_id,
  "⚠️ No hay usuarios cargados\n\n"
  "👉 El sistema aún no tiene usuarios autorizados\n"
  "👉 Usá: /autorizar ID Nombre");
  hayComandoTelegram=false;
  return;
}

String lista = "👥 Usuarios autorizados:\n\n";

for(int i=0;i<cantidadUsuarios;i++){
  lista += "👤 "+usuariosNombre[i]+" → "+usuariosID[i]+"\n";
}

enviarTelegram(chat_id, lista);
}

if(text.startsWith("/eliminar")){
comandoValido = true;

if(chat_id != CHAT_ID){
  enviarTelegram(chat_id,"⛔ Solo admin");
  hayComandoTelegram=false;
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

enviarTelegram(chat_id,"🗑 Usuario eliminado");

guardarEvento( nombreAdmin + " elimino a " + nombreEliminado);  // almaceno accion y nombre en histrial
hayComandoTelegram=false;
return;
}

}

enviarTelegram(chat_id,"⚠️ Usuario no encontrado");
}
}

// ===== NUEVO COMANDO /HORARIOS ===== 
if(text=="/horarios" || text=="📅 Horarios"){
comandoValido = true;
String mensaje;

mensaje+="🌱 *PROGRAMACION DE RIEGO*\n";
mensaje+="━━━━━━━━━━━━━━━━━━\n\n";

for(int r=0;r<8;r++){

// RELE 8 = BOMBA 
if(r==7){

mensaje+="🚰 *BOMBA*\n";
mensaje+="   ⚙️ Activacion automatica\n";
mensaje+="   Depende de valvulas activas.\n\n";

continue;
}

// RELE 7 = TANQUE 
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

// RELES 1–6 (TODO IGUAL QUE ANTES) 

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

enviarTelegram(chat_id,mensaje);

}

// ===== COMANDO HUMEDAD ===== 

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

enviarTelegram(chat_id,"⛔ Nuevo limite humedad: "+String(valor)+"%");

}else{

enviarTelegram(chat_id,"⚠️ Uso correcto: /humedad 1 a 100");

}

}

if(text=="/historial"){
  comandoValido = true;

  String msg = "📜 *HISTORIAL DE EVENTOS*\n";
  msg += "━━━━━━━━━━━━━━━━━━\n\n";

  int contador = 0;

  for(int i=0;i<MAX_EVENTOS;i++){

    int idx = (indiceEvento - 1 - i + MAX_EVENTOS) % MAX_EVENTOS;

    if(eventos[idx] != ""){

      String linea = eventos[idx] + "\n";

      // 🚨 evita superar límite de Telegram (~4096)
      if(msg.length() + linea.length() > 3500){
        enviarTelegram(chat_id, msg);
        msg = "";
      }

      msg += linea;
      contador++;
    }
  }

  if(contador == 0){
    msg += "⚠️ Sin eventos registrados";
  }

  // enviar lo que quedó
  if(msg.length() > 0){
    enviarTelegram(chat_id, msg);
  }
}

//   ===== CAMBIAR NOMBRE RELE ===== 

if(text.startsWith("/nombrerele")){
comandoValido = true;
int numero;
char nombre[30];

sscanf(text.c_str(), "/nombrerele %d %[^\n]", &numero, nombre);

numero--;

// 🚫 bloquear rele 7 en modo tanque 
if(numero==6 && modoTanqueAutomatico){

enviarTelegram(chat_id,
"⛔ No se puede cambiar nombre del rele 7 en modo tanque automatico");
hayComandoTelegram=false;
return;
}

if(numero>=0 && numero<7){

reles[numero].nombre = String(nombre);

prefs.begin("reles",false);
prefs.putString(("nombre"+String(numero)).c_str(),reles[numero].nombre);
prefs.end();
 String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
 guardarEvento(nombre + " cambio nombre de Rele " + String(numero+1) + " a " + reles[numero].nombre);  // almaceno accion y nombre en histrial 
enviarTelegram(
chat_id,
"✅ Nombre actualizado: Rele "+String(numero+1)+" → "+reles[numero].nombre
);

}

}

//  ===== COMANDO SENSORES ===== 

if(text=="/sensores" || text=="🌡 Sensores"){

comandoValido = true;
String msg;

msg+="📡 *SENSORES*\n";
msg+="━━━━━━━━━━━━━━━━━━\n\n";

msg+="🌡 Temperatura\n";
msg+="→ "+String(temperatura,1)+" °C\n\n";

msg+="💧 Humedad\n";
msg+="→ "+String(humedad,1)+" %\n\n";

msg+="🌱 Sensor Suelo\n";

if(sensorSueloActivo){

  msg+="→ 🟢 ACTIVO\n";

  msg+="→ Humedad: ";
  msg+=String(humedadSuelo);
  msg+=" %\n\n";

}else{

  msg+="→ 🔴 DESACTIVADO\n\n";
}


msg+="📶 WiFi\n";
msg+="→ "+textoWiFiEstado()+"\n\n";

msg+="🚰 Tanque Automatico\n";

if(modoTanqueAutomatico){

  msg+="→ 🟢 ACTIVADO\n";

  bool pedidoAgua = digitalRead(SENSOR_TANQUE) == LOW;

  if(pedidoAgua){
    msg+="→ Flotante pidiendo agua\n\n";
  }else{
    msg+="→ Tanque completo\n\n";
  }

}else{

  msg+="→ 🔴 DESACTIVADO\n\n";
}

msg+="━━━━━━━━━━━━━━━━━━";

enviarTelegram(chat_id, msg);

}
// -------- CONTROL MANUAL TOGGLE --------

if(bloqueoFisicoActivo){

    enviarTelegram(
        chat_id,
        "⛔ Sistema bloqueado físicamente\n"
        "No se puede ejecutar la acción solicitada"
    );

    comandoPendiente = "";
    chatPendiente = "";

    hayComandoTelegram = false;

    return;
}

for(int r=0;r<7;r++){

  String cmd="/rele"+String(r+1);

  if(text==cmd){

    // 🔒 BLOQUEO RELÉ 7 EN AUTOMÁTICO
    if(r == 6 && modoTanqueAutomatico){
      enviarTelegram(chat_id,
      "⛔ Relé 7 está en modo automático (tanque)\nNo se puede controlar manualmente");
      comandoValido = true;
      continue;
    }

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

enviarTelegram(chat_id,"✅ Rele "+String(numero+1)+" habilitado");

}else{

enviarTelegram(chat_id,"⚠️ Uso correcto: /habilitar 1 a 8");

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
enviarTelegram(chat_id,"⛔ Rele 8 protegido");
hayComandoTelegram=false;
return;
}

reles[numero].habilitado=false;

prefs.begin("reles",false);
prefs.putBool(("hab"+String(numero)).c_str(),false);
prefs.end();

enviarTelegram(chat_id,"⛔ Rele "+String(numero+1)+" deshabilitado");
String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(nombre + " deshabilita Rele" + String(numero+1));   /// almacenamos la info en el historial

}else{

enviarTelegram(chat_id,"⚠️ Uso correcto: /deshabilitar 1 a 8");

}

}

//-------------APAGAR TODOS LOS RELES ---------
if(text=="/todooff" || text=="⛔ Todo OFF"){

comandoValido = true;
String msg;

for(int r=0;r<8;r++){

digitalWrite(relePin[r],HIGH);
reles[r].encendido=false;}
delay((50));
String nombre = obtenerNombreUsuario(chat_id);   /// creamos variable, almacenamos con el nombre obtenernombreusuario lo que llega por chatid
guardarEvento(nombre + " apago todos los Rele ");   /// almacenamos la info en el historial
enviarTelegram(chat_id,"⛔ Todos los reles apagados manualmente");

}

// -------- PROGRAMAR --------

if(text.startsWith("/programar")){
comandoValido = true;
int numero,hora,minuto,duracion;

sscanf(text.c_str(), "/programar %d %d:%d %d",&numero,&hora,&minuto,&duracion);

numero--;

// 🚫 bloquear rele 7 en modo tanque automatico
if(numero == 6 && modoTanqueAutomatico){

  enviarTelegram(chat_id,
  "⛔ No se puede programar Rele 7 en modo tanque automatico");
   hayComandoTelegram=false;
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
enviarTelegram(chat_id,"✅ Horario agregado correctamente");

}else{

enviarTelegram(chat_id,"⛔ Maximo 5 horarios correctamente");

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

enviarTelegram(chat_id, msg);

}

}

if(!comandoValido){

String sugerencia = sugerirComando(text);

if(sugerencia != ""){
enviarTelegram(chat_id,
"❓ Comando no reconocido\n👉 Quizás quisiste decir: "+sugerencia);
}
else{
enviarTelegram(chat_id,
"❓ Comando no reconocido\n👉 Escribí /menu");
}

}



  // ==========================
 
  hayComandoTelegram = false;

  comandoPendiente = "";
  chatPendiente = "";
  

  clientTelegram.stop();
}

void enviarATodos(const String mensaje){

    Serial.println("\n===== ENVIAR A TODOS =====");

    Serial.print("Mensaje: ");
    Serial.println(mensaje);

    Serial.print("WiFi: ");
    Serial.println(WiFi.status());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());

    Serial.print("Usuarios: ");
    Serial.println(cantidadUsuarios);

    Serial.print("Tiempo: ");
    Serial.println(millis());

    if(WiFi.status()!=WL_CONNECTED){

        Serial.println("❌ WIFI DESCONECTADO");
        return;
    }

    Serial.println("ADMIN...");

    bool okAdmin = enviarTelegram(CHAT_ID,mensaje);

    if(okAdmin){
        Serial.println("✅ ADMIN OK");
    }else{
        Serial.println("❌ ADMIN ERROR");
    }

    delay(1200);
    yield();

    for(int i=0;i<cantidadUsuarios;i++){

        Serial.print("ID: ");
        Serial.println(usuariosID[i]);

        if(usuariosID[i]==""){
            Serial.println("ID VACIO");
            continue;
        }

        if(usuariosID[i]==CHAT_ID){
            Serial.println("ADMIN DUPLICADO");
            continue;
        }

        Serial.println("ENVIANDO...");

        bool ok = enviarTelegram(
            usuariosID[i],
            mensaje
        );

        if(ok){
            Serial.println("✅ OK");
        }else{
            Serial.println("❌ ERROR ENVIO");
        }

        delay(1200);
        yield();

        Serial.print("Heap actual: ");
        Serial.println(ESP.getFreeHeap());

        Serial.print("RSSI actual: ");
        Serial.println(WiFi.RSSI());

        Serial.println("----------------");
    }

    Serial.println("===== FIN =====\n");
}

void procesarMensajesPendientes(){

    static unsigned long ultimoEnvio = 0;

    if(millis() - ultimoEnvio < 1200){
        return;
    }

    if(totalMensajes <= 0){
        return;
    }

    Serial.println("===== MENSAJE =====");

    Serial.print("Cola: ");
    Serial.println(totalMensajes);

    Serial.print("Mensaje: ");
    Serial.println(colaMensajes[0]);

    Serial.print("WiFi: ");
    Serial.println(WiFi.status());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());

    ultimoEnvio = millis();

    enviarATodos(colaMensajes[0]);

    for(int i=0;i<totalMensajes-1;i++){

        colaMensajes[i]=colaMensajes[i+1];
    }

    totalMensajes--;

    Serial.println("===== FIN =====");
}