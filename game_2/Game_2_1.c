#include "Game_2.h"
#include "PongEngine.h"
#include "InputHandler.h"
#include "LCD.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "main.h"
#include "stm32l4xx_hal.h"

#include <stdio.h>

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;
extern Joystick_cfg_t joystick2_cfg;
extern Joystick_t joystick2_data;

typedef enum {
    GAME2_STATE_PLAYING = 0,
    GAME2_STATE_PAUSED,
    GAME2_STATE_GAME_OVER,
    GAME2_STATE_VICTORY
} Game2State;

static PongEngine_t signal_thief_engine;

#define GAME2_FRAME_TIME_MS 16U
#define PAUSE_EXIT_WINDOW_MS 5000U
#define GAME2_BUTTON_DEBOUNCE_MS 180U
#define GAME2_SCREEN_WIDTH 240U
#define GAME2_FONT_CELL_WIDTH 6U

static uint32_t pause_start_tick = 0;
static uint8_t btn2_was_down = 0;
static uint8_t btn3_was_down = 0;
static uint8_t btn8_was_down = 0;
static uint32_t btn2_last_tick = 0;
static uint32_t btn3_last_tick = 0;
static uint32_t btn8_last_tick = 0;
static uint8_t pause_exit_armed = 0;

static uint8_t Game2_PinDown(GPIO_TypeDef* port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

static uint16_t Game2_CenteredX(const char* text, uint8_t size)
{
    uint16_t chars = 0;
    uint16_t width = 0;

    while (text[chars] != '\0') {
        chars++;
    }

    width = (uint16_t)(chars * GAME2_FONT_CELL_WIDTH * size);
    if (width >= GAME2_SCREEN_WIDTH) {
        return 0;
    }
    return (uint16_t)((GAME2_SCREEN_WIDTH - width) / 2U);
}

static void Game2_PrintCentered(const char* text, uint16_t y, uint8_t colour, uint8_t size)
{
    LCD_printString(text, Game2_CenteredX(text, size), y, colour, size);
}

static void Game2_PrimeButtons(void)
{
    btn2_was_down = Game2_PinDown(BTN2_GPIO_Port, BTN2_Pin);
    btn3_was_down = Game2_PinDown(BTN3_GPIO_Port, BTN3_Pin);
    btn8_was_down = Game2_PinDown(BTN8_GPIO_Port, BTN8_Pin);
    btn2_last_tick = HAL_GetTick();
    btn3_last_tick = btn2_last_tick;
    btn8_last_tick = btn2_last_tick;
}

static uint8_t Game2_ButtonEvent(uint8_t interrupt_event,
                                 GPIO_TypeDef* port,
                                 uint16_t pin,
                                 uint8_t* was_down,
                                 uint32_t* last_tick)
{
    uint32_t now = HAL_GetTick();
    uint8_t down = Game2_PinDown(port, pin);
    uint8_t pressed = interrupt_event;

    if (down && !(*was_down) && (now - *last_tick) > GAME2_BUTTON_DEBOUNCE_MS) {
        pressed = 1;
        *last_tick = now;
    }

    if (interrupt_event) {
        *last_tick = now;
    }

    *was_down = down;
    return pressed;
}

static uint8_t Game2_BT2Pressed(void)
{
    return Game2_ButtonEvent(current_input.btn2_pressed,
                             BTN2_GPIO_Port, BTN2_Pin,
                             &btn2_was_down, &btn2_last_tick);
}

static uint8_t Game2_BT3Pressed(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t down = Game2_PinDown(BTN3_GPIO_Port, BTN3_Pin);
    uint8_t pressed = current_input.btn3_pressed;

    if (pressed) {
        btn3_last_tick = now;
    } else if (down && (now - btn3_last_tick) > GAME2_BUTTON_DEBOUNCE_MS) {
        pressed = 1;
        btn3_last_tick = now;
    }

    btn3_was_down = down;
    return pressed;
}

static uint8_t Game2_BT8Pressed(void)
{
    return Game2_ButtonEvent(current_input.btn8_pressed,
                             BTN8_GPIO_Port, BTN8_Pin,
                             &btn8_was_down, &btn8_last_tick);
}

static void Game2_Reset(void)
{
    PongEngine_Init(&signal_thief_engine, 0, 0, 0, 0, 0, 0.0f);
    pause_start_tick = 0;
    pause_exit_armed = 0;
    Game2_PrimeButtons();
}

static void Game2_DrawStartMenu(uint8_t selected)
{
    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(10, 10, 220, 220, 6, 0);
    LCD_Draw_Rect(18, 18, 204, 204, 1, 0);
    Game2_PrintCentered("SIGNAL THIEF", 38, 6, 2);
    Game2_PrintCentered("CAT AGENT", 64, 1, 1);
    LCD_Draw_Line(42, 88, 198, 88, 6);

    LCD_Draw_Rect(54, 108, 132, 30, selected == 0 ? 6 : 9, selected == 0 ? 0 : 1);
    Game2_PrintCentered("PLAY", 116, selected == 0 ? 6 : 1, 2);

    LCD_Draw_Rect(42, 150, 156, 30, selected == 1 ? 6 : 9, selected == 1 ? 0 : 1);
    Game2_PrintCentered("INSTRUCTION", 158, selected == 1 ? 6 : 1, 2);

    Game2_PrintCentered("UP/DOWN joystick", 198, 1, 1);
    Game2_PrintCentered("BT2/SW select", 212, 6, 1);
    LCD_Refresh(&cfg0);
}

static void Game2_ShowInstruction(void)
{
    Input_Clear();
    Game2_PrimeButtons();

    while (1) {
        uint8_t exit_pressed = 0;

        Input_Read();
        exit_pressed = Game2_BT3Pressed();

        if (exit_pressed) {
            buzzer_tone(&buzzer_cfg, 1600, 25);
            HAL_Delay(45);
            buzzer_off(&buzzer_cfg);
            Game2_PrimeButtons();
            return;
        }

        LCD_Fill_Buffer(0);
        LCD_Draw_Rect(10, 10, 220, 220, 1, 0);
        Game2_PrintCentered("INSTRUCTION", 26, 6, 2);
        LCD_Draw_Line(34, 56, 206, 56, 6);

        Game2_PrintCentered("Left joystick: move", 74, 1, 1);
        Game2_PrintCentered("Right joystick: aim", 92, 1, 1);
        Game2_PrintCentered("Right SW: signal trap", 110, 6, 1);
        Game2_PrintCentered("Decode signals", 136, 1, 1);
        Game2_PrintCentered("with right joystick", 152, 1, 1);
        Game2_PrintCentered("Choose upgrades", 178, 6, 1);
        Game2_PrintCentered("after weapon max", 194, 6, 1);
        Game2_PrintCentered("Press left SW back", 214, 5, 1);
        LCD_Refresh(&cfg0);
        HAL_Delay(35);
    }
}

static void Game2_ShowStartMenu(void)
{
    uint8_t selected = 0;
    Direction last_dir = CENTRE;

    Input_Clear();
    Game2_PrimeButtons();

    while (1) {
        UserInput input;
        uint8_t select_pressed = 0;

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);
        input = Joystick_GetInput(&joystick_data);

        if ((input.direction == N || input.direction == NE || input.direction == NW) &&
            last_dir == CENTRE) {
            selected = 0;
            buzzer_tone(&buzzer_cfg, 1200, 15);
        } else if ((input.direction == S || input.direction == SE || input.direction == SW) &&
                   last_dir == CENTRE) {
            selected = 1;
            buzzer_tone(&buzzer_cfg, 1200, 15);
        }
        last_dir = input.direction;

        select_pressed = Game2_BT2Pressed();
        select_pressed = (uint8_t)(select_pressed || Game2_BT8Pressed());

        if (select_pressed) {
            buzzer_tone(&buzzer_cfg, 1800, 28);
            HAL_Delay(55);
            buzzer_off(&buzzer_cfg);
            if (selected == 0) {
                Game2_PrimeButtons();
                return;
            }
            Game2_ShowInstruction();
        }

        Game2_DrawStartMenu(selected);
        HAL_Delay(40);
    }
}

