# Ingeniería de Materiales — WRO Future Engineers 2026

<p align="center">
  <img src="v-photos/FINAL/Front%20Picture%20Final.png" width="750">
</p>


Este repositorio contiene los materiales de ingeniería del modelo de un vehículo autónomo que participa en la competencia WRO Future Engineers en la temporada 2026.

## Contenido

* `t-photos` contiene 2 fotos del equipo (una oficial y una foto divertida con todos los miembros del equipo)
* `v-photos` contiene 6 fotos del vehículo (de todos los lados, desde arriba y desde abajo)
* `video` contiene el archivo video.md con el enlace a un video donde se demuestra la conducción autónoma en ambos desafíos
* `schemes` contiene los diagramas esquemáticos en formato JPEG o PNG de los componentes electromecánicos, ilustrando todos los elementos electrónicos y motores utilizados en el vehículo y cómo se conectan entre sí
* `src` contiene el código del software de control de todos los componentes que fueron programados para participar en la competencia
* `models` contiene los archivos STL de los modelos utilizados por la impresora 3D para producir los elementos del vehículo, incluyendo los soportes de sensores y la montura de la cámara
* `other` contiene documentación complementaria, incluyendo hojas de datos de componentes, bitácoras de pruebas y archivos de configuración auxiliares

---

## Equipo

**TEAM VOLT**

| Miembro | Edad |
|---|---|
| Carlos Jorge Rojas Ruiz | 17 |
| Stephano Wan Rong | 18 |
| Aadidev Nappanveetil Akhilesh | 17 |

---

## Introducción

Nuestro equipo **TEAM VOLT** creó nuestro auto de conducción autónoma **"Volt"** para participar en la competencia WRO Future Engineers 2026 en Lima, Perú. "Volt" es un vehículo autónomo de cuatro ruedas diseñado para navegar de forma completamente independiente alrededor de un circuito de 3x3 metros, detectar y esquivar obstáculos de colores, y completar tres vueltas de manera precisa y repetible sin ninguna intervención humana.

El nombre "Volt" refleja la energía y velocidad con la que nuestro equipo abordó cada etapa del desarrollo: desde el diseño mecánico hasta la integración de sensores y la programación del comportamiento autónomo.

**Por favor, lee los archivos README.md en cada carpeta para comprender mejor nuestra documentación.**

---

## Visión General del Vehículo

"Volt" es un vehículo de tracción trasera con dirección delantera controlada por servomotor. Su arquitectura fue diseñada para ser modular, liviana y fácil de mantener durante las sesiones de competencia. El controlador principal es un **Arduino Mega 2560**, seleccionado por su amplia cantidad de pines digitales y su compatibilidad con las librerías de control de motores y sensores que utilizamos.

### Dimensiones y Peso

El vehículo cumple con los requisitos establecidos en las reglas de WRO 2026:
- Dimensiones máximas: 300 x 200 mm de planta, 300 mm de altura
- Peso total aproximado: bajo 1.5 kg

---

## Arquitectura de Hardware

### Controlador Principal

El cerebro del vehículo es un **Arduino Mega 2560**. Elegimos esta plataforma porque ofrece suficientes pines digitales para conectar simultáneamente cuatro sensores ultrasónicos, un sensor de color, una cámara de visión, un servo y el driver de motor, sin necesidad de multiplexores adicionales.

### Sistema de Tracción

El sistema de propulsión utiliza motores **DG01D con caja reductora 48:1**, conectados al driver de motor **L298N**. Esta combinación proporciona suficiente torque para mantener la velocidad en el circuito mientras el L298N gestiona la dirección y velocidad del motor mediante señales PWM desde el Arduino.

Conexiones del driver L298N:
- **ENA** → Pin 12 (control de velocidad PWM)
- **IN1** → Pin 11
- **IN2** → Pin 10
- **VS (+12V)** → Batería principal
- **GND** → Tierra común

### Sistema de Dirección

La dirección delantera es controlada por un **servomotor SG90** conectado al pin 9 del Arduino. El servo mueve ambas ruedas delanteras simultáneamente a través de un varillaje mecánico. Los ángulos calibrados son:

- **Centro (recto):** 90°
- **Máximo izquierda:** 40°
- **Máximo derecha:** 140°

Una regla fundamental en nuestro código es que el servo se inicializa y centra en 90° como la primera operación absoluta en el `setup()`, con un delay de 1000ms, para evitar cualquier deriva al encender el vehículo.

### Sensores de Distancia — Ultrasonicos HC-SR04

Utilizamos **cuatro sensores ultrasónicos HC-SR04** para medir distancias en las cuatro direcciones cardinales. Esto le permite al vehículo detectar paredes, esquinas y la apertura del corredor interior en cada giro.

| Sensor | Trig | Echo |
|--------|------|------|
| Frontal | 30 | 32 |
| Izquierdo | 24 | 22 |
| Derecho | 50 | 52 |
| Trasero | 28 | 26 |

El sensor izquierdo y derecho son los más críticos para la lógica de detección de esquinas. Cuando el vehículo se aproxima a un giro, el sensor del lado interior del circuito detecta que la pared se aleja (de ~42 cm a ~238 cm), lo cual activa el giro. El umbral de detección establecido es de **120 cm**.

### Sensor de Color — TCS3200

Un sensor de color **TCS3200** está montado en la parte inferior del vehículo, apuntando hacia el suelo, para detectar las líneas naranjas y azules del circuito. Estas líneas se usan para contar las vueltas completadas.

