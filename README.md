# Maker ESP32

**English** | [Bahasa Melayu](README.ms.md)

Arduino projects for **Maker ESP32 + Robo ESP32**. The first sketch controls a
mobile robot with two DC motors using a PS4 / DualShock 4 controller.

## Project structure

```text
Arduino/
  MakerESP32_MobileRobot_PS4/
    MakerESP32_MobileRobot_PS4.ino
tests/
  test_controls.cpp
  stubs/
```

## Hardware and connections

| Component | Connection | GPIO |
|---|---|---|
| Right motor | Robo ESP32 MOTOR1 | A = 12, B = 13 |
| Left motor | Robo ESP32 MOTOR2 | A = 14, B = 27 |
| Maker ESP32 | ESP32 socket on Robo ESP32 | Follow the board orientation |
| PS4 / DualShock 4 | Bluetooth directly to Maker ESP32 | No USB receiver |

Robo ESP32 uses the motor driver in `PWM_PWM` mode. Maker ESP32 uses an
ESP32-WROOM-32E; do not substitute an ESP32-S3/C3 for a PS4 controller using
Bluetooth Classic. Compatibility with clone controllers requires physical testing.

Power Robo ESP32 with a **single-cell LiPo/Li-ion battery** through the appropriate
battery connector, or **3.6–6V at the Robo ESP32 VIN terminal**. The motor voltage
follows the board supply. Do not connect a 2S battery directly. These limits apply
to Robo ESP32, not the power pins on Maker ESP32. Match the motor voltage and
stall current to the driver and supply ratings. This project uses a 6V battery
and TT motors; the exact motor model and stall current have not been confirmed.

## Controls

Both control methods are active without a mode-switch button. **The D-pad takes
priority** while pressed. Releasing it returns control to the current stick
positions; if a stick is still deflected, the robot continues moving according
to that input.

### 1. Left D-pad: eight directions

`+` = forward, `-` = reverse, `0` = motor output stopped. The table describes robot
motion after correcting the polarity of each motor.

| D-pad | Movement | Left motor | Right motor |
|---|---|---|---|
| Up | Forward | + | + |
| Down | Reverse | - | - |
| Left | Rotate left in place | - | + |
| Right | Rotate right in place | + | - |
| Up + left | Forward left | 0 | + |
| Up + right | Forward right | + | 0 |
| Down + left | Reverse left | 0 | - |
| Down + right | Reverse right | - | 0 |

Diagonal directions drive only one wheel. Conflicting combinations such as
up + down stop the motor outputs.

### 2. Dual analog sticks

- **Left stick, Y axis:** up = forward, down = reverse.
- **Right stick, X axis:** left/right = steering.
- Left stick X and right stick Y are unused.
- Steering alone rotates the robot in place.
- Both inputs are mixed using `left = throttle + steering` and
  `right = throttle - steering`, then normalized to the PWM limit.
- Right steering always rotates the robot's body to the right, including while
  reversing; this is differential-drive rotation control. D-pad diagonals select
  the direction of travel shown in the table instead.

### Stopping and enabling movement

- **Normal stop:** release the D-pad and center the sticks. Motor outputs become
  zero on the next input report; there is no need to press Cross or wait 300 ms
  to stop. The robot stays ready.
- **Cross (×)** is an additional stop that overrides other inputs, even when a
  stick is still deflected. It stops the motor outputs and clears the ready state.
- To become ready again, release Cross and the D-pad, then center both active
  stick axes for **300 ms** while controller data is being received.
- The same neutral requirement applies after startup, reconnection, or timeout.
- No new data from the active controller for **300 ms**: stop the motors.
- Controller disconnection: stop the motors when the disconnection is detected.
- Only one gamepad is accepted; additional controllers are rejected. The touchpad
  virtual mouse is disabled.
- There is no need to hold R1 to drive.

Stopping means setting PWM outputs to zero. The wheels may continue moving due
to inertia; this is not a physical emergency stop. The software timeout requires
the loop/CPU to remain running. Use the power switch to disconnect power if needed.

## Arduino IDE setup