static void Game2_ShowIntro(void)
{
    uint16_t x = 0;

    buzzer_tone(&buzzer_cfg, 1800, 28);
    HAL_Delay(60);
    buzzer_off(&buzzer_cfg);

    for (x = 0; x < 240; x += 10) {
        LCD_Fill_Buffer(0);
        LCD_Draw_Rect(18, 48, 204, 128, 1, 0);
        Game2_PrintCentered("ACCESSING", 66, 1, 2);
        Game2_PrintCentered("SIGNAL GRID", 96, 6, 2);
        LCD_Draw_Line(28, 138, (uint16_t)(28 + (x * 184U) / 240U), 138, 6);
        LCD_Draw_Line((uint16_t)(212 - (x * 184U) / 240U), 150, 212, 150, 1);
        LCD_Refresh(&cfg0);
        HAL_Delay(42);
    }

    for (x = 0; x < 4; x++) {
        LCD_Fill_Buffer(0);
        Game2_PrintCentered("FIREWALL", 70, 2, 3);
        Game2_PrintCentered("Enemy detected", 116, 6, 2);
        LCD_Refresh(&cfg0);
        HAL_Delay(130);

        LCD_Fill_Buffer(0);
        LCD_Draw_Rect(12, 58, 216, 124, 2, 0);
        LCD_Refresh(&cfg0);
        HAL_Delay(70);
    }

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(18, 70, 204, 92, 6, 0);
    Game2_PrintCentered("MISSION:", 84, 1, 2);
    Game2_PrintCentered("STEAL SIGNAL", 116, 6, 2);
    LCD_Refresh(&cfg0);
    HAL_Delay(700);
}

