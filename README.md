# Pattern-Based Lock Mechanism (FSM + EEPROM)

Tinkercad Simulation: https://www.tinkercad.com/things/jDIGi1TuK2K-pattern-based-lock-mechanism
This Arduino project implements a pattern-based electronic lock using:

- 4 push buttons as the password input
- 4 LEDs for step-by-step feedback
- 1 LED to indicate Password Change Mode
- 1 push button to enter Password Change Mode
- 1 servo motor that rotates to unlock on correct pattern
- EEPROM to store the password so it survives power loss
- A finite state machine (FSM) to keep the logic clean and structured

## ✨ Features

### Pattern-Based Password
Password-based access using 4 buttons.

### Visual Feedback
- LED 1 → first correct press
- LED 2 → second correct press
- LED 3 → third correct press
- LED 4 → fourth correct press

### Wrong Input Handling
- All 4 LEDs flash rapidly for ~1 second.
- Sequence resets.

### Password Change Mode
- Triggered via a dedicated button (interrupt-based).
- Mode LED indicates when you're changing the password.
- New pattern is saved to EEPROM.

### Servo Unlock
- On correct pattern: servo rotates to unlock position, waits, then locks again.

### Soft Lockout
- After 3 wrong attempts, the system locks out for a defined duration.

### Finite State Machine
Uses switch-case structure with the following states:
- `password_change`
- `password_check`
- `password_correct`
- `password_wrong`

## 🧰 Hardware Connections

### Pins Used

**Password input buttons (with internal pull-ups):**
- pbs[0] → Digital 3
- pbs[1] → Digital 4
- pbs[2] → Digital 5
- pbs[3] → Digital 6

**Password feedback LEDs:**
- password_leds[0] → Digital 8
- password_leds[1] → Digital 9
- password_leds[2] → Digital 10
- password_leds[3] → Digital 11

**Password Change Mode:**
- Mode LED → Digital 12 (password_change_led)
- Mode button → Digital 2 (password_change_pb, interrupt pin, INPUT_PULLUP)

**Servo:**
- Signal → Digital 13

**Note:** All push buttons are configured with `INPUT_PULLUP`, so they should be wired to GND when pressed.

## 🔄 State Machine Overview

The logic is structured as a finite state machine via the `updateStateMachine()` function and the following enum:

```cpp
enum State {password_change, password_check, password_correct, password_wrong};
volatile byte currentState;
```

### 1. password_change

**Entered:**
- On startup if EEPROM at base address is 0.
- When the Password Change button (pin 2) triggers the interrupt.

**Behavior:**
- Mode LED (pin 12) turns ON.
- Waits for 4 button presses.
- Each press:
  - Debounced.
  - Button index (0–3) is stored in `password[i]`.
  - Same value is saved to EEPROM via `EEPROM.update(address + i, j)`.
- After 4 entries:
  - Mode LED OFF.
  - `currentState = password_check`.

### 2. password_check

**Status:** Default "idle" / main state.

**Behavior:**
- Waits for a sequence of 4 valid button presses.
- For each press:
  - Debounces and waits for release.
  - Compares button index `j` with stored `password[i]`.
  - If correct: Lights the corresponding LED `password_leds[i]` and moves to next position `i++`.
  - If wrong: Sets `currentState = password_wrong` and breaks out of the loop.
- If all 4 inputs are correct and still in this state: `currentState = password_correct`.
- If the state changed (wrong or change mode), LEDs are reset.

### 3. password_correct

**Behavior:**
- Rotates servo to open position (`SERVO_OPEN_ANGLE = 180`).
- Waits `UNLOCK_DURATION` (currently 5000 ms).
- Returns servo to closed position (`SERVO_CLOSED_ANGLE = 0`).
- Turns OFF all password LEDs.
- Returns to `password_check`.

### 4. password_wrong

**Behavior:**
- Keeps track of wrong attempts using timeout.
- If `timeout < MAX_TIMEOUT` (3):
  - Increments timeout.
  - Flashes all password LEDs rapidly for ~1 second.
  - Returns to `password_check`.
- Else:
  - Resets timeout to 0.
  - Enters lockout using `LOCKOUT_DURATION` (currently 50000 ms).
  - Returns to `password_check`.

**Tip:** You can increase `LOCKOUT_DURATION` to 300000 ms (5 minutes) to match a stricter soft lockout.

## 💾 EEPROM Behavior

**On startup:**
- If `EEPROM.read(address) == 0`, system assumes no password set → enters `password_change` to let you define a new one.
- Otherwise, it reads 4 bytes from EEPROM into the `password[]` array.

**On password change:**
- Each new button press is saved immediately using `EEPROM.update(address + i, j)`.
- This ensures the pattern is persistent across power cycles.

## ⚙️ Configuration Constants

You can tweak these at the top of the code:

```cpp
const byte PASSWORD_LENGTH = 4;
const byte SERVO_OPEN_ANGLE = 180;
const byte SERVO_CLOSED_ANGLE = 0;
const unsigned long UNLOCK_DURATION = 5000;   // ms the servo stays unlocked
const unsigned long LOCKOUT_DURATION = 50000; // ms lockout after MAX_TIMEOUT wrong tries
const byte MAX_TIMEOUT = 3;                   // number of wrong attempts before lockout
```

For a real 5-minute lockout:

```cpp
const unsigned long LOCKOUT_DURATION = 300000; // 5 minutes in ms
```

## 🚀 Getting Started

### Create the Arduino Sketch

1. Open the Arduino IDE.
2. Create a new sketch (e.g. `PatternLockFSM`).
3. Paste the provided code into the `.ino` file.

### Wire Up the Circuit

Connect buttons, LEDs, and servo according to the Hardware Connections section.

### Select Board & Port

1. Go to Tools → Board → Arduino Uno.
2. Go to Tools → Port → Select your port.

### Upload

Click the Upload button.

### First Run

- On first boot (or after EEPROM reset), the system will enter Password Change Mode.
- Enter your 4-button pattern → it will be saved to EEPROM.
- From then on, you can unlock using that pattern.

## 🧪 Suggested Tests

### Unlock Test
Enter the correct pattern → Servo should unlock, LEDs should behave sequentially.

### Failure Test
Enter a wrong pattern:
- LEDs should flash rapidly for 1 second.
- After 3 consecutive wrong tries, the system should lock out for `LOCKOUT_DURATION`.

### Change Password
Press the Password Change button:
- Mode LED should turn ON.
- New 4-button sequence should be stored and used from then on.
