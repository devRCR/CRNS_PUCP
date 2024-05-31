/*
 * Proyecto: Sistema de Registro y Monitoreo de Detectores de Neutrones Cósmicos
 * Autor: devRCR
 * Fecha: 31/05/2024
 * Descripción: Este proyecto utiliza un ESP32 para registrar y monitorear
 *              los datos de dos detectores, almacenando la información en
 *              una tarjeta SD y enviándola a la plataforma Ubidots.
 */
 
#include <Wire.h>
#include <RTClib.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "UbidotsEsp32Mqtt.h"

/****************************************
 *  Datos para la conexión con Ubidots
 ****************************************/
const char *UBIDOTS_TOKEN = "BBUS-U17lDn5hAx5YEO3cuYxiIwHKFXSNv1";
const char *WIFI_SSID = "POCO_F5";
const char *WIFI_PASS = "87654321";
const char *DEVICE_LABEL = "ESP32_IPEN";
const char *VARIABLE_LABEL_1 = "Detector1";
const char *VARIABLE_LABEL_2 = "Detector2";

Ubidots ubidots(UBIDOTS_TOKEN);

/****************************************
 *  Configuración de la tarjeta SD
 ****************************************/
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 5

/****************************************
 *  Configuración del RTC
 ****************************************/
RTC_DS3231 rtc;

/****************************************
 *  Configuración de Pines de Interrupción
 ****************************************/
#define INTERRUPT_PIN1 34
#define INTERRUPT_PIN2 35

volatile uint32_t pulseCount1 = 0; // Contador de pulsos del Detector 1
volatile uint32_t pulseCount2 = 0; // Contador de pulsos del Detector 2
uint32_t lastTime = 0;             // Marca de tiempo para el último registro de datos

/****************************************
 *  Prototipos de funciones
 ****************************************/
void IRAM_ATTR handleInterrupt1();
void IRAM_ATTR handleInterrupt2();
void logData(DateTime now);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void setupSDCard();
void setupRTC();
void setupInterrupts();
void publishDataToUbidots();
void createLogFileIfNeeded(DateTime now);

/****************************************
 *  Manejo de Interrupciones
 ****************************************/
void IRAM_ATTR handleInterrupt1() {
  pulseCount1++;
}

void IRAM_ATTR handleInterrupt2() {
  pulseCount2++;
}

/****************************************
 *  Configuración Inicial del Sistema
 ****************************************/
void setup() {
  Serial.begin(9600); // Inicializar comunicación serial
  Wire.begin(); // Inicializar comunicación I2C

  // Conectar a WiFi y configurar Ubidots
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setup();
  ubidots.reconnect();

  setupRTC(); // Configurar el RTC
  setupSDCard(); // Configurar la tarjeta SD
  setupInterrupts(); // Configurar las interrupciones

  lastTime = millis(); // Guardar el tiempo inicial
}

/****************************************
 *  Configuración del RTC
 ****************************************/
void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar RTC"); // Error si no se encuentra el RTC
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC perdió energía, configurando tiempo!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Configurar el tiempo si el RTC perdió energía
  }
}

/****************************************
 *  Configuración de la Tarjeta SD
 ****************************************/
void setupSDCard() {
  if (!SD.begin(SD_CS)) {
    Serial.println("Fallo al montar la tarjeta"); // Error si no se puede montar la tarjeta SD
    return;
  }

  DateTime now = rtc.now(); // Obtener la fecha y hora actuales
  createLogFileIfNeeded(now); // Crear el archivo de registro si es necesario
}

/****************************************
 *  Configuración de Interrupciones
 ****************************************/
void setupInterrupts() {
  pinMode(INTERRUPT_PIN1, INPUT_PULLUP); // Configurar el pin de interrupción 1
  pinMode(INTERRUPT_PIN2, INPUT_PULLUP); // Configurar el pin de interrupción 2
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN1), handleInterrupt1, RISING); // Adjuntar interrupción 1
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN2), handleInterrupt2, RISING); // Adjuntar interrupción 2
}

