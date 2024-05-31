# Proyecto: Sistema de Registro y Monitoreo de Detectores

## Descripción
Este proyecto utiliza un ESP32 para registrar y monitorear los datos de dos detectores, almacenando la información en una tarjeta SD y enviándola a la plataforma Ubidots.

## Autor
Renzo Chan Rios (devRCR)

## Fecha
31/05/2024

## Contenido del Repositorio
- **`codigo.ino`**: El código fuente del proyecto en formato Arduino.
- **`README.md`**: Este archivo que describe el proyecto.

## Requisitos
- Hardware:
  - ESP32
  - Módulo de tarjeta SD
  - RTC (DS3231)
- Software:
  - Arduino IDE
  - Bibliotecas necesarias (RTClib, UbidotsEsp32Mqtt, etc.)

## Instalación
1. Clona este repositorio en tu computadora.
2. Abre el archivo `codigo.ino` en Arduino IDE.
3. Conecta tu ESP32 al puerto USB de tu computadora.
4. Compila y carga el código en tu ESP32 utilizando Arduino IDE.
5. ¡Disfruta del monitoreo y registro de tus detectores!

## Configuración
- Configura tus credenciales de WiFi y Ubidots en el archivo `codigo.ino`.
- Asegúrate de conectar correctamente los componentes hardware (SD, RTC, etc.) a tu ESP32.

## Uso
- Enciende tu ESP32 y los detectores.
- Los datos serán registrados en la tarjeta SD y enviados a Ubidots automáticamente.

## Contribución
Las contribuciones son bienvenidas. Si tienes ideas de mejoras, correcciones de errores, o cualquier otra sugerencia, siéntete libre de abrir un _issue_ o enviar un _pull request_.

## Licencia
Este proyecto está bajo la [Licencia MIT](LICENSE).