static void Game2_DrawPaused(void)
{
    char pause_text[24];
    uint32_t elapsed_ms = HAL_GetTick() - pause_start_tick;
    uint32_t remain_s = 0;

    if (elapsed_ms < PAUSE_EXIT_WINDOW_MS) {
        remain_s = (PAUSE_EXIT_WINDOW_MS - elapsed_ms + 999U) / 1000U;
    }

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(18, 42, 204, 150, 6, 0);
    LCD_printString("PAUSED", 66, 58, 1, 3);
    LCD_Draw_Line(44, 96, 196, 96, 1);
    LCD_printString("BT3 again", 58, 112, 6, 2);
    LCD_printString("returns menu", 44, 136, 6, 2);
    if (remain_s > 0) {
        sprintf(pause_text, "Auto resume %lus", remain_s);
        LCD_printString(pause_text, 42, 170, 1, 1);
    } else {
        LCD_printString("Resuming...", 70, 170, 1, 1);
    }
    LCD_Refresh(&cfg0);
}

static void Game2_DrawGameOver(void)
{
    char score_text[32];

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(18, 34, 204, 164, 1, 0);
    LCD_printString("MISSION", 52, 50, 1, 3);
    LCD_printString("END", 84, 82, 2, 3);
    sprintf(score_text, "Score: %u", PongEngine_GetScore(&signal_thief_engine));
    LCD_printString(score_text, 54, 124, 6, 2);
    LCD_Draw_Line(48, 154, 192, 154, 6);
    LCD_printString("BT2 Retry", 58, 166, 1, 1);
    LCD_printString("BT3 Menu", 62, 182, 5, 1);
    LCD_Refresh(&cfg0);
}

static void Game2_DrawVictory(void)
{
    char score_text[32];

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(18, 34, 204, 168, 6, 0);
    LCD_Draw_Rect(24, 40, 192, 156, 1, 0);
    Game2_PrintCentered("MISSION", 56, 1, 2);
    Game2_PrintCentered("SUCCESS", 82, 6, 2);
    LCD_Draw_Line(54, 112, 186, 112, 6);
    Game2_PrintCentered("Time Chest secured", 126, 1, 1);
    sprintf(score_text, "Score: %u", PongEngine_GetScore(&signal_thief_engine));
    Game2_PrintCentered(score_text, 146, 6, 1);
    LCD_Draw_Line(58, 166, 182, 166, 6);
    Game2_PrintCentered("BT2 PLAY AGAIN", 176, 1, 1);
    Game2_PrintCentered("BT3 MENU", 190, 5, 1);
    LCD_Refresh(&cfg0);
}

