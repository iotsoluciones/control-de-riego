#include <WiFi.h>
#include <web_server.h>
#include <ElegantOTA.h>

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


// ─────────────────────────────────────────────
//  INIT
// ─────────────────────────────────────────────

void iniciarSistema() {

    Serial.begin(115200);
    Serial.println("TEST SERIAL");

    // --- Preferencias config ---
    prefs.begin("config", true);
    lat = prefs.getFloat("lat", -34.60);
    lon = prefs.getFloat("lon", -58.38);
    prefs.end();

    // --- Preferencias usuarios ---
    prefs.begin("users", true);
    cantidadUsuarios = prefs.getInt("cant", 0);
    for (int i = 0; i < cantidadUsuarios; i++) {
        usuariosID[i]     = prefs.getString(("id"  + String(i)).c_str(), "");
        usuariosNombre[i] = prefs.getString(("nom" + String(i)).c_str(), "");
    }
    prefs.end();

    // --- Pines ---
    pinMode(BottBloqueo,  INPUT);
    pinMode(Sensor_Suelo, INPUT);
    pinMode(BottAUX,      INPUT);
    pinMode(BOTON_RESET,  INPUT);
    pinMode(BottOFF,      INPUT);
    pinMode(SENSOR_TANQUE,INPUT);

    // --- Preferencias eventos ---
    prefs.begin("eventos", true);
    indiceEvento = prefs.getInt("indice", 0);
    int totalEventos = prefs.getInt("total", 0);
    for (int i = 0; i < MAX_EVENTOS; i++) {
        eventos[i] = prefs.getString(("e" + String(i)).c_str(), "");
    }
    prefs.end();

    // --- Display ---
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Error display");
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);  display.println("SolucionesIOT");
    display.setCursor(0, 20); display.println("Configurar WiFi...");
    display.display();

    // --- Sensores / Relés / WiFi ---
    dht.begin();
    iniciarReles();
    iniciarWifi();

    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    clientTelegram.setInsecure();
    clientTelegram.setTimeout(1000);
    myBot.setUpdateTime(500);
    myBot.setTelegramToken(BOTtoken.c_str());
    myBot.begin();
    delay(50);

  
    display.clearDisplay();
    display.setCursor(0, 0);  display.println("SolucionesIOT");
    display.setCursor(0, 20); display.println("WiFi OK - Espere...");
    display.display();
    delay(50);

    // --- Servidor Web (todo movido a web_server.cpp) ---
    iniciarServidorWeb();
    
    
    // --- Preferencias config (resto) ---
    prefs.begin("config", true);
    humedadLimite          = prefs.getFloat("humedadLimite", 90);
    ciudad                 = prefs.getString("ciudad", "Buenos Aires");
    modoTanqueAutomatico   = prefs.getBool("modoTanque",  true);
    sensorSueloActivo      = prefs.getBool("sensorSuelo", true);
    prefs.end();

    // --- Hora NTP ---
    configTime(gmtOffset_sec, 0, ntpServer);
    struct tm timeinfo;
    int intentos = 0;
    while (!getLocalTime(&timeinfo) && intentos < 5) {
        delay(50);
        intentos++;
    }

    cargarHorarios();
    leerSensores();

    inicio          = true;
    bloqueoArranque = millis();
}


