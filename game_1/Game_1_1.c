#include "Game_1.h"
#include "TombGame.h"
#include "Buzzer.h"
#include "InputHandler.h"
#include "LCD.h"
#include "the_cursed_tomestm32.h"
#include "main.h"
#include "stm32l4xx_hal.h"

extern "C" {

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;

#define GAME1_FRAME_TIME_MS 16U
#define GAME1_EXIT_HOLD_MS 800U

static uint8_t Game1_PinDown(GPIO_TypeDef* port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

MenuState Game1_Run(void)
{
    uint32_t exit_hold_start = 0U;

    Input_Clear();
    TombGame::ClearReturnToMenu();
    CursedTomePort_Init();

    while (1) {
        uint32_t frame_start = HAL_GetTick();

        Input_Read();

        if (Game1_PinDown(BTN2_GPIO_Port, BTN2_Pin) && Game1_PinDown(BTN3_GPIO_Port, BTN3_Pin)) {
            if (exit_hold_start == 0U) {
                exit_hold_start = HAL_GetTick();
            } else if ((HAL_GetTick() - exit_hold_start) >= GAME1_EXIT_HOLD_MS) {
                break;
            }
        } else {
            exit_hold_start = 0U;
        }

        CursedTomePort_Tick();

        if (TombGame::ShouldReturnToMenu()) {
            break;
        }

        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME1_FRAME_TIME_MS) {
            HAL_Delay(GAME1_FRAME_TIME_MS - frame_time);
        }
    }

    CursedTomePort_Stop();
    buzzer_off(&buzzer_cfg);
    LCD_Fill_Buffer(0);
    LCD_Refresh(&cfg0);
    Input_Clear();

    return MENU_STATE_HOME;
}

void Game1_Init(void)
{
    TombGame::Init();
}

void Game1_Tick(void)
{
    TombGame::Tick();
}

void Game1_Draw(void)
{
    TombGame::Draw();
}

void Game1_RunFrame(void)
{
    Game1_Tick();
    Game1_Draw();
}

int Game1_IsInWorldView(void)
{
    return TombGame::GetState() == TombGame::State::InGame ? 1 : 0;
}

}
