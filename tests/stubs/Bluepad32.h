// Host-test doubles only. Firmware builds use the real Bluepad32 package.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
using std::max;
template <typename T> T constrain(T value, T low, T high) {
  return std::min(std::max(value, low), high);
}
inline uint32_t testClock = 0;
inline uint32_t millis() { return testClock; }
inline void delay(uint32_t ms) { testClock += ms; }
inline struct SerialMock {
  void begin(int) {}
  void println(const char*) {}
} Serial;
constexpr uint8_t DPAD_UP = 1, DPAD_DOWN = 2, DPAD_RIGHT = 4, DPAD_LEFT = 8;
struct Controller {
  int leftY = 0, rightX = 0;
  uint8_t directions = 0;
  int trigger = 0;
  bool connected = true, fresh = true, cross = false, physicalGamepad = true;
  uint8_t dpad() const { return directions; }
  int axisY() const { return leftY; }
  int axisRX() const { return rightX; }
  bool a() const { return cross; }
  int throttle() const { return trigger; }
  bool isGamepad() const { return physicalGamepad; }
  bool isConnected() const { return connected; }
  bool hasData() const { return fresh; }
  void disconnect() { connected = false; }
};
using ControllerPtr = Controller*;
inline struct BluepadMock {
  bool changed = false;
  void setup(void (*)(ControllerPtr), void (*)(ControllerPtr)) {}
  void enableVirtualDevice(bool) {}
  bool update() { bool result = changed; changed = false; return result; }
} BP32;
