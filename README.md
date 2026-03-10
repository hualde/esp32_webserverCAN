# ESP32 Multilingual CAN WebServer

Este proyecto implementa un servidor web moderno en un **ESP32** utilizando **ESP-IDF v5.3.2**. Permite controlar secuencias de tramas **CAN (TWAI)** a través de una interfaz web con soporte para múltiples idiomas.

## ✨ Características

-   **Interfaz Premium**: Diseño con efecto *Glassmorphism* (cristal esmerilado) y totalmente responsivo.
-   **Multi-idioma**: Soporte para Español, Inglés y Francés con cambio dinámico sin recarga de página.
-   **Control CAN (TWAI)**: Envío de ráfagas de tramas CAN configuradas a través de un archivo JSON externo.
-   **Configuración vía JSON**: Modifica las tramas sin necesidad de tocar el código fuente en C.
-   **Modo AP**: El ESP32 crea su propio punto de acceso Wi-Fi.

## 🛠️ Requisitos Técnico

-   **Framework**: ESP-IDF v5.3.2.
-   **Hardware**: ESP32 (o variante compatible con TWAI).
-   **Pines CAN (Configurados por defecto)**:
    -   **TX**: GPIO 17 ---> GPIO17 de esp32 va a SN65HVD Pin TX
    -   **RX**: GPIO 16 ---> GPIO16 de esp32 va a SN65HVD Pin RX
    -   **Velocidad**: 500 kbps (Estándar de automoción).

## 📂 Estructura del Proyecto

-   `main/main.c`: Lógica central (Wi-Fi, Servidor HTTP, Driver TWAI, Parser JSON).
-   `main/web/index.html`: Estructura de la web.
-   `main/web/style.css`: Estilos visuales.
-   `main/web/script.js`: Lógica de traducción y llamadas al servidor.
-   `main/web/can_frames.json`: **Archivo de configuración de tramas CAN**.

## 🚀 Instalación y Uso

### 1. Preparar el entorno
Asegúrate de tener instalado y activo el entorno de ESP-IDF v5.3.2.

### 2. Compilar y Flashear
Desde la raíz del proyecto, ejecuta:
```bash
idf.py build flash monitor
```

### 3. Conexión
1.  Busca la red Wi-Fi: **`ESP32_Control_AP`**.
2.  Contraseña: **`12345678`**.
3.  Abre el navegador en: **`http://192.168.4.1`**.

## ⚙️ Configuración de Tramas CAN

Para cambiar las tramas que se envían al pulsar los botones, edita el archivo `main/web/can_frames.json`:

```json
{
    "action1": {
        "steps": [{
            "frames": [
                {
                    "id": "0x064",
                    "data": ["0x02", "0x10", "0xC0", 0, 0, 0, 0, 0],
                    "dlc": 8,
                    "delay_ms": 100
                }
            ]
        }]
    }
}
```
*   `id`: ID CAN en **decimal** (ej. `100`) o en **hex** como string (ej. `"0x064"`, `"064"`) — puedes copiar los IDs del sniffer.
*   `data`: Bytes en decimal o en hex como string (ej. `"0x02"`, `"0xC0"`).
*   `delay_ms`: Pausa en ms tras enviar la trama.

---
Desarrollado para integración en sistemas de diagnosis.
