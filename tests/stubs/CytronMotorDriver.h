// Records logical PWM commands; does not simulate motor physics or the driver.
#pragma once
enum MODE { PWM_PWM };
struct CytronMD {
  int speed = 0;
  CytronMD(MODE, int, int) {}
  void setSpeed(int value) { speed = value; }
};
