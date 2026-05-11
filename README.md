# Group 13 Final Submission

`Operation Signal Break` is a multi-game STM32 project built for a single
NUCLEO-L476RG target. The program boots into one shared mission menu and lets
the player launch three connected games on the same board and display.

Mission flow:
- Mission 1 - `LOCATE`: explore the tomb and find the signal source
- Mission 2 - `STEAL`: infiltrate the grid and steal the signal core
- Mission 3 - `ESCAPE`: survive the pursuit and escape with the signal

## Project Summary

This repository combines three game modules with one shared hardware and UI
layer:

- `game_1`: `The Cursed Tome`, a pseudo-3D tomb exploration and combat game
- `game_2`: `Signal Thief`, a dual-stick action game with upgrades and
  survival pressure
- `game_3`: a space combat package with asteroid, striker, and versus modes
- `shared`: the common mission menu and button input handling
- `Core`, `Drivers`, `Joystick`, `Buzzer`, `PWM`, `ST7789V2_Driver_STM32L4`:
  STM32 platform support and hardware drivers

The project is built as one executable, so all three games share the same LCD,
buttons, joysticks, buzzer, timers, and startup code.

## Software Architecture

At a high level, the program is split into three layers:

1. Platform layer
   STM32CubeMX-generated startup code, HAL drivers, LCD driver, buzzer, PWM,
   and joystick support.
2. Shared framework
   The mission menu in `shared/Menu.c` and debounced button handling in
   `shared/InputHandler.c`.
3. Game modules
   Each game owns its own runtime loop and returns control to the shared menu
   when finished.

Key entry points:

- `Core/Src/main.c` initializes hardware and dispatches menu/game states
- `shared/Menu.c` renders the `MISSION CONSOLE` menu
- `game_1/Game_1.c` launches Mission 1
- `game_2/Game_2.c` launches Mission 2
- `game_3/Game_3.c` launches Mission 3

## Repository Layout

```text
group_submission/
|- Core/                         STM32CubeMX-generated application code
|- Drivers/                      STM32 HAL and CMSIS drivers
|- shared/                       Mission menu and shared input layer
|- game_1/                       Mission 1 game code
|- game_2/                       Mission 2 game code
|- game_3/                       Mission 3 game code
|- Joystick/                     Joystick driver
|- Buzzer/                       Buzzer driver
|- PWM/                          PWM helper
|- ST7789V2_Driver_STM32L4/      LCD driver
|- cmake/                        Toolchain and CubeMX CMake integration
|- CMakeLists.txt                Main build definition
|- CMakePresets.json             Debug/Release/Game1Only presets
|- Unit_4_1_Menu_Template.ioc    STM32CubeMX project file
```

## Build Requirements

- ARM GCC toolchain for Cortex-M
- CMake 3.22 or newer
- Ninja
- STM32Cube-generated source tree already present in this repository
- Optional: `STM32_Programmer_CLI` for command-line flashing

This project already includes CMake presets for the common build modes.

## Build Instructions

Full multi-game debug build:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Mission 1 only build:

```powershell
cmake --preset Game1Debug
cmake --build --preset Game1Debug
```

Release build:

```powershell
cmake --preset Release
cmake --build --preset Release
```

Default full-build output:

- `build/Debug/Unit_4_1_Menu_Template.elf`
- `build/Debug/Unit_4_1_Menu_Template.bin`

## Flashing

Example command using STM32CubeProgrammer CLI:

```powershell
STM32_Programmer_CLI.exe `
  --connect port=SWD mode=UR reset=HWrst `
  --download build/Debug/Unit_4_1_Menu_Template.bin 0x08000000 `
  --verify `
  --start 0x08000000
```

If the CLI reports `No debug probe detected`, check the ST-LINK USB cable,
power, and board connection before retrying.

## Controls

Shared menu:

- Joystick up/down: move the mission selection
- `BT2` or `BT3`: confirm the highlighted mission

Mission 1:

- Tomb exploration and combat on the LCD
- Hold `BT2` and `BT3` together to return to the mission menu

Mission 2:

- Left joystick: movement
- Right joystick: aiming and mission interactions
- `BT2`, `BT3`, and the joystick switch buttons are used in menus and gameplay

Mission 3:

- Single-player and two-player space combat modes
- Uses one or two joysticks depending on the selected mode

## Notes

- The current CMake target name is still `Unit_4_1_Menu_Template`, so the ELF
  and BIN output keep that filename.
- `GAME1_ONLY` is supported through the `Game1Debug` preset for faster
  Mission 1 iteration.
- Build folders, binaries, and generated debug artifacts are intentionally
  ignored by `.gitignore`.
