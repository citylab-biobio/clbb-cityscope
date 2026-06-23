# Manual Técnico de Uso de CLBB CityScope

En esta guía se presentan los pasos técnicos a seguir para la instalación de los sistemas que manejan todo el proyecto del CityScope CLBB.

## Requisitos
1. [Docker](https://www.docker.com/products/docker-desktop/)
2. [Arduino IDE](https://www.arduino.cc/en/software/#ide)

## Instalación
Una vez instaladas todas las herramientas, se deben seguir estos pasos para poder levantar todos los servicios necesarios:

### Clonar repositorio
El primer paso es clonar el repositorio en tu máquina local.

```bash
git clone <url-de-este-repo> clbb-cityscope
cd clbb-cityscope
```

### Conexión a la red Wi-Fi
Para que el sistema funcione correctamente, debemos conectar el servidor principal (En nuestro caso es la Raspberry Pi) a la red Wi-Fi. Una vez conectado, es necesario obtener la dirección IP asignada a dicho dispositivo.

Este valor de IP debe insertarse en el archivo `.env` del repositorio principal y en el código del ESP32 para asegurar que todos los componentes se comuniquen dentro de la misma red local. Las secciones correspondientes para cada código son las siguientes:

1. **Código de `clbb-cityscope` (Backend/Frontend)**
   En el archivo `.env`, asigna la IP obtenida a la siguiente variable:
   ```env
   IP_ADDRESS=
   ```

2. **Código del ESP32**
   En el caso del ESP32, además de la IP del servidor, debes definir las credenciales de la red Wi-Fi a utilizar. Donde `WIFI_SSID` corresponde al nombre de la red y `WIFI_PASSWORD` a la clave de acceso.
   ```cpp
   const char* WIFI_SSID = "Tu_Red_WiFi";
   const char* WIFI_PASSWORD = "Tu_Contraseña";
   const char* API_URL = "http://{IP_ADDRESS}:{FRONT_PORT}/api/set_map_state/";
   ```
   *(Nota: Asegúrate de reemplazar explícitamente `{IP_ADDRESS}` y `{FRONT_PORT}` con la IP del servidor y el puerto en el código de tu Arduino)*.

### Flasheo del ESP32
Una vez definidos los parámetros en el código, podemos proceder a flashear el microcontrolador mediante el software **Arduino IDE**.

1. Asegúrate de tener instalada la librería de la placa ESP32. Esto se hace desde la pestaña del **Gestor de Tarjetas**. Busca "ESP32" e instala la opción `"esp32 by Espressif Systems"`.
2. Conecta tu placa ESP32 por USB y selecciona el puerto **COM** correspondiente en el menú `Herramientas > Puerto`.
3. Selecciona tu modelo de ESP32 en el menú `Herramientas > Placa`.
4. Carga el código al ESP32 haciendo clic en el botón **Subir** (Cargar).

Para una guía más visual y práctica, te sugerimos este [video tutorial](https://www.youtube.com/watch?v=ikBlhX-erSw) que explica paso a paso cómo cargar un código en el ESP32 desde el Arduino IDE.

### Construcción con Docker
Una vez que todo lo anterior esté configurado, pasamos a levantar los contenedores de Docker. Los pasos varían ligeramente dependiendo de tu sistema operativo.

#### Linux
En sistemas basados en Linux, el motor de Docker normalmente se ejecuta como un servicio en segundo plano. Por lo tanto, sólo debes asegurarte de tener Docker y Docker Compose instalados.

1. Abre una terminal.
2. Posiciónate en la carpeta raíz del proyecto (`clbb-cityscope`).
3. Ejecuta el siguiente comando:

```bash
docker compose up --build -d
```

#### Windows
En caso de utilizar Windows, antes de ejecutar el proyecto debes asegurarte de abrir la aplicación **Docker Desktop**. Una vez inicializado el motor de Docker, abre una terminal en la carpeta raíz del proyecto (`clbb-cityscope`) y ejecuta el siguiente comando:

```bash
docker compose up --build -d
```

Este comando descargará, compilará y levantará todos los contenedores del proyecto en segundo plano. Una vez termine el proceso de construcción, podrás acceder a los distintos servicios a través de las siguientes URLs en tu navegador:

- **Dashboard:** `http://<IP_ADDRESS>:<FRONT_PORT>/dashboard/`
- **Proyección:** `http://<IP_ADDRESS>:<FRONT_PORT>/projection/`
- **Control remoto:** `http://<IP_ADDRESS>:<FRONT_PORT>/remote-controller/`
- **API:** `http://<IP_ADDRESS>:<API_PORT>/api/`