Conexiones:
- S0 → 34, S1 → 36, S2 → 38, S3 → 40, OUT → 42

Valores de calibración confirmados sobre el tapete real de competencia:

| Superficie | R | G | B |
|------------|---|---|---|
| Blanco | ~21 | ~23 | ~7 |
| Naranja | ~24 | ~51 | ~13 |
| Azul | ~90 | ~93 | ~24 |

Umbrales de detección:
- **Naranja:** `G > 35 AND R < 40`
- **Azul:** `G > 70`

### Cámara de Visión — Keyestudio Sentry Vision 2

La **Keyestudio Sentry Vision 2** es una cámara de visión artificial con procesamiento interno que se comunica con el Arduino via **I2C** (SDA → pin 20, SCL → pin 21). Funciona en modo de detección de blobs de color y fue entrenada con dos colores personalizados:

- **Label 13** → Pilar rojo
- **Label 14** → Pilar verde

La cámara retorna el label del objeto detectado y su posición horizontal X (0–100) dentro del campo de visión, lo que permite al vehículo saber en qué lado de la cámara está el pilar y reaccionar correspondientemente.

### Sistema de Alimentación

El vehículo opera con **una sola batería de 7.4V** distribuida a través de un riel de breadboard:

- Batería + → L298N VS (alimentación motores)
- Batería + → Arduino VIN (alimentación lógica)
- Batería − → L298N GND y Arduino GND (tierra común)

El regulador interno del Arduino Mega convierte los 7.4V a 5V estables para alimentar todos los sensores conectados a sus pines de 5V y GND.

---

## Arquitectura de Software

Todo el software de control está escrito en **C++ para Arduino IDE** y se encuentra en la carpeta `src`. El código fue desarrollado de forma iterativa con múltiples prototipos de diagnóstico antes de llegar al código final de competencia.

### Open Challenge

El código del Open Challenge (`open_challenge_auto.ino`) navega el circuito de forma autónoma usando únicamente los sensores ultrasónicos y el sensor de color. Las características principales son:

**Detección automática de dirección:** El vehículo no necesita configuración manual para saber si el circuito es en sentido horario (CW) o antihorario (CCW). Al detectar la primera esquina, determina automáticamente la dirección y desactiva el sensor opuesto para el resto de la carrera, evitando giros incorrectos.

**Detección de esquinas por sensor lateral:** En lugar de usar el sensor frontal (que puede confundirse con los pilares del Obstacle Challenge), usamos el sensor lateral. Cuando la distancia lateral supera 120 cm, significa que el corredor interior se abrió y es momento de girar.

**Lógica de giro con commit mínimo:** Al detectar una esquina, el servo se fija a 40° o 140° durante un mínimo de 900ms antes de verificar si la pared regresó. Esto evita que el robot abandone el giro prematuramente al ver un falso positivo del sensor.

**Cooldown entre esquinas:** Después de cada giro hay un período de cooldown de 650ms durante el cual el sensor lateral no puede activar otro giro. Esto previene que el robot detecte el hueco de donde vino y gire de nuevo incorrectamente.

**Conteo de vueltas:** El sensor de color TCS3200 cuenta las líneas naranjas del circuito. Cada vuelta cruza 4 líneas naranjas, por lo tanto 3 vueltas = 12 cruces de naranja. Al detectar el cruce número 12, el robot espera 250ms y se detiene.

### Obstacle Challenge

El código del Obstacle Challenge (`obstacle_challenge_test.ino`) incorpora toda la lógica del Open Challenge y agrega la detección y evasión de pilares mediante la cámara Sentry Vision 2:

- **Pilar rojo (label 13):** El robot debe pasarlo por la derecha → giro a la derecha por 300ms
- **Pilar verde (label 14):** El robot debe pasarlo por la izquierda → giro a la izquierda por 300ms

La detección de pilares se ejecuta en cada ciclo del loop, con prioridad sobre la lógica de esquinas, garantizando una reacción inmediata al detectar un obstáculo.

---

## Librerías Externas

| Librería | Propósito | Fuente |
|----------|-----------|--------|
| `Servo.h` | Control del servo SG90 | Built-in Arduino IDE |
| `Wire.h` | Comunicación I2C con Sentry2 | Built-in Arduino IDE |
| `Sentry.h` | Detección de blobs con Sentry Vision 2 | Tosee Intelligence — Library Manager |

---

## Proceso de Desarrollo

Nuestro proceso de desarrollo siguió una metodología iterativa con las siguientes etapas:

1. **Prototipo de diagnóstico:** Código para verificar cada sensor individualmente — ultrasonicos, servo, sensor de color y cámara
2. **Calibración de colores:** Script dedicado para obtener valores reales de R, G, B del TCS3200 sobre el tapete de competencia
3. **Integración de cámara:** Confirmación de que la Sentry Vision 2 detecta correctamente labels 13 (rojo) y 14 (verde)
4. **Open Challenge v1:** Giro por tiempo fijo → reemplazado por sensor-confirmed turning
5. **Open Challenge v2:** Detección de esquinas por sensor frontal → reemplazado por sensor lateral (más confiable con pilares presentes)
6. **Open Challenge final:** Auto-detección de dirección, cooldown, commit mínimo de giro, parada por color
7. **Obstacle Challenge:** Integración de detección de pilares sobre el código de Open Challenge

---

*Desarrollado por TEAM VOLT — WRO Future Engineers 2026 — Lima, Perú*

