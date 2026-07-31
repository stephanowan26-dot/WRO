// ═══════════════════════════════════════════════════════
//  WRO 2026 - OPEN CHALLENGE - 3 VUELTAS
//  Detección automática de dirección
//  CCW = solo sensor izquierdo | CW = solo sensor derecho
//  Cooldown de 650ms después de cada esquina
//  Para 250ms después de la línea naranja número 12
// ═══════════════════════════════════════════════════════

#include <Servo.h>

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

// ── Ultrasonicos ─────────────────────────────────────────
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

// Valores calibrados:
// BLANCO:  R~21 G~23 B~7
// NARANJA: R~24 G~51 B~13 → G > 35 Y R < 40
#define ORANGE_G_MIN  35
#define ORANGE_R_MAX  40

// ── Configuración ────────────────────────────────────────
#define CORNER_DIST      120  // cm - pared lejos = esquina detectada
#define MIN_TURN_TIME    900  // ms - tiempo mínimo de giro
#define CORNER_COOLDOWN  650  // ms - cooldown después de cada esquina
#define STOP_DELAY       250  // ms - rueda después de la línea 12 y para

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
//  Leer distancia ultrasónico (cm)
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
//  Retorna: 0 = nada, 1 = naranja
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
//  Después de la 12va → espera 250ms → para
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
  Serial.println("ESQUINA DETECTADA - GIRANDO!");
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
//  Setup
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

  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  Serial.println("WRO 2026 - Open Challenge");
  Serial.println("Direccion se detecta automaticamente en la primera esquina.");
  Serial.println("Arrancando en 5 segundos...");
  delay(5000);
  Serial.println("ARRANCA!");
}

// ─────────────────────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────────────────────
void loop() {

  if (finished) {
    stopMotor();
    steeringServo.write(SERVO_CENTER);
    return;
  }

  countLines();

  float distL = readDistance(TRIG_L, ECHO_L);
  float distR = readDistance(TRIG_R, ECHO_R);

  Serial.print("I:"); Serial.print(distL);
  Serial.print(" D:"); Serial.println(distR);

  bool cooldownOk = (millis() - lastCornerTime > CORNER_COOLDOWN);

  if (direction == UNKNOWN) {
    if (cooldownOk && distL > CORNER_DIST) {
      direction = CCW;
      Serial.println("DIRECCION: CCW - solo sensor izquierdo");
      doTurn(SERVO_LEFT, TRIG_L, ECHO_L);
    }
    else if (cooldownOk && distR > CORNER_DIST) {
      direction = CW;
      Serial.println("DIRECCION: CW - solo sensor derecho");
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

  steeringServo.write(SERVO_CENTER);
  driveForward();
}
