#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

#define DHTTYPE DHT22

#define MAX_HORARIOS 6
#define MAX_USERS 6
#define MAX_EVENTOS 50

// BOTONES
#define BottAUX 2
#define BottBloqueo 33
#define BottOFF 15
#define BOTON_RESET 4

// DISPLAY
#define SDA_PIN 21
#define SCL_PIN 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// SENSORES
#define Sensor_Suelo 34
#define SENSOR_TANQUE 25
#define DHTPIN 32

#endif