MenuState Game2_Run(void)
{
    Game2State state = GAME2_STATE_PLAYING;

    Input_Clear();
    Game2_Reset();
    Game2_ShowStartMenu();
    Game2_ShowIntro();
    Input_Read();
    Game2_PrimeButtons();

    while (1) {
        uint32_t frame_start = HAL_GetTick();
        uint8_t bt2_pressed = 0;
        uint8_t bt3_pressed = 0;
        uint8_t bt8_pressed = 0;
        uint8_t bt8_down = 0;

        Input_Read();
        bt2_pressed = Game2_BT2Pressed();
        bt3_pressed = Game2_BT3Pressed();
        bt8_pressed = Game2_BT8Pressed();
        bt8_down = Game2_PinDown(BTN8_GPIO_Port, BTN8_Pin);

        if (state == GAME2_STATE_GAME_OVER) {
            if (bt2_pressed) {
                buzzer_off(&buzzer_cfg);
                Game2_Reset();
                state = GAME2_STATE_PLAYING;
            } else if (bt3_pressed) {
                buzzer_off(&buzzer_cfg);
                LCD_Fill_Buffer(0);
                LCD_Refresh(&cfg0);
                Input_Clear();
                return MENU_STATE_HOME;
            }

            Game2_DrawGameOver();
        } else if (state == GAME2_STATE_VICTORY) {
            if (bt2_pressed) {
                buzzer_off(&buzzer_cfg);
                Game2_Reset();
                state = GAME2_STATE_PLAYING;
            } else if (bt3_pressed) {
                buzzer_off(&buzzer_cfg);
                LCD_Fill_Buffer(0);
                LCD_Refresh(&cfg0);
                Input_Clear();
                return MENU_STATE_HOME;
            }

            Game2_DrawVictory();
        } else if (state == GAME2_STATE_PAUSED) {
            if (!Game2_PinDown(BTN3_GPIO_Port, BTN3_Pin)) {
                pause_exit_armed = 1;
            }

            if (pause_exit_armed && bt3_pressed &&
                (HAL_GetTick() - pause_start_tick) <= PAUSE_EXIT_WINDOW_MS) {
                buzzer_off(&buzzer_cfg);
                LCD_Fill_Buffer(0);
                LCD_Refresh(&cfg0);
                Input_Clear();
                return MENU_STATE_HOME;
            }

            if ((HAL_GetTick() - pause_start_tick) > PAUSE_EXIT_WINDOW_MS) {
                state = GAME2_STATE_PLAYING;
                continue;
            }

            Game2_DrawPaused();
        } else {
            if (bt2_pressed) {
                PongEngine_UseUltimate(&signal_thief_engine);
            }
            if (bt3_pressed) {
                state = GAME2_STATE_PAUSED;
                pause_start_tick = HAL_GetTick();
                pause_exit_armed = 0;
            }

            if (state == GAME2_STATE_PLAYING) {
                Joystick_Read(&joystick_cfg, &joystick_data);
                Joystick_Read(&joystick2_cfg, &joystick2_data);
                {
                    UserInput input = Joystick_GetInput(&joystick_data);
                    UserInput aim_input = Joystick_GetInput(&joystick2_data);
                    PongEngine_HandleSecondStickButton(&signal_thief_engine,
                                                       aim_input,
                                                       bt8_down,
                                                       bt8_pressed);
                    uint8_t lives = PongEngine_UpdateDual(&signal_thief_engine, input, aim_input, 0);

                    LCD_Fill_Buffer(0);
                    PongEngine_Draw(&signal_thief_engine);
                    LCD_Refresh(&cfg0);

                    if (lives == 0) {
                        state = GAME2_STATE_GAME_OVER;
                    } else if (PongEngine_IsVictoryComplete(&signal_thief_engine)) {
                        state = GAME2_STATE_VICTORY;
                    }
                }
            }
        }

        {
            uint32_t frame_time = HAL_GetTick() - frame_start;
            if (frame_time < GAME2_FRAME_TIME_MS) {
                HAL_Delay(GAME2_FRAME_TIME_MS - frame_time);
            }
        }
    }
}