void loopSistema() {
    
    loopServidorWeb();   // ← reemplaza server.handleClient()

    if (OTAEnCurso) return;   // ⛔ corta TODO el loop mientras OTA en curso

    // --- Aviso de inicio ---
    if (inicio && WiFi.status() == WL_CONNECTED) {
        if (millis() - bloqueoArranque > 5000 && !avisoInicioEnviado) {
            Serial.println("ENVIANDO AVISO INICIO A TODOS");
            enviarATodos("☑️ Sistema iniciado correctamente");
            avisoInicioEnviado = true;
            inicio = false;
        }
    }

    // --- Bloqueo de seguridad ---
    bool lectura = digitalRead(BottBloqueo);
    if (lectura != bloqueoAnterior) {
        delay(30);
        lectura = digitalRead(BottBloqueo);
        if (lectura != bloqueoAnterior) {
            bloqueoActual = lectura;
            bloqueoSistemaCompleto();
            bloqueoAnterior = lectura;
        }
    }

    if (bloqueoActivoGlobal) {
        mostrarDisplayBloqueado();
        return;
    }


    static unsigned long timerTelegram = 0;
    if(millis() - timerTelegram > 250){
        timerTelegram = millis();
        
         manejarTelegram();
        procesarMensajesPendientes();
    }


    // --- Debug heap ---
    static unsigned long ultimoPrint = 0;
    if (millis() - ultimoPrint > 2000) {
        ultimoPrint = millis();
        Serial.print("Loop OK | Heap: "); Serial.print(ESP.getFreeHeap());
        Serial.print(" | RSSI: ");        Serial.println(WiFi.RSSI());
    }

    unsigned long nowe = millis();

    // --- Botón RESET WiFi ---
    bool estadoBoton = digitalRead(BOTON_RESET);
    if (estadoBoton == HIGH && !botonActivo) {
        tiempoPresionado = millis();
        botonActivo = true;
        modoreset   = true;
    }

    if (modoreset) {
        if (estadoBoton == HIGH) {
            resetWifi();
        } else {
            enviarTelegram(CHAT_ID, "❌ Reset Red WiFi cancelado - ⛔ NO SE PUDO EJECUTAR LA FUNCION!!");
            botonActivo  = false;
            avisoEnviado = false;
            modoreset    = false;
        }
        hayComandoTelegram = false;
        return;
    }

    // --- Display ---
    if (millis() - timerDisplay > 500) {
        timerDisplay = millis();
        actualizarDisplay();
    }

    if (hayComandoTelegram) procesarTelegram();

    controlarBotonAUX();
    controlarTanque();
    controlarBomba();

    if (nowe - timerClock > 2000) {
        timerClock = nowe;
        actualizarHora();
    }


//////////--  conteto antes de expirar admin temporal -- //////////    
    if(adminTemporal){

    if(millis()-tiempoAdmin> 30000){   /// expira admin temporal cada 30 segundos
        adminTemporal=false;
        Serial.println("Admin expirado");
    }
}


//// COMANDO PROTEGIDO PENDIENTE (ejemplo: cambio de clave)

if(esperandoClave &&
millis()-tiempoComandoProtegido>10000    /// si pasaron mas de 10 segundos, se cancela el comando protegido pendiente
){
    esperandoClave=false;
    comandoProtegidoPendiente="";
}



   
///-------LEEMOS DHT Y SENSOR DE SUELO CADA 5 SEGUNDOS-------///
static unsigned long tSensores = 0;
unsigned long ahora = millis();
if(ahora - tSensores > 5000){
    tSensores = ahora;
    leerSensores();
}


    // --- Horarios de riego ---
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        controlMultiplesHorarios(timeinfo.tm_hour, timeinfo.tm_min);
    }

    // --- WiFi ---
    if (millis() - bloqueoEventosCriticos > 5000) {
        if (nowe - timerWiFi > 5000) {
            timerWiFi = nowe;
            reconectarWiFi();
            controlarWiFi();
        }
    }

    // --- Clima ---
    if (millis() - bloqueoEventosCriticos > 18000) {
        if (nowe - timerClima > 18000000) {
            timerClima = nowe;
            consultarClima();
        }
    }

    // --- Apagado de relé pendiente ---
    if (esperandoApagadoRele && nowe - timerApagadoRele >= 60000) {
        digitalWrite(relePin[relePendienteApagado], HIGH);
        reles[relePendienteApagado].encendido = false;
        esperandoApagadoRele = false;
    }

    // --- Reporte periódico ---
    if (nowe - timerReporte > 18000000) {
        enviarATodos("📡 Sistema OK\n");
        timerReporte = nowe;
    }

    // --- Botón todos OFF ---
    if (digitalRead(BottOFF) == HIGH && nowe - debounceOFF > 10000) {
        debounceOFF = nowe;
        apagarTodosReles();
    }

    // --- Reinicio pendiente ---
    if (millis() - bloqueoEventosCriticos > 3000) {
        if (reinicioPendiente && nowe - timerReinicio >= 80000) {
            ESP.restart();
        }
    }

    // --- Soltar botón RESET ---
    if (estadoBoton == LOW && botonActivo) {
        enviarTelegram(CHAT_ID, "No se pudo realizar el reset WiFi desde el botón");
        botonActivo  = false;
        avisoEnviado = false;
        modoreset    = false;
    }

  }

