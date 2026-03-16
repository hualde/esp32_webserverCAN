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

## 🧭 Flujo de Acciones (tramas por paso)

> Nota: `can_frames.json` está embebido en el firmware. Tras cambiarlo, recompila y flashea.

### Acción 1 — Codificación

**Paso 1 (seguridad UDS)**
- `TX 712 → 02 3E 00 FF FF FF FF FF`
- `RX 77C → 02 7E 00 AA AA AA AA AA`
- `TX 712 → 02 10 03 FF FF FF FF FF`
- `RX 77C → 06 50 03 00 32 01 F4 AA`
- `TX 712 → 02 27 03 FF FF FF FF FF`
- `RX 77C → 06 67 03 B1 B2 B3 B4 AA` (seed)
- `TX 712 → 06 27 04 K1 K2 K3 K4 FF` (K3K4 = B3B4 + 0x4B31)
- `RX 77C → 02 67 04 AA AA AA AA AA` (acceso OK → paso 2)

**Paso 2 (escritura coding)**
- En la web se seleccionan los bits para construir `XX`:
  - Bit 0: DSR (activar o no)
  - Bit 1: Asistente aparcamiento (activar o no)
  - Bits 2–3 (TSC, selección única): `00` desactivado, `04` con Lernwerten, `08` sin valores
  - Bit 4: Asistente mantenimiento carril
  - Bit 5: Sensor ángulo dirección externo
  - Bit 7: Perfil conductor activo
- `TX 712 → 07 2E 06 00 XX 1F 00 00`
- `RX 77C → 03 6E 06 00 AA AA AA AA`

**Paso 3 (reset y sesión)**
- `TX 712 → 02 11 02 FF FF FF FF FF`
- `RX 77C → 03 7F 11 78 AA AA AA AA` (pending)
- `RX 77C → 02 51 02 AA AA AA AA AA` (reset OK)
- `TX 712 → 02 3E 00 FF FF FF FF FF`
- `RX 77C → 02 7E 00 AA AA AA AA AA`
- `TX 712 → 02 10 03 FF FF FF FF FF`
- `RX 77C → 06 50 03 00 32 01 F4 AA`

**Paso 4 (fingerprint y fecha)**
- `TX 712 → 10 09 2E F1 98 0A 2C 2F`
- `RX 77C → 30 0F 03 AA AA AA AA AA`
- `TX 712 → 21 CF 86 9F FF FF FF FF`
- `RX 77C → 03 6E F1 98 AA AA AA AA`
- `TX 712 → 06 2E F1 99 26 03 06 FF`
- `RX 77C → 03 6E F1 99 AA AA AA AA`

**Paso 5 (verificación)**
- `TX 712 → 03 22 06 00 FF FF FF FF`
- `RX 77C → 07 62 06 00 XX 1F 00 00` (XX debe coincidir con paso 2)

### Acción 2 — Ajuste de los topes

**Paso 1 (seguridad UDS)**
- `TX 712 → 02 3E 00 FF FF FF FF FF`
- `RX 77C → 02 7E 00 AA AA AA AA AA`
- `TX 712 → 02 10 03 FF FF FF FF FF`
- `RX 77C → 06 50 03 00 32 01 F4 AA`
- `TX 712 → 02 27 03 FF FF FF FF FF`
- `RX 77C → 06 67 03 B1 B2 B3 B4 AA` (seed)
- `TX 712 → 06 27 04 K1 K2 K3 K4 FF` (K3K4 = B3B4 + 0x4B31)
- `RX 77C → 02 67 04 AA AA AA AA AA` (acceso OK → paso 2)

**Paso 2 (lectura estado + iniciar rutina)**
- `TX 712 → 03 22 18 1B FF FF FF FF`
- `RX 77C → 10 08 62 18 1B 01 01 01`
- `TX 712 → 30 00 00 FF FF FF FF FF`
- `RX 77C → 21 01 01 AA AA AA AA AA`
- `TX 712 → 04 31 01 04 16 FF FF FF`
- `RX 77C → 04 71 01 04 16 AA AA AA`
- `TX 712 → 04 31 03 04 16 FF FF FF`
- `RX 77C → 07 71 03 04 16 01 FF FF` (rutina activa → paso 3)

**Paso 3 (polling — operario gira volante tope a tope)**
- `TX 712 → 03 22 18 16 FF FF FF FF` (repetir cada ~400 ms)
- `RX 77C → 07 62 18 16 00 01 01 01` (en curso)
- `RX 77C → 07 62 18 16 00 00 01 00` (completado → paso 4)

**Paso 4 (cierre)**
- `TX 712 → 04 31 02 04 16 FF FF FF`
- `RX 77C → 04 71 02 04 16 AA AA AA`
- `TX 712 → 04 31 03 04 16 FF FF FF`
- `RX 77C → 07 71 03 04 16 02 FF FF`
- `TX 712 → 03 22 19 23 FF FF FF FF`
- `RX 77C → 10 09 62 19 23 01 00 28`
- `TX 712 → 30 00 00 FF FF FF FF FF`
- `RX 77C → 21 64 D7 E2 AA AA AA AA`
- `TX 712 → 04 14 FF FF FF FF FF FF`
- `RX 77C → 03 7F 14 78 AA AA AA AA` (pending)
- `RX 77C → 01 54 AA AA AA AA AA AA` (DTCs borrados)

---
Desarrollado para integración en sistemas de diagnosis.
