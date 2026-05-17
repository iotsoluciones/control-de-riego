#include "variables.h"

// ===== OBJETOS =====

Preferences prefs;

WiFiManager wm;

WiFiClientSecure clientTelegram;

AsyncTelegram2 myBot(clientTelegram);

TaskHandle_t TelegramTaskHandle = NULL;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHTPIN, DHTTYPE);

// ===== RELES =====

int relePin[8]={23,13,14,19,18,05,27,26};

// ===== HISTORIAL =====

String eventos[MAX_EVENTOS];

int indiceEvento = 0;

// ===== TIMERS =====
unsigned long bloqueoEventosCriticos = 0;
unsigned long timerCambioBomba = 0;
unsigned long ultimoCambioBloqueo = 0;
unsigned long timerWiFi = 0;
unsigned long timerDisplay = 0;
unsigned long timerTanque = 0;
unsigned long timerClima = 0;
unsigned long timerReporte = 0;
unsigned long bloqueoArranque = 0;
unsigned long bloqueoManualTiempo = 0;
unsigned long bloqueoBombaManual = 0;
unsigned long bloqueoLluviaTiempo = 0;
unsigned long tiempoPresionado = 0;
unsigned long timerApagadoRele = 0;
unsigned long timerBomba = 0;
unsigned long timerReinicio = 0;
unsigned long debounceOFF = 0;
unsigned long timerTelegram = 0;
unsigned long timerClock = 0;
unsigned long timerSensor = 0;

// ===== ESTADOS =====

bool OTAEnCurso = false;
bool wifiAnterior = true;
bool bombaEncendida = false;
bool ultimoEstadoTanque = false;
bool avisoInicioEnviado =false;
bool estadoAnteriorBomba = false;
bool tanqueNecesitaAgua = false;
bool esperandoBomba = false;
bool esperandoConfirmacionSalir = false;
bool inicio = false;
bool modoreset = false;
bool botonActivo = false;
bool avisoEnviado = false;
bool bloqueoAnterior = false;
bool delayTanqueActivo = false;
bool lluviaBloqueada = false;
bool bloqueoActivoGlobal = false;
bool sensorSueloActivo = true;
bool avisoHumedadEnviado = false;
bool wifiEstadoAnterior = true;
bool modoTanqueAutomatico = true;
bool bloqueoHumedad = true;
bool esperandoApagadoRele = false;
bool esperaBomba = false;
bool reinicioPendiente = false;
bool bloqueoActual = false;
bool bloqueoSuelo = true;
bool bloqueoFisicoActivo = false;
// ===== CLIMA =====

float probabilidadLluvia = 0;
float probabilidadLluviaSuavizada = 0;

float lat = 0;
float lon = 0;

String ciudad = "Buenos Aires";

// ===== SENSORES =====

float temperatura = 0;
float humedad = 0;
float humedadLimite = 90;

int humedadSuelo = 0;
int humedadSueloLimite = 60;

// ===== TELEGRAM =====

String comandoPendiente = "";
String chatPendiente = "";

String BOTtoken;

String CHAT_ID;


String colaMensajes[20];

int totalMensajes = 0;
// ===== USUARIOS =====

String usuarios[MAX_USERS];

String usuariosID[MAX_USERS];

String usuariosNombre[MAX_USERS];

int cantidadUsuarios = 0;

// ===== RELES =====

Rele reles[8];

// ===== HORARIOS =====

Horario horarios[8][MAX_HORARIOS];

int cantidadHorarios[8]={0};

// ===== CONFIG =====

const char* ntpServer = "pool.ntp.org";

const long gmtOffset_sec = -10800;

// ===== TEXTOS =====

String horaActual = "";

String ultimoEvento = "Sistema iniciado";

// ===== WIFI MANAGER =====

WiFiManagerParameter param_token("token","Token Telegram","",60);

WiFiManagerParameter param_chatid("chatid","Chat ID","",20);

// ===== CONTROL =====

int relePendienteApagado = -1;