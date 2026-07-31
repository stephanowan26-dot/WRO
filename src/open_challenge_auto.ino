// ═══════════════════════════════════════════════════════
//  WRO 2026 - OPEN CHALLENGE - 3 LAPS
//  Auto direction detection - first corner sets direction
//  CCW = LEFT sensor only | CW = RIGHT sensor only
//  650ms cooldown after each corner
//  Stops 250ms after 12th orange line
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

// ── Ultrasonics ──────────────────────────────────────────
#define TRIG_L  24
#define ECHO_L  22
#define TRIG_R  50
#define ECHO_R  52

// ── Color sensor ─────────────────────────────────────────
#define S0   34
#define S1   36
#define S2   38
#define S3   40
#define OUT  42

// Calibrated thresholds:
// WHITE:  R~21 G~23 B~7
// ORANGE: R~24 G~51 B~13 → G > 35 AND R < 40
#define ORANGE_G_MIN  35
#define ORANGE_R_MAX  40

// ── Config ───────────────────────────────────────────────
#define CORNER_DIST      120  // cm - wall far = corner detected
#define MIN_TURN_TIME    900  // ms - minimum turn commit time
#define CORNER_COOLDOWN  650  // ms - cooldown after each corner
#define STOP_DELAY       250  // ms - roll after 12th orange then stop

// ── Direction state ──────────────────────────────────────
enum Direction { UNKNOWN, CCW, CW };
Direction direction = UNKNOWN;

// ── Lap counting ─────────────────────────────────────────
#define TOTAL_ORANGE  12
int orangeCount    = 0;
bool lastWasOrange = false;
unsigned long lastLineTime   = 0;
unsigned long lastCornerTime = 0;
#define LINE_COOLDOWN 1500

bool finished = false;

// ─────────────────────────────────────────────────────────
//  Read ultrasonic distance (cm)
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
//  Read one color sensor channel
// ─────────────────────────────────────────────────────────
int readChannel(int s2val, int s3val) {
  digitalWrite(S2, s2val);
  digitalWrite(S3, s3val);
  delay(10);
  return pulseIn(OUT, LOW, 100000);
}

// ─────────────────────────────────────────────────────────
//  Detect orange line
//  Returns: 0 = nothing, 1 = orange
// ─────────────────────────────────────────────────────────
int detectLine() {
  int r = readChannel(LOW, LOW);
  int g = readChannel(HIGH, HIGH);
  if (g > ORANGE_G_MIN && r < ORANGE_R_MAX) return 1;
  return 0;
}

// ─────────────────────────────────────────────────────────
//  Motor control
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
//  Count orange line crossings
//  After 12th orange → wait 250ms → stop
// ─────────────────────────────────────────────────────────
void countLines() {
  unsigned long now = millis();
  if (now - lastLineTime > LINE_COOLDOWN) {
    int line = detectLine();
    if (line == 1 && !lastWasOrange) {
      orangeCount++;
      lastWasOrange = true;
      lastLineTime  = now;
      Serial.print("ORANGE #");
      Serial.print(orangeCount);
      Serial.print(" / ");
      Serial.println(TOTAL_ORANGE);

      // 12th orange line = 3 laps done
      if (orangeCount >= TOTAL_ORANGE) {
        Serial.println("12 ORANGE LINES - stopping in 250ms!");
        delay(STOP_DELAY);
        finished = true;
        stopMotor();
        steeringServo.write(SERVO_CENTER);
        Serial.println("3 LAPS COMPLETE - STOPPED!");
      }
    }
    else if (line == 0) {
      lastWasOrange = false;
    }
  }
}

// ─────────────────────────────────────────────────────────
//  Execute a corner turn
//  Commits to MIN_TURN_TIME then waits for wall to return
// ─────────────────────────────────────────────────────────
void doTurn(int servoAngle, int checkTrig, int checkEcho) {
  Serial.println("CORNER DETECTED - TURNING!");
  steeringServo.write(servoAngle);
  driveForward();
  delay(MIN_TURN_TIME);

  // Keep turning until wall comes back into range
  while (readDistance(checkTrig, checkEcho) > CORNER_DIST) {
    steeringServo.write(servoAngle);
    driveForward();
    Serial.println("Still turning...");
  }

  Serial.println("Corner done - going straight!");
  steeringServo.write(SERVO_CENTER);
  lastCornerTime = millis();
  delay(300);
}

// ─────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────
void setup() {
  // Servo must be attached and centered FIRST
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);
  delay(1000);

  Serial.begin(9600);

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  stopMotor();

  // Ultrasonic pins
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  // Color sensor pins - 20% frequency scaling
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  Serial.println("WRO 2026 - Open Challenge");
  Serial.println("Direction auto-detected on first corner.");
  Serial.println("Starting in 5 seconds...");
  delay(5000);
  Serial.println("GO!");
}

// ─────────────────────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────────────────────
void loop() {

  // Robot stopped - hold position
  if (finished) {
    stopMotor();
    steeringServo.write(SERVO_CENTER);
    return;
  }

  // Count orange line crossings
  countLines();

  // Read both wall sensors
  float distL = readDistance(TRIG_L, ECHO_L);
  float distR = readDistance(TRIG_R, ECHO_R);

  Serial.print("L:"); Serial.print(distL);
  Serial.print(" R:"); Serial.println(distR);

  // Check if cooldown period has passed since last corner
  bool cooldownOk = (millis() - lastCornerTime > CORNER_COOLDOWN);

  // Direction unknown - check both sensors for first corner
  if (direction == UNKNOWN) {
    if (cooldownOk && distL > CORNER_DIST) {
      direction = CCW;
      Serial.println("DIRECTION SET: CCW - Left sensor active only");
      doTurn(SERVO_LEFT, TRIG_L, ECHO_L);
    }
    else if (cooldownOk && distR > CORNER_DIST) {
      direction = CW;
      Serial.println("DIRECTION SET: CW - Right sensor active only");
      doTurn(SERVO_RIGHT, TRIG_R, ECHO_R);
    }
  }

  // CCW confirmed - only left sensor triggers corners
  else if (direction == CCW) {
    if (cooldownOk && distL > CORNER_DIST) {
      doTurn(SERVO_LEFT, TRIG_L, ECHO_L);
    }
  }

  // CW confirmed - only right sensor triggers corners
  else if (direction == CW) {
    if (cooldownOk && distR > CORNER_DIST) {
      doTurn(SERVO_RIGHT, TRIG_R, ECHO_R);
    }
  }

  // Go straight
  steeringServo.write(SERVO_CENTER);
  driveForward();
}
