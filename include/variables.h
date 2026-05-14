#ifndef VARIABLES_H
#define VARIABLES_H

#include "config.h"

// ===== OBJETOS =====
extern String comandoPendiente;
extern String chatPendiente;

extern bool hayComandoTelegram;
extern String text;
extern String chat_id;
extern String nombreUsuario;

extern Preferences prefs;

extern WiFiManager wm;

extern WiFiClientSecure clientTelegram;

extern AsyncTelegram2 myBot;

extern Adafruit_SSD1306 display;

extern DHT dht;

// ===== RELES =====

extern int relePin[8];

// ===== HISTORIAL =====

extern String eventos[MAX_EVENTOS];

extern int indiceEvento;

// ===== TIMERS =====
extern unsigned long bloqueoEventosCriticos;
extern bool estadoAnteriorBomba;
extern unsigned long timerCambioBomba;
extern unsigned long ultimoCambioBloqueo;
extern unsigned long timerWiFi;
extern unsigned long timerDisplay;
extern unsigned long timerTanque;
extern unsigned long timerClima;
extern unsigned long timerReporte;
extern unsigned long bloqueoArranque;
extern unsigned long bloqueoManualTiempo;
extern unsigned long bloqueoBombaManual;
extern unsigned long bloqueoLluviaTiempo;
extern unsigned long tiempoPresionado;
extern unsigned long timerApagadoRele;
extern unsigned long timerBomba;
extern unsigned long timerReinicio;
extern unsigned long debounceOFF;
extern unsigned long timerTelegram;
extern unsigned long timerClock;
extern unsigned long timerSensor;

// ===== ESTADOS =====
extern bool OTAEnCurso;
extern bool wifiAnterior;
extern bool bombaEncendida;
extern bool ultimoEstadoTanque;
extern bool avisoInicioEnviado;
extern bool tanqueNecesitaAgua;
extern bool esperandoBomba;
extern bool esperandoConfirmacionSalir;
extern bool inicio;
extern bool modoreset;
extern bool botonActivo;
extern bool avisoEnviado;
extern bool bloqueoAnterior;
extern bool delayTanqueActivo;
extern bool lluviaBloqueada;
extern bool bloqueoActivoGlobal;
extern bool sensorSueloActivo;
extern bool avisoHumedadEnviado;
extern bool wifiEstadoAnterior;
extern bool modoTanqueAutomatico;
extern bool bloqueoHumedad;
extern bool esperandoApagadoRele;
extern bool esperaBomba;
extern bool reinicioPendiente;
extern bool bloqueoActual;
extern bool bloqueoSuelo;
extern bool bloqueoFisicoActivo;
// ===== CLIMA =====

extern float probabilidadLluvia;
extern float probabilidadLluviaSuavizada;

extern float lat;
extern float lon;

extern String ciudad;

// ===== SENSORES =====

extern float temperatura;
extern float humedad;
extern float humedadLimite;

extern int humedadSuelo;
extern int humedadSueloLimite;

// ===== TELEGRAM =====

extern String BOTtoken;
extern String CHAT_ID;

// ===== USUARIOS =====

extern String usuarios[MAX_USERS];

extern String usuariosID[MAX_USERS];

extern String usuariosNombre[MAX_USERS];

extern int cantidadUsuarios;

// ===== RELES STRUCT =====

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

extern Rele reles[8];

// ===== HORARIOS =====

struct Horario{
  byte hora;
  byte minuto;
  int duracion;
};

extern Horario horarios[8][MAX_HORARIOS];

extern int cantidadHorarios[8];

// ===== CONFIG =====

extern const char* ntpServer;

extern const long gmtOffset_sec;

// ===== TEXTOS =====

extern String horaActual;

extern String ultimoEvento;

// ===== WIFI MANAGER =====

extern WiFiManagerParameter param_token;

extern WiFiManagerParameter param_chatid;

// ===== CONTROL =====

extern int relePendienteApagado;

#endif