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
    -   **TX**: GPIO 18
    -   **RX**: GPIO 19
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
    "action1": [
        {
            "id": 1861,
            "data": [2, 16, 192, 0, 0, 0, 0, 0],
            "dlc": 8,
            "delay_ms": 100
        }
    ]
}
```
*   `id`: Identificador de la trama (en decimal. Ejemplo: 1861 es `0x745`).
*   `data`: Array de 8 bytes con los datos.
*   `delay_ms`: Pausa tras enviar la trama actual.

---
Desarrollado para integración en sistemas de diagnosis.
