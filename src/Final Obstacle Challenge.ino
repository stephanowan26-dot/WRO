// ═══════════════════════════════════════════════════════
//  WRO 2026 - DESAFÍO DE OBSTÁCULOS - PRUEBA
//  Rojo (13)  → giro derecha 300ms
//  Verde (14) → giro izquierda 300ms
//  Esquinas con sensor izquierdo o derecho (auto dirección)
//  Para después de 12 líneas naranjas + 250ms
// ═══════════════════════════════════════════════════════

#include <Servo.h>
#include <Sentry.h>
#include <Wire.h>

typedef Sentry2 Sentry;
Sentry sentry;

// ── Servo ────────────────────────────────────────────────
Servo steeringServo;
#define SERVO_PIN    9
#define SERVO_CENTER 90
#define SERVO_LEFT   40
#define SERVO_RIGHT  140

// ── Motor ────────────────────────────────────────────────
#define ENA  12
#define IN1  11
#define IN2  10
#define SPEED_FULL  200

// ── Ultrasónicos ─────────────────────────────────────────
#define TRIG_L  24
#define ECHO_L  22
#define TRIG_R  50
#define ECHO_R  52

// ── Sensor de color ──────────────────────────────────────
#define S0   34
#define S1   36
#define S2   38
#define S3   40
#define OUT  42

#define ORANGE_G_MIN  35
#define ORANGE_R_MAX  40

// ── Configuración ────────────────────────────────────────
#define CORNER_DIST      120  // cm - pared lejos = esquina
#define MIN_TURN_TIME    900  // ms - tiempo mínimo de giro
#define CORNER_COOLDOWN  650  // ms - tiempo de espera después de esquina
#define STOP_DELAY       250  // ms - rueda después de línea 12 y para
#define PILLAR_TURN_TIME 300  // ms - giro al detectar pilar

// ── Estado de dirección ──────────────────────────────────
enum Direction { UNKNOWN, CCW, CW };
Direction direction = UNKNOWN;

// ── Conteo de vueltas ────────────────────────────────────
#define TOTAL_ORANGE  12
int orangeCount    = 0;
bool lastWasOrange = false;
unsigned long lastLineTime   = 0;
unsigned long lastCornerTime = 0;
#define LINE_COOLDOWN 1500

bool finished = false;

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
//  Leer canal del sensor de color
// ─────────────────────────────────────────────────────────
int readChannel(int s2val, int s3val) {
  digitalWrite(S2, s2val);
  digitalWrite(S3, s3val);
  delay(10);
  return pulseIn(OUT, LOW, 100000);
}

// ─────────────────────────────────────────────────────────
//  Detectar línea naranja
// ─────────────────────────────────────────────────────────
int detectLine() {
  int r = readChannel(LOW, LOW);
  int g = readChannel(HIGH, HIGH);
  if (g > ORANGE_G_MIN && r < ORANGE_R_MAX) return 1;
  return 0;
}

