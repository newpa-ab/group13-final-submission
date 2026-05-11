#ifndef APP_IO_H
#define APP_IO_H

#include <stdint.h>

#include "Buzzer.h"
#include "Joystick.h"

typedef enum {
    APP_BUTTON_BT2 = 0,
    APP_BUTTON_BT3,
    APP_BUTTON_LEFT,
    APP_BUTTON_RIGHT,
    APP_BUTTON_UP,
    APP_BUTTON_DOWN,
    APP_BUTTON_COUNT
} AppButton_t;

extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;
extern Buzzer_cfg_t buzzer_cfg;

void AppIO_Init(void);
void AppIO_Poll(void);
UserInput AppIO_GetInput(void);
uint8_t AppIO_ButtonDown(AppButton_t button);
uint8_t AppIO_ButtonPressed(AppButton_t button);
uint8_t AppIO_ComboHeld(AppButton_t first, AppButton_t second, uint32_t hold_ms);

#endif
