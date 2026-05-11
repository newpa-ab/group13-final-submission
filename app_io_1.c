#include "app_io.h"

#include <math.h>
#include <string.h>

#include "adc.h"
#include "main.h"
#include "tim.h"

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} AppButtonDef_t;

static const AppButtonDef_t g_button_defs[APP_BUTTON_COUNT] = {
    {BTN2_GPIO_Port, BTN2_Pin},
    {BTN3_GPIO_Port, BTN3_Pin},
    {BTN4_GPIO_Port, BTN4_Pin},
    {BTN5_GPIO_Port, BTN5_Pin},
    {BTN8_GPIO_Port, BTN8_Pin},
    {BTN9_GPIO_Port, BTN9_Pin}
};

Joystick_cfg_t joystick_cfg;
Joystick_t joystick_data;
Buzzer_cfg_t buzzer_cfg;

static uint8_t g_button_down[APP_BUTTON_COUNT];
static uint8_t g_button_pressed[APP_BUTTON_COUNT];
static uint32_t g_button_down_since[APP_BUTTON_COUNT];
static UserInput g_last_input;

static uint8_t appio_read_button(AppButton_t button)
{
    return HAL_GPIO_ReadPin(g_button_defs[button].port, g_button_defs[button].pin) == GPIO_PIN_RESET ? 1U : 0U;
}

static UserInput appio_build_dpad_input(void)
{
    UserInput input;
    int8_t dx = 0;
    int8_t dy = 0;

    memset(&input, 0, sizeof(input));
    input.direction = CENTRE;
    input.angle = -1.0f;
    input.magnitude = 0.0f;

    if (g_button_down[APP_BUTTON_LEFT]) {
        dx--;
    }
    if (g_button_down[APP_BUTTON_RIGHT]) {
        dx++;
    }
    if (g_button_down[APP_BUTTON_UP]) {
        dy++;
    }
    if (g_button_down[APP_BUTTON_DOWN]) {
        dy--;
    }

    if (dx == 0 && dy == 0) {
        return input;
    }

    input.magnitude = (dx != 0 && dy != 0) ? 0.85f : 1.0f;

    if (dx == 0 && dy > 0) {
        input.direction = N;
        input.angle = 0.0f;
    } else if (dx > 0 && dy > 0) {
        input.direction = NE;
        input.angle = 45.0f;
    } else if (dx > 0 && dy == 0) {
        input.direction = E;
        input.angle = 90.0f;
    } else if (dx > 0 && dy < 0) {
        input.direction = SE;
        input.angle = 135.0f;
    } else if (dx == 0 && dy < 0) {
        input.direction = S;
        input.angle = 180.0f;
    } else if (dx < 0 && dy < 0) {
        input.direction = SW;
        input.angle = 225.0f;
    } else if (dx < 0 && dy == 0) {
        input.direction = W;
        input.angle = 270.0f;
    } else {
        input.direction = NW;
        input.angle = 315.0f;
    }

    return input;
}

void AppIO_Init(void)
{
    memset(&joystick_cfg, 0, sizeof(joystick_cfg));
    memset(&joystick_data, 0, sizeof(joystick_data));
    memset(&buzzer_cfg, 0, sizeof(buzzer_cfg));
    memset(g_button_down, 0, sizeof(g_button_down));
    memset(g_button_pressed, 0, sizeof(g_button_pressed));
    memset(g_button_down_since, 0, sizeof(g_button_down_since));

    joystick_cfg.adc = &hadc1;
    joystick_cfg.x_channel = ADC_CHANNEL_1;
    joystick_cfg.y_channel = ADC_CHANNEL_2;
    joystick_cfg.sampling_time = ADC_SAMPLETIME_6CYCLES_5;
    joystick_cfg.center_x = JOYSTICK_DEFAULT_CENTER_X;
    joystick_cfg.center_y = JOYSTICK_DEFAULT_CENTER_Y;
    joystick_cfg.deadzone = JOYSTICK_DEADZONE;
    joystick_cfg.setup_done = 0U;

    Joystick_Init(&joystick_cfg);
    Joystick_Calibrate(&joystick_cfg);

    buzzer_cfg.htim = &htim2;
    buzzer_cfg.channel = TIM_CHANNEL_3;
    buzzer_cfg.tick_freq_hz = 1000000U;
    buzzer_cfg.min_freq_hz = 40U;
    buzzer_cfg.max_freq_hz = 12000U;
    buzzer_cfg.setup_done = 0U;
    buzzer_cfg.pwm_started = 0U;
    buzzer_init(&buzzer_cfg);

    g_last_input.direction = CENTRE;
    g_last_input.angle = -1.0f;
    g_last_input.magnitude = 0.0f;

    AppIO_Poll();
}

void AppIO_Poll(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t i;

    Joystick_Read(&joystick_cfg, &joystick_data);
    g_last_input = Joystick_GetInput(&joystick_data);

    for (i = 0U; i < APP_BUTTON_COUNT; i++) {
        uint8_t down = appio_read_button((AppButton_t)i);
        if (down && !g_button_down[i]) {
            g_button_pressed[i] = 1U;
            g_button_down_since[i] = now;
        } else if (!down) {
            g_button_down_since[i] = 0U;
        }
        g_button_down[i] = down;
    }
}

UserInput AppIO_GetInput(void)
{
    UserInput dpad_input = appio_build_dpad_input();

    if (g_last_input.direction != CENTRE || dpad_input.direction == CENTRE) {
        return g_last_input;
    }

    return dpad_input;
}

uint8_t AppIO_ButtonDown(AppButton_t button)
{
    return g_button_down[button];
}

uint8_t AppIO_ButtonPressed(AppButton_t button)
{
    uint8_t pressed = g_button_pressed[button];
    g_button_pressed[button] = 0U;
    return pressed;
}

uint8_t AppIO_ComboHeld(AppButton_t first, AppButton_t second, uint32_t hold_ms)
{
    uint32_t since;

    if (!g_button_down[first] || !g_button_down[second]) {
        return 0U;
    }

    since = g_button_down_since[first];
    if (g_button_down_since[second] > since) {
        since = g_button_down_since[second];
    }

    if (since == 0U) {
        return 0U;
    }

    return (HAL_GetTick() - since) >= hold_ms ? 1U : 0U;
}
