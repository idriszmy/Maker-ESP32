#include <cassert>
#include <iostream>
#include "../Arduino/MakerESP32_MobileRobot_PS4/MakerESP32_MobileRobot_PS4.ino"

void expectMotors(int left, int right) {
  assert(motorLeft.speed == left);
  assert(motorRight.speed == right);
}

void report(Controller& ctl, uint32_t now) {
  testClock = now;
  ctl.fresh = true;
  BP32.changed = true;
  loop();
}

int main() {
  setup();
  expectMotors(0, 0);

  // Every valid D-pad direction, plus every conflicting bit combination.
  const int expected[16][2] = {
    {0, 0}, {255, 255}, {-255, -255}, {0, 0},
    {255, -255}, {255, 0}, {-255, 0}, {0, 0},
    {-255, 255}, {0, 255}, {0, -255}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}
  };
  for (int bits = 0; bits < 16; ++bits) {
    driveDpad(bits);
    expectMotors(expected[bits][0], expected[bits][1]);
  }
  driveDpad(255);
  expectMotors(0, 0);

  // Deadband, odd symmetry, monotonic output and clipping at axis limits.
  for (int axis = -600; axis <= 600; ++axis) {
    assert(abs(axisToPwm(axis)) <= MAX_PWM);
    assert(axisToPwm(axis) == -axisToPwm(-axis));
    if (abs(axis) <= AXIS_DEADBAND) assert(axisToPwm(axis) == 0);
    if (axis < 600) assert(axisToPwm(axis + 1) >= axisToPwm(axis));
  }

  Controller ctl;
  ctl.leftY = -512;
  driveAnalog(&ctl);
  expectMotors(255, 255);
  ctl.leftY = 512;
  driveAnalog(&ctl);
  expectMotors(-255, -255);
  ctl.leftY = 0;
  ctl.rightX = 512;
  driveAnalog(&ctl);
  expectMotors(255, -255);
  ctl.rightX = -512;
  driveAnalog(&ctl);
  expectMotors(-255, 255);
  ctl.leftY = -512;
  ctl.rightX = 256;
  driveAnalog(&ctl);
  assert(motorLeft.speed == MAX_PWM);
  assert(motorRight.speed > 0 && motorRight.speed < MAX_PWM);
  for (int y = -512; y <= 512; y += 8) {
    for (int x = -512; x <= 512; x += 8) {
      ctl.leftY = y;
      ctl.rightX = x;
      driveAnalog(&ctl);
      assert(abs(motorLeft.speed) <= MAX_PWM);
      assert(abs(motorRight.speed) <= MAX_PWM);
    }
  }

  // Connecting with a held control must never arm or move the robot.
  onConnectedController(&ctl);
  report(ctl, 10);
  report(ctl, 200);
  expectMotors(0, 0);
  assert(!armed);
  ctl.leftY = ctl.rightX = 0;
  report(ctl, 210);
  report(ctl, 400);
  report(ctl, 510);
  assert(armed);
  ctl.leftY = -512;
  ctl.directions = DPAD_LEFT;
  report(ctl, 520);
  expectMotors(-255, 255);  // D-pad overrides the held forward stick.
  ctl.directions = 0;
  report(ctl, 530);
  expectMotors(255, 255);   // Releasing D-pad returns to live analog inputs.

  // Releasing either drive method stops without needing Cross.
  ctl.leftY = 0;
  report(ctl, 531);
  expectMotors(0, 0);
  assert(armed);
  ctl.directions = DPAD_UP;
  report(ctl, 532);
  expectMotors(255, 255);
  ctl.directions = 0;
  report(ctl, 533);
  expectMotors(0, 0);
  assert(armed);
  ctl.leftY = -512;
  report(ctl, 534);
  expectMotors(255, 255);

  // Updates from another device must not renew this controller's timeout.
  ctl.fresh = false;
  BP32.changed = true;
  testClock = 833;
  loop();
  expectMotors(255, 255);
  BP32.changed = true;
  testClock = 834;
  loop();
  expectMotors(0, 0);
  assert(!armed);
  report(ctl, 840);
  expectMotors(0, 0);

  ctl.leftY = 0;
  report(ctl, 850);
  report(ctl, 1000);
  report(ctl, 1150);
  assert(armed);
  ctl.leftY = -512;
  report(ctl, 1160);
  expectMotors(255, 255);
  report(ctl, 1600);       // Fresh data after a long loop gap still disarms.
  expectMotors(0, 0);
  assert(!armed);

  // Cross overrides motion and requires release + neutral before rearming.
  ctl.leftY = 0;
  report(ctl, 1610);
  report(ctl, 1800);
  report(ctl, 1910);
  assert(armed);
  ctl.cross = true;
  ctl.directions = DPAD_UP;
  report(ctl, 1920);
  expectMotors(0, 0);
  ctl.cross = false;
  report(ctl, 1930);
  expectMotors(0, 0);

  Controller extra;
  onConnectedController(&extra);
  assert(!extra.connected && gamepad == &ctl);
  onDisconnectedController(&extra);
  assert(gamepad == &ctl);
  onDisconnectedController(&ctl);
  expectMotors(0, 0);
  assert(!armed && gamepad == nullptr);

  // Timeout subtraction must remain correct across millis() rollover.
  testClock = UINT32_MAX - 200;
  ctl.directions = 0;
  onConnectedController(&ctl);
  report(ctl, UINT32_MAX - 200);
  report(ctl, UINT32_MAX - 50);
  report(ctl, 100);
  assert(armed);
  ctl.leftY = -512;
  report(ctl, 110);
  expectMotors(255, 255);
  ctl.fresh = false;
  testClock = 410;
  loop();
  expectMotors(0, 0);
  assert(!armed);

  std::cout << "PASS: D-pad, analog mixing, deadband, priority, neutral arming, "
               "stop, timeout, controller ownership and clock rollover\n";
}
