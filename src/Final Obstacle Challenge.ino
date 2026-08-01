// ═══════════════════════════════════════════════════════
//  WRO 2026 - DESAFÍO DE OBSTÁCULOS - FINAL
//  1. Arranque: detectar dirección (pared pegada < 10cm)
//  2. Ejecutar maniobra de salida del estacionamiento
//  3. Correr el desafío de obstáculos con detección de pilar a media esquina
//     El robot PAUSA a media vuelta para revisar la cámara
//     ROJO  → deriva a la derecha (>90) → corrección barrido IZQUIERDA
//     VERDE → deriva a la izquierda (<90) → corrección barrido DERECHA
// ═══════════════════════════════════════════════════════

#include <Servo.h>
#include <Sentry.h>
#include <Wire.h>

typedef Sentry2 Sentry;
Sentry sentry;

// ── Servo ────────────────────────────────────────────────
Servo steeringServo;
#define SERVO_PIN          9
#define SERVO_CENTER       94
#define SERVO_LEFT         46
#define SERVO_RIGHT        140
#define SERVO_SLIGHT_LEFT  60
#define SERVO_SLIGHT_RIGHT 120

// ── Ángulos de pilar a media esquina ─────────────────────
#define CORNER_MID_RIGHT   115
#define CORNER_MID_LEFT     70

// ── Ángulos y tiempos de corrección ──────────────────────
#define CORNER_CORRECT_ANGLE_LEFT    50
#define CORNER_CORRECT_ANGLE_RIGHT  130
#define CORNER_CORRECT_TIME_LEFT    400
#define CORNER_CORRECT_TIME_RIGHT   400

// ── Motor ────────────────────────────────────────────────
#define ENA  12
#define IN1  11
#define IN2  10
#define SPEED_STRAIGHT  110
#define SPEED_CURVE     150
#define SPEED_TWEAK     130
#define SPEED_EXIT      180
#define SPEED_PHASE3    150

// ── Ultrasónicos ─────────────────────────────────────────
#define TRIG_L  24
#define ECHO_L  22
#define TRIG_R  50
#define ECHO_R  52

// ── Configuración: Esquina ───────────────────────────────
#define CORNER_DIST         92
#define MIN_TURN_TIME_CCW  1800
#define MIN_TURN_TIME_CW   1800
#define HALF_TURN_TIME      1000   // ms - primera mitad del giro
#define SECOND_HALF_TIME    600   // ms - segunda mitad del giro (ajustable)
#define MID_PAUSE_TIME     600   // ms - pausa a media vuelta para revisar cámara
#define CORNER_COOLDOWN    1500
#define EXIT_COOLDOWN       800
#define PING_GAP            60

// ── Contador de giros - parar después de TOTAL_TURNS_TO_STOP esquinas ──
#define TOTAL_TURNS_TO_STOP      12  // número de esquinas antes de parar
#define STOP_DELAY_AFTER_LAST_TURN_MS 500  // ms - esperar esto después de
                                             // la última esquina antes de la parada total

// ── Detección de dirección al arrancar ───────────────────
#define WALL_STUCK_DIST    10

// ── Salida de estacionamiento CW ─────────────────────────
#define CW_REVERSE_LEFT_TIME    450
#define CW_FWD_RIGHT_TIME      1100
#define CW_FWD_LEFT_TIME       1500

// ── Salida de estacionamiento CCW ────────────────────────
#define CCW_REVERSE_RIGHT_TIME  450
#define CCW_FWD_LEFT_TIME      1100
#define CCW_FWD_RIGHT_TIME     1500

#define PRE_TURN_TIME    300

// ── Ajuste pilar rojo (línea recta) ──────────────────────
#define RED_TWEAK_RIGHT_TIME    1000
#define RED_TWEAK_LEFT_TIME     800

// ── Ajuste pilar verde (línea recta) ─────────────────────
#define GREEN_TWEAK_LEFT_TIME   1000
#define GREEN_TWEAK_RIGHT_TIME  800

