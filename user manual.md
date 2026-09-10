# Manual de uso — HMI de captación de pozos (Aysafi)

Placa: **Cheap Yellow Display** (ESP32-2432S028R, pantalla táctil 320×240).

El HMI muestra en vivo **2 estaciones de bombeo** (nivel, caudal, acumulados,
alarmas) y permite operarlas: silenciar/reconocer alarmas, poner la sirena en
AUTO/MANUAL y poner a cero los acumulados. Lee los datos por **Modbus TCP** del
LOGO! 9 (o del simulador). Si pierde el enlace, cae solo a datos simulados y lo
avisa.

---

## 1. Primer encendido

1. Inserta una **microSD** (FAT32) en la ranura lateral. En el primer arranque el
   HMI crea `hmi_config.json` con los valores de fábrica.
2. Al encender verás el splash y luego la lista de **ESTACIONES**.
3. Entra a **Ajustes** (icono de engranaje) → **Configuración** e introduce el
   **PIN** (de fábrica `1234`).
4. Rellena la **red WiFi** de planta, el **destino del PLC** (IP, puerto, unit) y,
   si quieres, los nombres de cada estación y un PIN nuevo. **GUARDAR** →
   "Reiniciar" para que tome la red.
5. En *Ajustes / Diagnóstico*, "Enlace PLC" debe pasar a **CONECTADO** (verde).

Sin microSD el HMI funciona igual: guarda la configuración en su memoria interna
(NVS). La microSD solo añade un archivo editable y sobrevive a un borrado de NVS.

---

## 2. Pantallas

| Pantalla | Cómo se llega | Qué muestra / hace |
|---|---|---|
| **Estaciones** | inicio | 2 tarjetas: nivel, caudal, presión/sirena/tapa, RSSI, estado (OK / ALARMA / SIN ENLACE). Cabecera con origen (PLC-SIM / LOGO! real) y latido. |
| **Detalle** | tocar una tarjeta | Arco de nivel, caudal, acumulado del día, primera alarma. Estado **PENDIENTE ACK** si hay una alarma latcheada sin reconocer. Botones **SILENCIAR**, **ACCIONES**, histórico. |
| **Acciones** | *Detalle → ACCIONES* | **SILENCIAR** sirena · **RECONOCER ALARMAS** (limpia las pendientes) · **SIRENA AUTO/MANUAL** · **RESET DÍA / MES** de los acumulados. Todas piden confirmación. |
| **Histórico** | *Detalle → lista* | Acumulado de los últimos días. |
| **Ajustes / Diagnóstico** | engranaje | Enlace PLC, IP del HMI, WiFi + RSSI, origen/latido, tramas OK/ERR, estado del almacenamiento. Botones **Configuración**, **Escala**, **Recalibrar**. |
| **Rangos de escala** | *Ajustes → Escala* | Edita el cero/span crudo↔ingeniería, unidad y filtro de nivel y caudal, **por estación**, y lo **APLICA al PLC**. La escala vive en el PLC; esto es el editor. |
| **PIN** | antes de *Configuración* | Teclado 0–9. 3 fallos → vuelve a Ajustes. |
| **Configuración** | *Ajustes → Configuración* (tras PIN) | Red WiFi, destino del PLC, nombres de estación, **tema**, PIN nuevo, **Buscar actualización**. |

---

## 3. Alarmas

Las alarmas las calcula el PLC. En el HMI:

- **Activas**: se ven en el Detalle y en la tarjeta de Estaciones.
- **Latcheadas** (marcha en seco, tapa abierta): quedan marcadas aunque la causa
  desaparezca, como **PENDIENTE ACK**. Se limpian con **RECONOCER ALARMAS** en la
  pantalla de Acciones, y solo si la causa ya no está presente.
- **SILENCIAR** calla la sirena hasta la próxima alarma nueva; no borra nada.

---

## 4. Tema y brillo

- **Brillo**: automático según la luz ambiente (sensor LDR). Se calibra y ajusta
  por consola serie (USB, 115200 baudios): `light`, `light cal bright|dark`,
  `light set <param> <n>`, `light save`.
- **Tema de colores**: claro / oscuro / **automático** (sigue la luz ambiente).
  Se cambia en *Configuración* o por consola: `theme auto|claro|oscuro`.
- **Hora**: se sincroniza por Internet (SNTP) cuando hay WiFi. Servidor y zona
  horaria por consola: `time server <host>`, `time tz <TZ>`.

Todo se guarda en la microSD/NVS y se conserva al reiniciar.

---

## 5. Actualización de firmware (OTA)

El HMI se actualiza **por WiFi**, sin cable, cuando hay una versión nueva
publicada.

- **A mano**: *Ajustes → Configuración* (tras el PIN) → **Buscar actualización**.
  Confirma; la pantalla pasa a "Actualización de firmware" con el progreso. Al
  terminar se reinicia solo con la versión nueva.
- **Automático**: cada 6 horas, con WiFi conectada, comprueba en silencio; solo
  toma la pantalla si hay algo que instalar.
- La red WiFi configurada debe tener **salida a Internet**.

**No cortes la alimentación** mientras la pantalla diga "Escribiendo...".

---

## 6. MODO BÁSICO

Si el HMI se reinicia de forma anormal (cuelgue, corte de tensión) varias veces
seguidas sin llegar a estabilizarse, arranca en **MODO BÁSICO**: ignora la
microSD y los ajustes guardados y usa solo los valores de fábrica, para poder
diagnosticar. La pantalla lo indica. Corrige la causa (SD dañada, alimentación
inestable) y reinicia; tras un arranque estable vuelve a la normalidad.

---

## 7. Problemas frecuentes

| Síntoma | Causa probable / solución |
|---|---|
| "Enlace PLC: SIN CONEXIÓN (sim)" | WiFi o IP/puerto del PLC mal en *Configuración*; el PLC no responde en `:puerto`; están en segmentos de red distintos |
| Los datos se ven pero "origen: PLC-SIM" | está leyendo del simulador, no del LOGO! real — revisa el enlace y que el LOGO! publique el bloque global |
| El táctil responde descalibrado | *Ajustes → Recalibrar* y toca las cruces |
| "almacenamiento: NVS (sin microSD)" | no hay tarjeta o no montó — funciona igual; para volver a usarla, tarjeta FAT32 y reinicia |
| El PIN no lo acepta | de fábrica es `1234`; si lo cambiaste y lo olvidaste, hay que borrar la configuración (soporte) |
| "Buscar actualización" no encuentra nada | ya está en la última versión, o la WiFi no tiene salida a Internet |
| Arrancó en **MODO BÁSICO** | hubo varios reinicios anormales; ver sección 6 |
| Pantalla muy oscura o muy clara | el brillo va por el sensor de luz; calíbralo por consola (`light cal bright` / `light cal dark`) |
