# ESP32 CityScope

Código para el control de los lectores RFID del sistema CityScope CLBB. Controla un ESP32 conectado a 7 sensores NFC PN532 a través del bus SPI de forma directa (usando un pin SS/CS por sensor). Detecta las placas físicas colocadas en la maqueta y envía su estado a la API del sistema cada 2 segundos, implementando además tolerancia a fallos y auditoría de hardware.

---

## Hardware requerido

| Componente | Cantidad | Descripción |
|---|---|---|
| ESP32 | 1 | Microcontrolador principal |
| PN532 | 7 | Módulos lectores NFC/RFID |
| Placas RFID | 14 | 7 placas estado actual + 7 estado futuro |

---

## Conexiones

Todos los sensores PN532 se conectan en paralelo al bus SPI estándar del ESP32, compartiendo los pines de reloj y datos. Sin embargo, cada sensor tiene su propio pin de selección (`SS`/`CS`) para que el ESP32 pueda comunicarse con ellos de forma individual.

### Bus SPI Compartido

| Pin ESP32 | Pin PN532 | Función |
|---|---|---|
| GPIO 18 | SCK | Reloj SPI |
| GPIO 19 | MISO | Datos entrada (MISO) |
| GPIO 23 | MOSI | Datos salida (MOSI) |

### Pines de Selección (SS / Chip Select)

| Slot Lógico | Pin ESP32 |
|---|---|
| Sensor 1 | GPIO 4 |
| Sensor 2 | GPIO 16 |
| Sensor 3 | GPIO 17 |
| Sensor 4 | GPIO 21 |
| Sensor 5 | GPIO 32 |
| Sensor 6 | GPIO 25 |
| Sensor 7 | GPIO 26 |

---

## Configuración

Antes de flashear el firmware, edita las siguientes constantes en `ESP32_CityScope.ino`:

```cpp
const char* WIFI_SSID = "Tu_Red_WiFi";      // Nombre de la red WiFi
const char* WIFI_PASSWORD = "Tu_Password";  // Contraseña de la red WiFi
const char* API_URL = "http://<IP_ADDRESS>:<FRONT_PORT>/api/set_map_state/";
```

> El ESP32 debe estar en la misma red local que el equipo que corre `clbb-cityscope`.

---

## Librerías requeridas

Instalar desde el Library Manager de Arduino IDE:

- `Adafruit PN532` — comunicación con los módulos NFC
- Las librerías `WiFi` y `HTTPClient` ya vienen incluidas en el core del ESP32.

---

## Placas registradas

El firmware mapea el UID hexadecimal de cada placa a un ID lógico que la API entiende. La tabla actual es:

| UID | ID Lógico | Slot | Estado |
|---|---|---|---|
| BBBE1406 | 13 | 1 | Actual |
| 3FBA5E02 | 14 | 2 | Actual |
| 6922EB18 | 15 | 3 | Actual |
| 6094FC55 | 16 | 4 | Actual |
| 4FA95802 | 17 | 5 | Actual |
| 70190B55 | 18 | 6 | Actual |
| 60D8F155 | 19 | 7 | Actual |
| 3FEC4002 | 20 | 1 | Futuro |
| 35284E06 | 21 | 2 | Futuro |
| 69043A18 | 22 | 3 | Futuro |
| 0BAE1706 | 23 | 4 | Futuro |
| 1C164C06 | 24 | 5 | Futuro |
| 60801355 | 25 | 6 | Futuro |
| B963C855 | 26 | 7 | Futuro |

Para registrar una placa nueva o al realizar cambios de tag NFC, debes agregar su UID y un ID lógico nuevo al mapa `ID_list` en el código.

---

## Funcionamiento y Tolerancia a Fallos

El nuevo firmware elimina el uso del multiplexor e implementa un sistema de topología directa y mucho más robusto:

```text
setup()
    ↓
Conectar WiFi
    ↓
Inicializar 7 sensores PN532 de forma escalonada (mitigador de brownout)

loop() cada 2 segundos
    ↓
Auditoría de Hardware:
    ├── ¿El sensor responde? (Hardware sano)
    │   ├── Leer tarjeta
    │   └── Actualizar caché con la tarjeta leída (o 0 si no hay)
    └── ¿El sensor no responde? (Fallo SPI / Físico)
        └── Recuperar el último valor conocido desde la Caché (Tolerancia a fallos)
    ↓
Generar Diagnóstico y enviar cadena a la API
GET /api/set_map_state/?slots=13,0,15,16,0,18,19
```

**Tolerancia a fallos**: El código incluye un arreglo de memoria (`cacheEstados`). Si un sensor se desconecta temporalmente, experimenta una caída de tensión o falla en el bus SPI, el sistema reporta el sensor caído en el log, pero mantiene e inyecta a la API el último estado que tenía en memoria para ese slot. Esto evita que la plataforma registre un "vacío" erróneo y desconfigure la ciudad bruscamente.

---

## Monitoreo

Conecta el ESP32 por USB y abre el Serial Monitor a **115200 baudios** para ver el estado y los diagnósticos de tolerancia a fallos en tiempo real:

```text
[WIFI] Conectado a la red WiFi
[WIFI] IP asignada: 192.168.1.x
[NFC] Iniciando escaneo de sensores en topología directa...
[NFC] Sensor ENCONTRADO en Canal 1
...
[STATUS] Listo

[STATUS] Iniciando escaneo completo...
[NFC] Tarjeta detectada en Canal 1
------------------------------------------------
[DIAGNÓSTICO DE BUS SPI]
  -> Sensores EN LÍNEA (Hardware OK): Canales [ 1 2 3 4 5 6 7 ]
[STATUS] Cadena generada para API: 13,14,0,16,0,18,19
[STATUS] Enviando carga...
[HTTP] Código respuesta: 200
[STATUS] Ciclo completado. Esperando 2s...
```