/****************************************
 *  Bucle Principal
 ****************************************/
void loop() {
  DateTime now = rtc.now(); // Obtener la fecha y hora actuales

  if (millis() - lastTime >= 60000) { // 60000 ms para 60 segundos
    // Desactivar interrupciones mientras se guardan los datos
    detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN1));
    detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN2));

    logData(now); // Registrar datos en la tarjeta SD

    publishDataToUbidots(); // Publicar datos en Ubidots

    // Reiniciar contadores
    pulseCount1 = 0;
    pulseCount2 = 0;

    // Reactivar interrupciones después de guardar los datos
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN1), handleInterrupt1, RISING);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN2), handleInterrupt2, RISING);

    lastTime = millis(); // Actualizar la marca de tiempo
  }

  ubidots.loop(); // Mantener la conexión con Ubidots
}

/****************************************
 *  Función para Registrar Datos
 ****************************************/
void logData(DateTime now) {
  char filename[20];
  sprintf(filename, "/%04d%02d%02d.txt", now.year(), now.month(), now.day()); // Crear el nombre del archivo basado en la fecha
  char stringData[40];
  sprintf(stringData, "%02d:%02d:%02d,%4d,%4d\n", now.hour(), now.minute(), now.second(), pulseCount1, pulseCount2); // Crear la cadena de datos

  appendFile(SD, filename, stringData); // Añadir los datos al archivo

  Serial.println(filename); // Imprimir el nombre del archivo
  Serial.println(stringData); // Imprimir la cadena de datos
}

/****************************************
 *  Función para Crear Archivo de Registro si es Necesario
 ****************************************/
void createLogFileIfNeeded(DateTime now) {
  char filename[20];
  sprintf(filename, "/%04d%02d%02d.txt", now.year(), now.month(), now.day()); // Crear el nombre del archivo basado en la fecha

  File file = SD.open(filename);
  if (!file) {
    Serial.println("El archivo no existe, creando archivo..."); // Crear el archivo si no existe
    writeFile(SD, filename, "Timestamp,Detector1 counts,Detector2 counts \r\n");
  } else {
    Serial.println("El archivo ya existe"); // El archivo ya existe
  }
  file.close();
}

/****************************************
 *  Función para Publicar Datos en Ubidots
 ****************************************/
void publishDataToUbidots() {
  Serial.print("DET1: ");
  Serial.print(pulseCount1); // Imprimir el contador del Detector 1
  Serial.print(", DET2: ");
  Serial.println(pulseCount2); // Imprimir el contador del Detector 2

  ubidots.add(VARIABLE_LABEL_1, pulseCount1); // Añadir el valor del Detector 1 a Ubidots
  ubidots.add(VARIABLE_LABEL_2, pulseCount2); // Añadir el valor del Detector 2 a Ubidots
  ubidots.publish(DEVICE_LABEL); // Publicar los datos en Ubidots
}

/****************************************
 *  Función para Escribir en la Tarjeta SD
 ****************************************/
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Escribiendo archivo: %s\n", path); // Imprimir el nombre del archivo
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Fallo al abrir el archivo para escritura"); // Error si no se puede abrir el archivo
    return;
  }
  if (file.print(message)) {
    Serial.println("Archivo escrito"); // Confirmar que el archivo fue escrito
  } else {
    Serial.println("Fallo al escribir"); // Error si falla la escritura
  }
  file.close();
}

/****************************************
 *  Función para Añadir Datos en la Tarjeta SD
 ****************************************/
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Añadiendo datos al archivo: %s\n", path); // Imprimir el nombre del archivo
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Fallo al abrir el archivo para añadir datos"); // Error si no se puede abrir el archivo
    writeFile(SD, path, "Timestamp,Detector1 counts,Detector2 counts \r\n");
    return;
  }
  if (file.print(message)) {
    Serial.println("Mensaje añadido"); // Confirmar que los datos fueron añadidos
  } else {
    Serial.println("Fallo al añadir"); // Error si falla al añadir datos
  }
  file.close();
}
