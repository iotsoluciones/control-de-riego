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
menu += "💥  /menurapido\n";
menu += "🔍  /historial\n\n";

menu += "      ESTADO Y DATOS\n";
menu += "📊  /estado - Estado general\n";
menu += "📡  /sensores - Temp / Humedad / WiFi\n";
menu += "🌧   /clima - Probabilidad de lluvia\n";
menu += "📅  /horarios - Ver riegos programados\n\n";

menu += "      UBICACIÓN\n";
menu += "🌍  /ciudad Nombre - Buscar ciudad\n";
menu += "📍  /ubicacion latit-long - Manual\n\n";

menu += "      CONTROL MANUAL\n";
menu += " 🎛  /panel - Panel con botones\n";
menu += "⚡  /rele1 a /rele7 - Encender/Apagar\n";
menu += "⛔  /todooff - Apagar todo\n\n";

menu += "      PROGRAMACIÓN\n";
menu += "⏰  /programar N HH:MM SEG\n";
menu += "🗑   /borrar N I - Borrar horario\n";
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
menu += "🔴  /deshabilitartodos\n\n";

menu += "      USUARIOS\n";
menu += "👥  /usuarios\n";
menu += "➕  /autorizar ID Nombre\n";
menu += "➕  /nombreusuario ID Nombre\n";
menu += "🚷  /salir\n\n";

menu += "      CONFIGURACIÓN (⚠️Requiere clave)\n";
menu += "🔐  /cambiarclave - Clave acceso.\n";
menu += "🔄  /reiniciar - Reinicia el sistema.\n";
menu += "➖  /eliminarusuario ID - Elimina usuario.\n";
menu += "🧹  /borrarhistorial - Elimina historial.\n";
menu += "🧹  /borrarnombres - Borra todos los nombres.\n";
menu += "🧹  /borrarhorarios - Borrar todos los horarios.\n";
menu += "⛔  /factoryreset - ATENCION! borrar todos los datos almacenados en el sistema, vuevle reset de fabrica\n\n";


menu += "✔️ Sistema inteligente activo";

enviarTelegram(chat_id, menu);

}

void enviarPanelReles(String chat_id){

ReplyKeyboard teclado;


if(!modoTanqueAutomatico){
teclado.addRow();
teclado.addButton((String("⚡ ") + reles[0].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[1].nombre).c_str());

teclado.addRow();

teclado.addButton((String("⚡ ") + reles[2].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[3].nombre).c_str());

teclado.addRow();

teclado.addButton((String("⚡ ") + reles[4].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[5].nombre).c_str());

teclado.addRow();

teclado.addButton((String("⚡ ") + reles[6].nombre).c_str());

teclado.addButton("⛔ Todo OFF");

teclado.addRow();

teclado.addButton("💥 Menurapido");

}else{

teclado.addRow();
teclado.addButton((String("⚡ ") + reles[0].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[1].nombre).c_str());

teclado.addRow();

teclado.addButton((String("⚡ ") + reles[2].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[3].nombre).c_str());

teclado.addRow();

teclado.addButton((String("⚡ ") + reles[4].nombre).c_str());
teclado.addButton((String("⚡ ") + reles[5].nombre).c_str());

teclado.addRow();

teclado.addButton("⛔ Todo OFF");
teclado.addButton("💥 Menurapido");
}    

teclado.enableResize();

TBMessage msg;


msg.chatId = atoll(chat_id.c_str());

myBot.sendMessage(
    msg,
    "🎛 CONTROL DE RELE",
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

teclado.addButton("📅 Horarios");
teclado.addButton("📜 historial");

teclado.addRow();

teclado.addButton("🚰 Tanque SI");
teclado.addButton("🚫 Tanque NO");

teclado.addRow();

teclado.addButton("👥 usuarios");
teclado.addButton("🚷 salir");

teclado.enableResize();

TBMessage msg;

msg.chatId = atoll(chat_id.c_str());

myBot.sendMessage(
    msg,
    "📲 ACCESOS RÁPIDOS",
    teclado
);
}