#define PILLAR_COOLDOWN         2000

// ── Etiquetas de blob ──────────────────────────────────────
#define LABEL_RED    13
#define LABEL_GREEN  14

// ── Estado de dirección ──────────────────────────────────
enum Direction { UNKNOWN, CCW, CW };
Direction direction = UNKNOWN;

unsigned long lastCornerTime = 0;
unsigned long lastPillarTime = 0;
int turnCount = 0;
bool deadStopped = false;

// ─────────────────────────────────────────────────────────
//  Leer distancia ultrasónica (cm)
// ─────────────────────────────────────────────────────────
float readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 999;
  float dist = duration * 0.034 / 2.0;
  if (dist > 400) return 999;
  return dist;
}

// ─────────────────────────────────────────────────────────
//  Motor
// ─────────────────────────────────────────────────────────
void driveForward(int speed) {
  analogWrite(ENA, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void driveBackward(int speed) {
  analogWrite(ENA, speed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void stopMotor() {
  digitalWrite(ENA, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

// ─────────────────────────────────────────────────────────
//  Revisar pilares - retorna 0=ninguno 1=rojo 2=verde
// ─────────────────────────────────────────────────────────
int checkPillars() {
  int detected = sentry.GetValue(Sentry::kVisionBlob, kStatus);
  if (detected > 0) {
    int label = sentry.GetValue(Sentry::kVisionBlob, kLabel, 1);
    int x     = sentry.GetValue(Sentry::kVisionBlob, kXValue, 1);
    if (label == LABEL_RED) {
      Serial.print("ROJO X:"); Serial.println(x);
      return 1;
    }
    else if (label == LABEL_GREEN) {
      Serial.print("VERDE X:"); Serial.println(x);
      return 2;
    }
  }
  return 0;
}

// ─────────────────────────────────────────────────────────
//  Giro en esquina con revisión a media vuelta (PAUSA en el punto medio)
// ─────────────────────────────────────────────────────────
void doTurn(int servoAngle, int turnTime) {
  Serial.println("ESQUINA DETECTADA - GIRANDO!");

  // Fase A: primera mitad al ángulo normal
  steeringServo.write(servoAngle);
  driveForward(SPEED_CURVE);
  delay(HALF_TURN_TIME);

  // PAUSA para revisar la cámara claramente
  Serial.println("Pausando para revisar pilar...");
  stopMotor();
  delay(MID_PAUSE_TIME);

  // Fase B: revisar cámara
  int pillar = checkPillars();

  if (pillar == 1) {
    // ROJO → deriva a la derecha por SECOND_HALF_TIME → corrige IZQUIERDA
    Serial.println("MEDIA-ESQUINA ROJO → deriva derecha + corrige IZQUIERDA");
    steeringServo.write(CORNER_MID_RIGHT);
    driveForward(SPEED_CURVE);
    delay(SECOND_HALF_TIME);

    steeringServo.write(CORNER_CORRECT_ANGLE_LEFT);
    driveForward(SPEED_TWEAK);
    delay(CORNER_CORRECT_TIME_LEFT);
  }
  else if (pillar == 2) {
    // VERDE → deriva a la izquierda por SECOND_HALF_TIME → corrige DERECHA
    Serial.println("MEDIA-ESQUINA VERDE → deriva izquierda + corrige DERECHA");
    steeringServo.write(CORNER_MID_LEFT);
    driveForward(SPEED_CURVE);
    delay(SECOND_HALF_TIME);

    steeringServo.write(CORNER_CORRECT_ANGLE_RIGHT);
    driveForward(SPEED_TWEAK);
    delay(CORNER_CORRECT_TIME_RIGHT);
  }
  else {
    // Sin pilar → completa el giro al ángulo original por SECOND_HALF_TIME
    steeringServo.write(servoAngle);
    driveForward(SPEED_CURVE);
    delay(SECOND_HALF_TIME);
  }

  steeringServo.write(SERVO_CENTER);
  lastCornerTime = millis();

  // ── Contador de giros ─────────────────────────────────
  turnCount++;
  Serial.print("CONTEO DE GIROS: ");
  Serial.println(turnCount);

  if (turnCount >= TOTAL_TURNS_TO_STOP) {
    delay(STOP_DELAY_AFTER_LAST_TURN_MS);
    stopMotor();
    steeringServo.write(SERVO_CENTER);
    deadStopped = true;
    Serial.println("PARADA TOTAL - se alcanzó el conteo de giros");
  }

  delay(300);
}

// ─────────────────────────────────────────────────────────
//  Ajuste pilar ROJO (detección en línea recta)
// ─────────────────────────────────────────────────────────
void doRedTweak() {
  Serial.println("=== AJUSTE ROJO ===");
  steeringServo.write(SERVO_SLIGHT_RIGHT);
  driveForward(SPEED_TWEAK);
  delay(RED_TWEAK_RIGHT_TIME);
  steeringServo.write(SERVO_SLIGHT_LEFT);
  driveForward(SPEED_TWEAK);
  delay(RED_TWEAK_LEFT_TIME);
  steeringServo.write(SERVO_CENTER);
  lastPillarTime = millis();
}

// ─────────────────────────────────────────────────────────
//  Ajuste pilar VERDE (detección en línea recta)
// ─────────────────────────────────────────────────────────
void doGreenTweak() {
  Serial.println("=== AJUSTE VERDE ===");
  steeringServo.write(SERVO_SLIGHT_LEFT);
  driveForward(SPEED_TWEAK);
  delay(GREEN_TWEAK_LEFT_TIME);
  steeringServo.write(SERVO_SLIGHT_RIGHT);
  driveForward(SPEED_TWEAK);
  delay(GREEN_TWEAK_RIGHT_TIME);
  steeringServo.write(SERVO_CENTER);
  lastPillarTime = millis();
}

// ─────────────────────────────────────────────────────────
//  Salida de estacionamiento CW
// ─────────────────────────────────────────────────────────
void parkingExitCW() {
  Serial.println("=== SALIDA DE ESTACIONAMIENTO CW ===");

  Serial.println("Fase 1: reversa IZQUIERDA");
  steeringServo.write(SERVO_LEFT);
  delay(PRE_TURN_TIME);
  driveBackward(SPEED_EXIT);
  delay(CW_REVERSE_LEFT_TIME);
  stopMotor();

  Serial.println("Fase 2: adelante DERECHA");
  steeringServo.write(SERVO_RIGHT);
  delay(PRE_TURN_TIME);
  driveForward(SPEED_EXIT);
  delay(CW_FWD_RIGHT_TIME);
  stopMotor();

  Serial.println("Fase 3: adelante IZQUIERDA (LENTO)");
  steeringServo.write(SERVO_LEFT);
  delay(PRE_TURN_TIME);
  driveForward(SPEED_PHASE3);
  delay(CW_FWD_LEFT_TIME);
  stopMotor();

  Serial.println("=== SALIDA COMPLETA ===");
}

// ─────────────────────────────────────────────────────────
//  Salida de estacionamiento CCW
// ─────────────────────────────────────────────────────────
void parkingExitCCW() {
  Serial.println("=== SALIDA DE ESTACIONAMIENTO CCW ===");

  Serial.println("Fase 1: reversa DERECHA");
  steeringServo.write(SERVO_RIGHT);
  delay(PRE_TURN_TIME);
  driveBackward(SPEED_EXIT);
  delay(CCW_REVERSE_RIGHT_TIME);
  stopMotor();

  Serial.println("Fase 2: adelante IZQUIERDA");
  steeringServo.write(SERVO_LEFT);
  delay(PRE_TURN_TIME);
  driveForward(SPEED_EXIT);
  delay(CCW_FWD_LEFT_TIME);
  stopMotor();

  Serial.println("Fase 3: adelante DERECHA (LENTO)");
  steeringServo.write(SERVO_RIGHT);
  delay(PRE_TURN_TIME);
  driveForward(SPEED_PHASE3);
  delay(CCW_FWD_RIGHT_TIME);
  stopMotor();

  Serial.println("=== SALIDA COMPLETA ===");
}

// ─────────────────────────────────────────────────────────
//  Detectar dirección según qué pared está pegada
// ─────────────────────────────────────────────────────────
Direction detectStartupDirection() {
  Serial.println("Detectando dirección de arranque...");
  int leftStuckCount  = 0;
  int rightStuckCount = 0;

  for (int i = 0; i < 5; i++) {
    float distL = readDistance(TRIG_L, ECHO_L);
    delay(PING_GAP);
    float distR = readDistance(TRIG_R, ECHO_R);

    Serial.print("I:"); Serial.print(distL);
    Serial.print(" D:"); Serial.println(distR);

    if (distL < WALL_STUCK_DIST) leftStuckCount++;
    if (distR < WALL_STUCK_DIST) rightStuckCount++;
    delay(500);
  }

  if (leftStuckCount > rightStuckCount) {
    Serial.println(">>> Pared izquierda pegada → CW");
    return CW;
  } else {
    Serial.println(">>> Pared derecha pegada → CCW");
    return CCW;
  }
}

// ─────────────────────────────────────────────────────────
//  Configuración
// ─────────────────────────────────────────────────────────
void setup() {
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);
  delay(1000);

  Serial.begin(9600);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  stopMotor();

  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  Wire.begin();
  Serial.println("Conectando Sentry2...");
  while (SENTRY_OK != sentry.begin(&Wire)) {
    Serial.println("Esperando Sentry2...");
    delay(500);
  }
  sentry.VisionBegin(Sentry::kVisionBlob);
  Serial.println("Sentry2 listo!");

  Serial.println("WRO 2026 - Final Obstacle Challenge");

  direction = detectStartupDirection();

  if (direction == CW) {
    parkingExitCW();
  } else {
    parkingExitCCW();
  }

  lastCornerTime = millis() - CORNER_COOLDOWN + EXIT_COOLDOWN;
  Serial.println("ARRANCA! Desafío de obstáculos ON...");
}

// ─────────────────────────────────────────────────────────
//  Bucle principal
// ─────────────────────────────────────────────────────────
void loop() {

  // ── Parada total - detiene para siempre después de TOTAL_TURNS_TO_STOP ──
  if (deadStopped) {
    return;  // ya está parado, no hace nada más
  }

  float distL = 999;
  float distR = 999;

  if (direction == CCW) {
    distL = readDistance(TRIG_L, ECHO_L);
    Serial.print("I:"); Serial.print(distL);
    Serial.println(" D:OFF");
  } else {
    distR = readDistance(TRIG_R, ECHO_R);
    Serial.print("I:OFF");
    Serial.print(" D:"); Serial.println(distR);
  }

  bool cornerCooldownOk = (millis() - lastCornerTime > CORNER_COOLDOWN);
  bool pillarCooldownOk = (millis() - lastPillarTime > PILLAR_COOLDOWN);

  if (direction == CCW) {
    if (cornerCooldownOk && distL > CORNER_DIST) {
      doTurn(SERVO_LEFT, MIN_TURN_TIME_CCW);
      return;
    }
  }
  else if (direction == CW) {
    if (cornerCooldownOk && distR > CORNER_DIST) {
      doTurn(SERVO_RIGHT, MIN_TURN_TIME_CW);
      return;
    }
  }

  if (pillarCooldownOk) {
    int pillar = checkPillars();
    if (pillar == 1) {
      doRedTweak();
      return;
    }
    else if (pillar == 2) {
      doGreenTweak();
      return;
    }
  }

  steeringServo.write(SERVO_CENTER);
  driveForward(SPEED_STRAIGHT);
}
