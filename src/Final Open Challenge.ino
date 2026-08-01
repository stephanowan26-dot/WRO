// ═══════════════════════════════════════════════════════
//  WRO 2026 - DESAFÍO OPEN - FINAL
//  Detección automática de dirección - la primera esquina fija la dirección
//  CCW = detección de hueco con sensor IZQUIERDO
//  CW  = detección de hueco con sensor DERECHO
//  Sin conteo de vueltas - corre indefinidamente
//
//  ARREGLO DE INTERFERENCIA: una vez fijada la dirección, el
//  ultrasónico opuesto nunca se vuelve a activar. Dos sensores
//  pitando seguido dejan que uno capte el eco del otro, dando
//  una lectura falsa de ~57cm en el hueco que bloqueaba el giro.
// ═══════════════════════════════════════════════════════

#include <Servo.h>

// ── Servo ────────────────────────────────────────────────
Servo steeringServo;
#define SERVO_PIN    9
#define SERVO_CENTER 93
#define SERVO_LEFT   46
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

// ── Configuración ────────────────────────────────────────
#define CORNER_DIST        92  // cm - pared lejos = esquina detectada
#define MIN_TURN_TIME_CCW  875  // ms - giro izquierda CCW
#define MIN_TURN_TIME_CW   875  // ms - giro derecha CW
#define CORNER_COOLDOWN    1500  // ms - espera después de cada esquina
#define PING_GAP            60  // ms - separación mínima entre dos pings
                                // Solo se usa antes de saber la dirección,
                                // cuando aún hay que leer ambos sensores.
#define TURN_COUNT_COOLDOWN     500  // ms - espacio mínimo entre el conteo de dos giros
#define TOTAL_TURNS_TO_STOP      12  // número de giros (esquinas) antes de parar
#define STOP_DELAY_AFTER_LAST_TURN_MS 900  // ms - esperar esto después del
                                             // último giro antes de la parada total
                                             // cambia este número para ajustarlo

// ── Estado de dirección ──────────────────────────────────
enum Direction { UNKNOWN, CCW, CW };
Direction direction = UNKNOWN;

unsigned long lastCornerTime = 0;
unsigned long lastTurnCountTime = 0;
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
//  Ejecutar giro en esquina - por tiempo puro, SIN bucle while
// ─────────────────────────────────────────────────────────
void doTurn(int servoAngle, int turnTime) {
  Serial.println("ESQUINA DETECTADA - GIRANDO!");
  steeringServo.write(servoAngle);
  driveForward();
  delay(turnTime);
  steeringServo.write(SERVO_CENTER);
  lastCornerTime = millis();

  // ── Contador de giros/vueltas ─────────────────────────
  if (millis() - lastTurnCountTime > TURN_COUNT_COOLDOWN) {
    turnCount++;
    lastTurnCountTime = millis();
    Serial.print("CONTEO DE GIROS: ");
    Serial.println(turnCount);

    if (turnCount >= TOTAL_TURNS_TO_STOP) {
      delay(STOP_DELAY_AFTER_LAST_TURN_MS);
      stopMotor();
      steeringServo.write(SERVO_CENTER);
      deadStopped = true;
      Serial.println("PARADA TOTAL - se alcanzó el conteo de giros");
    }
  }

  delay(300);
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

  Serial.println("WRO 2026 - Final Open Challenge");
  Serial.println("Arrancando en 5 segundos...");
  delay(5000);
  Serial.println("ARRANCA!");
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

  // ── Lectura de sensores - nunca dos pings seguidos ───
  if (direction == UNKNOWN) {
    // Dirección aún no fijada: se necesitan ambos sensores, pero
    // espaciados para que el primer ping muera antes de que dispare el segundo.
    distL = readDistance(TRIG_L, ECHO_L);
    delay(PING_GAP);
    distR = readDistance(TRIG_R, ECHO_R);

    Serial.print("I:"); Serial.print(distL);
    Serial.print(" D:"); Serial.println(distR);
  }
  else if (direction == CCW) {
    // Solo se activa el sensor IZQUIERDO. El DERECHO está apagado.
    distL = readDistance(TRIG_L, ECHO_L);

    Serial.print("I:"); Serial.print(distL);
    Serial.println(" D:OFF");
  }
  else {  // CW
    // Solo se activa el sensor DERECHO. El IZQUIERDO está apagado.
    distR = readDistance(TRIG_R, ECHO_R);

    Serial.print("I:OFF");
    Serial.print(" D:"); Serial.println(distR);
  }

  bool cooldownOk = (millis() - lastCornerTime > CORNER_COOLDOWN);

  if (direction == UNKNOWN) {
    if (cooldownOk && distL > CORNER_DIST) {
      direction = CCW;
      Serial.println("DIRECCION: CCW");
      doTurn(SERVO_LEFT, MIN_TURN_TIME_CCW);
    }
    else if (cooldownOk && distR > CORNER_DIST && distR < 400) {
      direction = CW;
      Serial.println("DIRECCION: CW");
      doTurn(SERVO_RIGHT, MIN_TURN_TIME_CW);
    }
  }
  else if (direction == CCW) {
    if (cooldownOk && distL > CORNER_DIST ) {
      doTurn(SERVO_LEFT, MIN_TURN_TIME_CCW);
    }
  }
  else if (direction == CW) {
    if (cooldownOk && distR > CORNER_DIST ) {
      doTurn(SERVO_RIGHT, MIN_TURN_TIME_CW);
    }
  }

  if (!deadStopped) {
    steeringServo.write(SERVO_CENTER);
    driveForward();
  }
}
