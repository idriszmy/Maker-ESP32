#include <Bluepad32.h>
#include <CytronMotorDriver.h>

// Maker ESP32 + Robo ESP32: MOTOR1 right, MOTOR2 left.
constexpr uint8_t RIGHT_A = 12;
constexpr uint8_t RIGHT_B = 13;
constexpr uint8_t LEFT_A = 14;
constexpr uint8_t LEFT_B = 27;
constexpr int MAX_PWM = 255;        // Full PWM range; analog input remains proportional.
constexpr int DPAD_PWM = 128;       // About 50%; hold R2 for MAX_PWM.
constexpr int AXIS_LIMIT = 512;
constexpr int AXIS_DEADBAND = 40;
constexpr uint32_t INPUT_TIMEOUT_MS = 300;
constexpr uint32_t NEUTRAL_HOLD_MS = 300;
constexpr bool INVERT_RIGHT = false;
constexpr bool INVERT_LEFT = false;

static_assert(MAX_PWM > 0 && MAX_PWM <= 255, "MAX_PWM must be 1..255");
static_assert(DPAD_PWM > 0 && DPAD_PWM <= MAX_PWM, "DPAD_PWM must be 1..MAX_PWM");
static_assert(AXIS_DEADBAND >= 0 && AXIS_DEADBAND < AXIS_LIMIT, "Invalid deadband");

CytronMD motorRight(PWM_PWM, RIGHT_A, RIGHT_B);
CytronMD motorLeft(PWM_PWM, LEFT_A, LEFT_B);
ControllerPtr gamepad = nullptr;

bool armed = false;
bool neutralPending = false;
uint32_t neutralSinceMs = 0;
uint32_t lastInputMs = 0;

void stopMotors() {
  motorRight.setSpeed(0);
  motorLeft.setSpeed(0);
}

void disarm() {
  stopMotors();
  armed = false;
  neutralPending = false;
}

void drive(int left, int right) {
  left = constrain(left, -MAX_PWM, MAX_PWM);
  right = constrain(right, -MAX_PWM, MAX_PWM);
  motorLeft.setSpeed(INVERT_LEFT ? -left : left);
  motorRight.setSpeed(INVERT_RIGHT ? -right : right);
}

int axisToPwm(int axis) {
  axis = constrain(axis, -AXIS_LIMIT, AXIS_LIMIT);
  const int magnitude = abs(axis);
  if (magnitude <= AXIS_DEADBAND) return 0;

  // Rescale after the deadband so speed starts smoothly from zero.
  const int pwm = (magnitude - AXIS_DEADBAND) * MAX_PWM /
                  (AXIS_LIMIT - AXIS_DEADBAND);
  return axis < 0 ? -pwm : pwm;
}

bool controlsNeutral(ControllerPtr ctl) {
  return ctl->dpad() == 0 &&
         abs(ctl->axisY()) <= AXIS_DEADBAND &&
         abs(ctl->axisRX()) <= AXIS_DEADBAND;
}

void driveDpad(uint8_t dpad, int speed) {
  // Diagonals describe the direction of travel, including when reversing.
  switch (dpad) {
    case DPAD_UP:                   drive(speed, speed); break;
    case DPAD_DOWN:                 drive(-speed, -speed); break;
    case DPAD_LEFT:                 drive(-speed, speed); break;
    case DPAD_RIGHT:                drive(speed, -speed); break;
    case DPAD_UP | DPAD_LEFT:       drive(0, speed); break;
    case DPAD_UP | DPAD_RIGHT:      drive(speed, 0); break;
    case DPAD_DOWN | DPAD_LEFT:     drive(0, -speed); break;
    case DPAD_DOWN | DPAD_RIGHT:    drive(-speed, 0); break;
    default: stopMotors(); break;  // Reject conflicting or unknown directions.
  }
}

void driveAnalog(ControllerPtr ctl) {
  const int throttle = axisToPwm(-ctl->axisY());
  const int steering = axisToPwm(ctl->axisRX());
  int left = throttle + steering;
  int right = throttle - steering;
  const int peak = max(abs(left), abs(right));

  // Preserve the turn ratio when both inputs would exceed the speed limit.
  if (peak > MAX_PWM) {
    left = left * MAX_PWM / peak;
    right = right * MAX_PWM / peak;
  }
  drive(left, right);
}

void processInput(ControllerPtr ctl, uint32_t now) {
  // PS4 Cross is Bluepad32's A button. Stop overrides every drive input.
  if (ctl->a()) {
    disarm();
    return;
  }

  if (!armed) {
    stopMotors();
    if (!controlsNeutral(ctl)) {
      neutralPending = false;
      return;
    }
    if (!neutralPending) {
      neutralPending = true;
      neutralSinceMs = now;
    }
    if (uint32_t(now - neutralSinceMs) >= NEUTRAL_HOLD_MS) {
      armed = true;
      neutralPending = false;
      Serial.println("Ready: D-pad or left Y + right X. Cross = stop.");
    }
    return;
  }

  // D-pad takes priority; release it to return to the current stick inputs.
  if (ctl->dpad() != 0) {
    driveDpad(ctl->dpad(), ctl->r2() ? MAX_PWM : DPAD_PWM);
  }
  else driveAnalog(ctl);
}

void onConnectedController(ControllerPtr ctl) {
  // Follow the official Bluepad32 example: retain the first controller in the
  // callback without filtering its class. Some DS4 connections only finish
  // populating their gamepad data after this callback returns.
  if (gamepad != nullptr) return;

  disarm();
  gamepad = ctl;
  lastInputMs = millis();
  Serial.println("Controller connected. Release D-pad and centre both sticks.");
}

void onDisconnectedController(ControllerPtr ctl) {
  if (ctl != gamepad) return;
  disarm();
  gamepad = nullptr;
  Serial.println("Controller disconnected: motors stopped.");
}

void setup() {
  stopMotors();
  Serial.begin(115200);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  // Keep Bluetooth keys across resets; do not forget keys on every boot.
  Serial.println("Pair PS4: hold SHARE + PS until the light bar flashes.");
}

void loop() {
  const bool updated = BP32.update();
  const uint32_t now = millis();

  if (gamepad == nullptr || !gamepad->isConnected() || !gamepad->isGamepad()) {
    disarm();
  } else {
    // Check the gap BEFORE accepting a new report, including after a loop stall.
    if (uint32_t(now - lastInputMs) >= INPUT_TIMEOUT_MS) {
      if (armed) Serial.println("Input timeout: stopped. Return controls to neutral.");
      disarm();
    }
    if (updated && gamepad->hasData()) {
      lastInputMs = now;
      processInput(gamepad, now);
    }
  }

  delay(1);  // Yield to Bluetooth/RTOS tasks without a long control-loop delay.
}