// ─────────────────────────────────────────────────────────
//  Control de motor
// ─────────────────────────────────────────────────────────
void driveForward() {
  analogWrite(ENA, SPEED_FULL);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void stopMotor() {
  digitalWrite(ENA, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

// ─────────────────────────────────────────────────────────
//  Contar líneas naranjas
// ─────────────────────────────────────────────────────────
void countLines() {
  unsigned long now = millis();
  if (now - lastLineTime > LINE_COOLDOWN) {
    int line = detectLine();
    if (line == 1 && !lastWasOrange) {
      orangeCount++;
      lastWasOrange = true;
      lastLineTime  = now;
      Serial.print("NARANJA #");
      Serial.print(orangeCount);
      Serial.print(" / ");
      Serial.println(TOTAL_ORANGE);

      if (orangeCount >= TOTAL_ORANGE) {
        Serial.println("12 NARANJAS - parando en 250ms!");
        delay(STOP_DELAY);
        finished = true;
        stopMotor();
        steeringServo.write(SERVO_CENTER);
        Serial.println("3 VUELTAS COMPLETAS - PARADO!");
      }
    }
    else if (line == 0) {
      lastWasOrange = false;
    }
  }
}

// ─────────────────────────────────────────────────────────
//  Ejecutar giro en esquina
// ─────────────────────────────────────────────────────────
void doTurn(int servoAngle, int checkTrig, int checkEcho) {
  Serial.println("ESQUINA - GIRANDO!");
  steeringServo.write(servoAngle);
  driveForward();
  delay(MIN_TURN_TIME);

  while (readDistance(checkTrig, checkEcho) > CORNER_DIST) {
    steeringServo.write(servoAngle);
    driveForward();
    Serial.println("Todavia girando...");
  }

  Serial.println("Esquina lista - recto!");
  steeringServo.write(SERVO_CENTER);
  lastCornerTime = millis();
  delay(300);
}

// ─────────────────────────────────────────────────────────
//  Detectar y reaccionar a pilares
// ─────────────────────────────────────────────────────────
void checkPillars() {
  int detected = sentry.GetValue(Sentry::kVisionBlob, kStatus);
  if (detected > 0) {
    int label = sentry.GetValue(Sentry::kVisionBlob, kLabel, 1);
    int x     = sentry.GetValue(Sentry::kVisionBlob, kXValue, 1);

    if (label == 13) {
      // Rojo → pasar por derecha → giro derecha
      Serial.print("ROJO detectado X:"); Serial.println(x);
      steeringServo.write(SERVO_RIGHT);
      driveForward();
      delay(PILLAR_TURN_TIME);
      steeringServo.write(SERVO_CENTER);
    }
    else if (label == 14) {
      // Verde → pasar por izquierda → giro izquierda
      Serial.print("VERDE detectado X:"); Serial.println(x);
      steeringServo.write(SERVO_LEFT);
      driveForward();
      delay(PILLAR_TURN_TIME);
      steeringServo.write(SERVO_CENTER);
    }
  }
}

// ─────────────────────────────────────────────────────────
//  Configuración
// ─────────────────────────────────────────────────────────
void setup() {
  // Servo primero siempre
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);
  delay(1000);

  Serial.begin(9600);

  // Motor
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  stopMotor();

  // Ultrasónicos
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  // Sensor de color
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Sentry2 mediante I2C
  Wire.begin();
  Serial.println("Conectando Sentry2...");
  while (SENTRY_OK != sentry.begin(&Wire)) {
    Serial.println("Esperando Sentry2...");
    delay(500);
  }
  sentry.VisionBegin(Sentry::kVisionBlob);
  Serial.println("Sentry2 listo!");

  Serial.println("WRO 2026 - Obstacle Challenge");
  Serial.println("Arrancando en 5 segundos...");
  delay(5000);
  Serial.println("ARRANCA!");
}

// ─────────────────────────────────────────────────────────
//  Bucle principal
// ─────────────────────────────────────────────────────────
void loop() {

  if (finished) {
    stopMotor();
    steeringServo.write(SERVO_CENTER);
    return;
  }

  // Contar líneas naranjas
  countLines();

  // Revisar pilares con cámara
  checkPillars();

  // Leer sensores de pared
  float distL = readDistance(TRIG_L, ECHO_L);
  float distR = readDistance(TRIG_R, ECHO_R);

  Serial.print("I:"); Serial.print(distL);
  Serial.print(" D:"); Serial.println(distR);

  bool cooldownOk = (millis() - lastCornerTime > CORNER_COOLDOWN);

  // Dirección desconocida - revisa ambos sensores
  if (direction == UNKNOWN) {
    if (cooldownOk && distL > CORNER_DIST) {
      direction = CCW;
      Serial.println("DIRECCION: CCW");
      doTurn(SERVO_LEFT, TRIG_L, ECHO_L);
    }
    else if (cooldownOk && distR > CORNER_DIST) {
      direction = CW;
      Serial.println("DIRECCION: CW");
      doTurn(SERVO_RIGHT, TRIG_R, ECHO_R);
    }
  }
  else if (direction == CCW) {
    if (cooldownOk && distL > CORNER_DIST) {
      doTurn(SERVO_LEFT, TRIG_L, ECHO_L);
    }
  }
  else if (direction == CW) {
    if (cooldownOk && distR > CORNER_DIST) {
      doTurn(SERVO_RIGHT, TRIG_R, ECHO_R);
    }
  }

  // Ir recto
  steeringServo.write(SERVO_CENTER);
  driveForward();
}