1. Add these two URLs in **Preferences → Additional Boards Manager URLs**:

   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
   ```

2. In Boards Manager, install the official ESP32 package following the Bluepad32
   guide, and **ESP32 + Bluepad32 version 4.1.0**. This is the Bluepad32 package
   version used for the verified build in this project.
3. Select **ESP32 Dev Module from the ESP32 + Bluepad32 menu**. Selecting a regular
   ESP32 board does not provide the required Bluepad32 Bluetooth integration.
4. In Library Manager, install **Cytron Motor Drivers Library version 1.0.1**.
   Bluepad32 comes with the board package; PS4Controller is not required.
5. Set **Flash Size: 8MB** and **Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)**.
   Select the Maker ESP32 USB port. Leave other settings at the board package defaults.
6. Open `Arduino/MakerESP32_MobileRobot_PS4/MakerESP32_MobileRobot_PS4.ino`,
   then Verify and Upload. **Keep the wheels off the ground during initial testing.**
7. Open Serial Monitor at **115200 baud**.
8. Hold **SHARE + PS** on the controller until the light bar flashes to pair.
   After the connected message, center the controls until the `Ready` message appears.

The code preserves Bluetooth pairing keys across resets. It does not call
`forgetBluetoothKeys()` on every boot. If the controller tries to connect to a
previous console or computer, disconnect it there and try SHARE + PS pairing again.

## Sketch settings

| Constant | Default | Purpose |
|---|---|---|
| `MAX_PWM` | 255 | Analog PWM limit, 100% duty |
| `DPAD_PWM` | 255 | Fixed D-pad PWM; must not exceed `MAX_PWM` |
| `AXIS_DEADBAND` | 40 | Tolerance around stick center, axis range ±512 |
| `INPUT_TIMEOUT_MS` | 300 | Maximum time without new data |
| `NEUTRAL_HOLD_MS` | 300 | Neutral period before becoming ready |
| `INVERT_RIGHT` | false | Reverse the right motor direction |
| `INVERT_LEFT` | false | Reverse the left motor direction |

The D-pad uses full PWM; analog input still controls PWM proportionally from zero
to the maximum. PWM percentage does not guarantee a matching percentage of physical
speed. If a motor does not start at low PWM, check the supply, load, and mechanics
before increasing the analog input. The code rescales the input after the deadband,
without a minimum-PWM jump or ramp.

## Testing on the robot

1. Lift the wheels. Confirm there is no motion during startup, before pairing,
   or when pairing with a stick deflected.
2. Once ready, briefly press up. Both wheels should drive the robot forward.
   If one is reversed, change `INVERT_RIGHT` or `INVERT_LEFT`, then upload again.
3. Check all eight D-pad directions against the table and test both sticks separately.
4. Check that the D-pad overrides analog input and returns to analog control when released.
5. Test Cross while moving: the outputs stop, and movement does not resume until
   the controls return to neutral.
6. Turn off the controller while driving with the wheels raised. Check that the
   motors stop and reconnecting with a held input does not immediately move them.
7. Test slowly on the floor; check drift, current/supply, motor heating, resets or
   brownouts, Bluetooth response, and stopping distance. The D-pad uses 100% PWM;
   use small analog inputs for slow testing.

## Validation

Verified on 9 September 2026:

- **ESP32 compile passed:** `esp32-bluepad32:esp32@4.1.0` and
  `Cytron Motor Drivers Library@1.0.1`.
- FQBN: `esp32-bluepad32:esp32:esp32:FlashSize=8M,PartitionScheme=huge_app`.
- Sketch: **719417 bytes**, global RAM: **87228 bytes**.
- **Host logic tests passed:** eight D-pad directions and invalid combinations,
  deadband, analog output limits, mixing, D-pad priority, release-to-stop without
  Cross, neutral arming, Cross stop, timeout including a new report after a long
  gap, controller ownership, and `millis()` rollover.
- **Not yet tested on hardware:** board upload, PS4 pairing, motor direction,
  supply performance, and actual stopping time.

Compile with Arduino CLI after installing the dependencies:

```sh
arduino-cli compile \
  --fqbn 'esp32-bluepad32:esp32:esp32:FlashSize=8M,PartitionScheme=huge_app' \
  --build-path /tmp/maker-esp32-build-output \
  Arduino/MakerESP32_MobileRobot_PS4
```

Run the host tests from the repository root with a C++17 compiler:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs \
  tests/test_controls.cpp -o /tmp/maker-esp32-test
/tmp/maker-esp32-test
```

The tests include the actual sketch with Arduino/Bluepad32/motor stubs. They test
command logic only, not the Bluetooth stack, electrical outputs, or physical motion.
Arduino IDE does not use these stubs.

## Official references

- [Maker ESP32](https://my.cytron.io/p-maker-esp32-bundle)
- [Robo ESP32 and datasheet](https://my.cytron.io/p-robo-esp32)
- [Cytron motor pin example](https://github.com/CytronTechnologies/Cytron-ROBO-ESP32/blob/main/Getting%20Started%20Guide/Arduino/DCMotor/DCMotor.ino)
- [Cytron Motor Driver Library](https://github.com/CytronTechnologies/CytronMotorDriver)
- [Bluepad32 Arduino guide](https://bluepad32.readthedocs.io/en/latest/plat_arduino/)
- [Supported controllers and pairing](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/)
- [Cytron ESP32 + PS4 tutorial](https://my.cytron.io/tutorial/esp32-ps4controller-beginner)
