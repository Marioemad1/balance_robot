#include <Wire.h>
#include <MPU6050.h>

// Motor Pins
int ENA = 11;
int IN1 = 10;
int IN2 = 9;
int IN3 = 8;
int IN4 = 7;
int ENB = 12;

// MPU and balance variables
MPU6050 mpu;
float angle = 0.0;
float angle_gyro = 0.0;
float balance = 0.0; // this is your target balance angle (could be slightly adjusted)
unsigned long previousTime = 0;

// PID terms (tune these)
float Kp = 20;   // proportional gain
float Ki = 0.5;  // integral gain
float Kd = 1.0;  // derivative gain

float error = 0;
float previousError = 0;
float integral = 0;
float derivative = 0;

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("connected");
  } else {
    Serial.println("not_connected");
  }
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float ax_g = ax / 16384.0;     // Acceleration in g
  float az_g = az / 16384.0;
  float gx_dps = gx / 131.0;     // Gyro in degrees/sec

  float angle_acc = atan2(ax_g, az_g) * 180.0 / PI;

  unsigned long currentTime = millis();
  float dt = (currentTime - previousTime) / 1000.0; // dt in seconds
  previousTime = currentTime;

  angle = 0.98 * (angle + gx_dps * dt) + 0.02 * angle_acc;  // complementary filter

  // ---------------- PID Controller ------------------
  error = angle - balance;
  integral += error * dt;
  derivative = (error - previousError) / dt;
  previousError = error;

  float pidOutput = Kp * error + Ki * integral + Kd * derivative;

  // ---------------- Speed Control -------------------
  // Adding gx boost to help in fast falling
  int gxBoost = map(abs(gx_dps), 0, 200, 0, 80); // optional boost
  int speed = abs(pidOutput) + gxBoost;
  speed = constrain(speed, 100, 255);  // avoid weak motor effort

  Serial.print("Angle: ");
  Serial.print(angle);
  Serial.print(" | Error: ");
  Serial.print(error);
  Serial.print(" | Speed: ");
  Serial.println(speed);

  // ------------- Motor Direction Logic ----------------
  if (abs(error) < 1.0) {
    // balanced
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
  else if (error > 0) {
    // falling forward → move backward
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  else {
    // falling backward → move forward
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
}

