#include "telegram_menu.h"
#include "variables.h"
#include "telegram.h"


void enviarMenuTelegram(String chat_id){

String menu;

menu += "     🌱 RIEGO AUTOMÁTICO INTELIGENTE\n";
menu += "                  🌐SolucionesIOT\n";
menu += "       ━━━━━━━━━━━━━━━━━━\n\n";

menu += "      SISTEMA\n";
menu += "📋  /menu\n";
menu += "⚡  /menurapido\n";
menu += "🔍  /historial\n";
menu += "❌ /borrarhistorial\n";
menu += "🔄  /reiniciar\n\n";

menu += "      ESTADO Y DATOS\n";
menu += "📊  /estado - Estado general\n";
menu += "📡  /sensores - Temp / Humedad / WiFi\n";
menu += "🌧  /clima - Probabilidad de lluvia\n";
menu += "📅  /horarios - Ver riegos programados\n\n";

menu += "      UBICACIÓN\n";
menu += "🌍  /ciudad Nombre - Buscar ciudad\n";
menu += "📍  /ubicacion latit-long - Manual\n\n";

menu += "      CONTROL MANUAL\n";
menu += "🎛  /panel - Panel con botones\n";
menu += "⚡  /rele1 a /rele7 - Encender/Apagar\n";
menu += "⛔  /todooff - Apagar todo\n\n";

menu += "      PROGRAMACIÓN\n";
menu += "⏰  /programar N HH:MM SEG\n";
menu += "🗑  /borrar N I - Borrar horario\n";
menu += "🧹  /borrarhorarios - Borrar todo\n\n";

menu += "      HUMEDAD\n";
menu += "💧  /humedad N - Límite de humedad\n\n";

menu += "      TANQUE\n";
menu += "🚰  /tanquesi - Automático\n";
menu += "🚫  /tanqueno - Manual\n\n";

menu += "      SENSOR TIERRA\n";
menu += "✔️  /sensorsi - Automático\n";
menu += "✖️  /sensorno - Manual\n\n";

menu += "      RELES\n";
menu += "✏️  /nombrerele N Nombre\n";
menu += "✅  /habilitar N\n";
menu += "⛔  /deshabilitar N\n";
menu += "🟢  /habilitartodos\n";
menu += "🔴  /deshabilitartodos\n";
menu += "🧹  /borrarnombres\n\n";

menu += "      USUARIOS\n";
menu += "👥  /usuarios\n";
menu += "➕  /autorizar ID Nombre\n";
menu += "➕  /nombreusuario ID Nombre\n";
menu += "➖  /eliminar ID\n";
menu += "➖  /salir\n\n";

menu += "✔️ Sistema inteligente activo";

enviarTelegram(chat_id, menu);

}

void enviarPanelReles(String chat_id){

ReplyKeyboard teclado;

teclado.addRow();
teclado.addButton(reles[0].nombre.c_str());
teclado.addButton(reles[1].nombre.c_str());

teclado.addRow();
teclado.addButton(reles[2].nombre.c_str());
teclado.addButton(reles[3].nombre.c_str());

teclado.addRow();
teclado.addButton(reles[4].nombre.c_str());
teclado.addButton(reles[5].nombre.c_str());

teclado.addRow();
teclado.addButton("⚡ Menurapido");

if(!modoTanqueAutomatico){
    
    teclado.addButton(reles[6].nombre.c_str());

}

teclado.enableResize();

TBMessage msg;


msg.chatId = atoll(chat_id.c_str());

myBot.sendMessage(
    msg,
    "🎛 PANEL DE CONTROL",
    teclado
);
}

void enviarMenuRapido(String chat_id){

ReplyKeyboard teclado;

teclado.addRow();

teclado.addButton("📋 Menu");
teclado.addButton("🎛 Panel");


teclado.addRow();

teclado.addButton("📊 Estado");
teclado.addButton("🌡 Sensores");

teclado.addRow();

teclado.addButton("⛔ Todo OFF");
teclado.addButton("📅 Horarios");

teclado.addRow();

teclado.addButton("🚰 Tanque SI");
teclado.addButton("🚫 Tanque NO");

teclado.enableResize();

TBMessage msg;

msg.chatId = atoll(chat_id.c_str());

myBot.sendMessage(
    msg,
    "📲 ACCESOS RÁPIDOS",
    teclado
);
}
