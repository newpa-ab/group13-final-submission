#include "Game_3.h"

#include "AsteroidsEngine.h"
#include "AsteroidsAssets.h"
#include "Buzzer.h"
#include "InputHandler.h"
#include "Joystick.h"
#include "LCD.h"
#include "StrikerEngine.h"
#include "VersusEngine.h"
#include "main.h"
#include "stm32l4xx_hal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;
extern Joystick_cfg_t joystick2_cfg;
extern Joystick_t joystick2_data;

typedef enum {
    GAME3_STATE_MENU = 0,
    GAME3_STATE_SHIP_SELECT,
    GAME3_STATE_VERSUS_SHIP_SELECT_P1,
    GAME3_STATE_VERSUS_SHIP_SELECT_P2,
    GAME3_STATE_ASTEROIDS,
    GAME3_STATE_STRIKER,
    GAME3_STATE_VERSUS,
    GAME3_STATE_SUCCESS_ANIM,
    GAME3_STATE_GAME_OVER
} Game3State_t;

typedef enum {
    GAME3_MODE_ASTEROIDS = 0,
    GAME3_MODE_STRIKER,
    GAME3_MODE_VERSUS
} Game3Mode_t;

#define GAME3_FPS 60
#define GAME3_FRAME_TIME_MS (1000U / GAME3_FPS)
#define GAME3_RESULT_LOCK_MS 1200U
#define GAME3_SUCCESS_ANIM_MS 2600U

static AsteroidsEngine_t asteroids_engine;
static StrikerEngine_t striker_engine;
static VersusEngine_t versus_engine;
static Game3State_t game_state = GAME3_STATE_MENU;
static Game3Mode_t selected_mode = GAME3_MODE_ASTEROIDS;
static Game3Mode_t active_mode = GAME3_MODE_ASTEROIDS;
static SpaceShipType_t selected_ship = SHIP_RAIL;
static SpaceShipType_t versus_p1_ship = SHIP_RAIL;
static SpaceShipType_t versus_p2_ship = SHIP_SPREAD;
static AsteroidsDifficulty_t selected_difficulty = AST_DIFFICULTY_NORMAL;
static Direction last_menu_direction = CENTRE;
static uint16_t asteroids_high_score = 0;
static uint16_t striker_high_score = 0;
static uint32_t buzzer_stop_tick = 0;
static uint32_t led_stop_tick = 0;
static uint32_t damage_flash_until_tick = 0;
static uint32_t powerup_flash_until_tick = 0;
static uint32_t result_unlock_tick = 0;
static uint32_t success_anim_start_tick = 0;

static void start_beep(uint32_t freq_hz, uint16_t duration_ms);
static void service_feedback_timers(void);
static void update_menu_selection(Direction direction);
static void update_ship_selection(Direction direction);
static const char* mode_name(Game3Mode_t mode);
static const char* ship_name(SpaceShipType_t ship);
static const char* ship_trait(SpaceShipType_t ship);
static const char* ship_detail(SpaceShipType_t ship, Game3Mode_t mode);
static uint8_t max_lives_for_difficulty(AsteroidsDifficulty_t difficulty);
static uint8_t ship_accent(SpaceShipType_t ship);
static uint16_t text_width_px(const char* text, uint8_t font_size);
static void draw_centered_text(const char* text, uint16_t y, uint8_t colour, uint8_t font_size);
static void draw_simple_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t accent, uint8_t filled);
static void draw_mode_card(uint16_t x, uint16_t y, Game3Mode_t mode, uint8_t selected);
static void draw_ship_info(SpaceShipType_t ship, Game3Mode_t mode, uint8_t accent);
static void draw_hud_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t accent);
static void draw_health_bar(uint8_t lives, uint8_t max_lives);
static void draw_powerup_status(uint16_t weapon_timer, uint16_t magnet_timer, uint8_t shield);
static void draw_damage_flash(void);
static void draw_powerup_flash(void);
static void draw_star_icon(uint16_t x, uint16_t y, uint8_t colour);
static void draw_crown_icon(uint16_t x, uint16_t y, uint8_t colour);
static void draw_result_badge(uint16_t x, uint16_t y, uint8_t accent);
static void draw_mode_icon(uint16_t x, uint16_t y, Game3Mode_t mode, uint8_t accent);
static const char* solo_result_title(Game3Mode_t mode);
static void render_menu(void);
static void render_ship_select(void);
static void render_asteroids(void);
static void render_striker(void);
static void render_versus(void);
static void render_success_animation(void);
static void render_game_over(void);

MenuState Game3_Run(void)
{
    uint32_t last_tick = HAL_GetTick();

    AsteroidsEngine_Init(&asteroids_engine);
    AsteroidsEngine_SetDifficulty(&asteroids_engine, selected_difficulty);
    StrikerEngine_Init(&striker_engine);
    VersusEngine_Init(&versus_engine);
    game_state = GAME3_STATE_MENU;
    last_menu_direction = CENTRE;
    buzzer_stop_tick = 0;
    led_stop_tick = 0;
    damage_flash_until_tick = 0;
    powerup_flash_until_tick = 0;
    result_unlock_tick = 0;
    success_anim_start_tick = 0;
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    while (1) {
        uint32_t now = HAL_GetTick();
        uint8_t fire_pressed;
        uint8_t alt_pressed;
        uint8_t p2_fire_pressed;
        uint8_t p2_alt_pressed;
        UserInput input;
        UserInput input2;

        if ((now - last_tick) < GAME3_FRAME_TIME_MS) {
            service_feedback_timers();
            continue;
        }

        last_tick = now;

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);
        Joystick_Read(&joystick2_cfg, &joystick2_data);
        input = Joystick_GetInput(&joystick_data);
        input2 = Joystick_GetInput(&joystick2_data);

        fire_pressed = current_input.btn2_pressed;
        alt_pressed = current_input.btn3_pressed;
        p2_fire_pressed = current_input.btn9_pressed;
        p2_alt_pressed = current_input.btn8_pressed;

        switch (game_state) {
        case GAME3_STATE_MENU:
            update_menu_selection(joystick_data.direction);

            if (fire_pressed) {
                active_mode = selected_mode;
                if (active_mode == GAME3_MODE_VERSUS) {
                    selected_ship = versus_p1_ship;
                    game_state = GAME3_STATE_VERSUS_SHIP_SELECT_P1;
                } else {
                    game_state = GAME3_STATE_SHIP_SELECT;
                }
                start_beep(900, 80);
            } else if (alt_pressed) {
                buzzer_off(&buzzer_cfg);
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
                return MENU_STATE_HOME;
            }
            render_menu();
            break;

        case GAME3_STATE_SHIP_SELECT:
            update_ship_selection(joystick_data.direction);

            if (fire_pressed) {
                if (active_mode == GAME3_MODE_STRIKER) {
                    StrikerEngine_SetShipType(&striker_engine, selected_ship);
                    StrikerEngine_StartGame(&striker_engine);
                    game_state = GAME3_STATE_STRIKER;
                } else {
                    AsteroidsEngine_SetShipType(&asteroids_engine, selected_ship);
                    AsteroidsEngine_SetDifficulty(&asteroids_engine, AST_DIFFICULTY_NORMAL);
                    AsteroidsEngine_StartGame(&asteroids_engine);
                    game_state = GAME3_STATE_ASTEROIDS;
                }
                start_beep(980, 80);
            } else if (alt_pressed) {
                game_state = GAME3_STATE_MENU;
                start_beep(520, 60);
            }
            render_ship_select();
            break;

        case GAME3_STATE_VERSUS_SHIP_SELECT_P1:
            update_ship_selection(joystick_data.direction);

            if (fire_pressed) {
                versus_p1_ship = selected_ship;
                selected_ship = versus_p2_ship;
                game_state = GAME3_STATE_VERSUS_SHIP_SELECT_P2;
                start_beep(900, 70);
            } else if (alt_pressed) {
                game_state = GAME3_STATE_MENU;
                start_beep(520, 60);
            }
            render_ship_select();
            break;

        case GAME3_STATE_VERSUS_SHIP_SELECT_P2:
            update_ship_selection(joystick2_data.direction);

            if (p2_fire_pressed || fire_pressed) {
                versus_p2_ship = selected_ship;
                VersusEngine_SetShips(&versus_engine, versus_p1_ship, versus_p2_ship);
                VersusEngine_StartGame(&versus_engine);
                game_state = GAME3_STATE_VERSUS;
                start_beep(980, 80);
            } else if (p2_alt_pressed || alt_pressed) {
                selected_ship = versus_p1_ship;
                game_state = GAME3_STATE_VERSUS_SHIP_SELECT_P1;
                start_beep(520, 60);
            }
            render_ship_select();
            break;

        case GAME3_STATE_ASTEROIDS:
        {
            uint8_t events = AsteroidsEngine_Update(&asteroids_engine, input, fire_pressed, alt_pressed);

            if (events & AST_EVENT_FIRE) {
                start_beep(1600, 35);
            }
            if (events & AST_EVENT_ROCK_HIT) {
                start_beep(1000, 60);
            }
            if (events & AST_EVENT_SHIP_HIT) {
                start_beep(240, 180);
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
                led_stop_tick = HAL_GetTick() + 200;
                damage_flash_until_tick = HAL_GetTick() + 260;
            }
            if (events & AST_EVENT_SHIELD_BLOCK) {
                start_beep(520, 110);
                damage_flash_until_tick = HAL_GetTick() + 160;
            }
            if (events & AST_EVENT_POWERUP) {
                start_beep(1350, 95);
                powerup_flash_until_tick = HAL_GetTick() + 220U;
            }
            if (events & AST_EVENT_WAVE_CLEAR) {
                start_beep(1300, 120);
            }

            if (AsteroidsEngine_IsGameOver(&asteroids_engine)) {
                uint16_t score = AsteroidsEngine_GetScore(&asteroids_engine);
                if (score > asteroids_high_score) {
                    asteroids_high_score = score;
                }
                game_state = GAME3_STATE_GAME_OVER;
                result_unlock_tick = HAL_GetTick() + GAME3_RESULT_LOCK_MS;
                start_beep(180, 300);
            }

            render_asteroids();
            break;
        }

        case GAME3_STATE_STRIKER:
        {
            uint8_t events = StrikerEngine_Update(&striker_engine, input, fire_pressed);

            if (events & STRIKER_EVENT_FIRE) {
                start_beep(1600, 28);
            }
            if (events & STRIKER_EVENT_ENEMY_HIT) {
                start_beep(1050, 45);
            }
            if (events & STRIKER_EVENT_PLAYER_HIT) {
                start_beep(240, 160);
                HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
                led_stop_tick = HAL_GetTick() + 180;
                damage_flash_until_tick = HAL_GetTick() + 240;
            }
            if (events & STRIKER_EVENT_SHIELD_BLOCK) {
                start_beep(520, 100);
                damage_flash_until_tick = HAL_GetTick() + 140;
            }
            if (events & STRIKER_EVENT_POWERUP) {
                start_beep(1350, 90);
                powerup_flash_until_tick = HAL_GetTick() + 220U;
            }
            if (events & STRIKER_EVENT_WAVE) {
                start_beep(1300, 100);
            }

            if (StrikerEngine_IsGameOver(&striker_engine)) {
                uint16_t score = StrikerEngine_GetScore(&striker_engine);
                if (score > striker_high_score) {
                    striker_high_score = score;
                }
                if (StrikerEngine_IsMissionComplete(&striker_engine)) {
                    game_state = GAME3_STATE_SUCCESS_ANIM;
                    success_anim_start_tick = HAL_GetTick();
                    start_beep(1300, 180);
                } else {
                    game_state = GAME3_STATE_GAME_OVER;
                    result_unlock_tick = HAL_GetTick() + GAME3_RESULT_LOCK_MS;
                    start_beep(180, 300);
                }
            }

            render_striker();
            break;
        }

        case GAME3_STATE_VERSUS:
        {
            uint8_t events = VersusEngine_Update(&versus_engine,
                                                 input,
                                                 input2,
                                                 fire_pressed,
                                                 p2_fire_pressed,
                                                 alt_pressed,
                                                 p2_alt_pressed);

            if (events & (VERSUS_EVENT_P1_FIRE | VERSUS_EVENT_P2_FIRE)) {
                start_beep(1500, 28);
            }
            if (events & VERSUS_EVENT_HIT) {
                start_beep(300, 110);
                damage_flash_until_tick = HAL_GetTick() + 170U;
            }
            if (events & VERSUS_EVENT_SHIELD_BLOCK) {
                start_beep(560, 80);
            }
            if (events & VERSUS_EVENT_POWERUP) {
                start_beep(1350, 90);
                powerup_flash_until_tick = HAL_GetTick() + 180U;
            }
            if (events & VERSUS_EVENT_DASH) {
                start_beep(900, 35);
            }
            if (VersusEngine_IsGameOver(&versus_engine)) {
                game_state = GAME3_STATE_GAME_OVER;
                result_unlock_tick = HAL_GetTick() + GAME3_RESULT_LOCK_MS;
                start_beep(180, 300);
            }

            render_versus();
            break;
        }

        case GAME3_STATE_SUCCESS_ANIM:
            if ((int32_t)(HAL_GetTick() - (success_anim_start_tick + GAME3_SUCCESS_ANIM_MS)) >= 0) {
                game_state = GAME3_STATE_GAME_OVER;
                result_unlock_tick = HAL_GetTick() + GAME3_RESULT_LOCK_MS;
                start_beep(720, 80);
            }
            render_success_animation();
            break;

        case GAME3_STATE_GAME_OVER:
            if ((int32_t)(HAL_GetTick() - result_unlock_tick) >= 0 &&
                (fire_pressed || alt_pressed || p2_fire_pressed || p2_alt_pressed)) {
                game_state = GAME3_STATE_MENU;
                start_beep(700, 70);
            }
            render_game_over();
            break;

        default:
            game_state = GAME3_STATE_MENU;
            break;
        }

        service_feedback_timers();
    }
}

static void start_beep(uint32_t freq_hz, uint16_t duration_ms)
{
    buzzer_tone(&buzzer_cfg, freq_hz, 50);
    buzzer_stop_tick = HAL_GetTick() + duration_ms;
}

static void service_feedback_timers(void)
{
    if (buzzer_stop_tick != 0 && (int32_t)(HAL_GetTick() - buzzer_stop_tick) >= 0) {
        buzzer_off(&buzzer_cfg);
        buzzer_stop_tick = 0;
    }

    if (led_stop_tick != 0 && (int32_t)(HAL_GetTick() - led_stop_tick) >= 0) {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        led_stop_tick = 0;
    }
}

static void update_menu_selection(Direction direction)
{
    if (direction == N && last_menu_direction != N) {
        selected_mode = (selected_mode == GAME3_MODE_ASTEROIDS) ? GAME3_MODE_VERSUS : (Game3Mode_t)(selected_mode - 1U);
        start_beep(720, 35);
    } else if (direction == S && last_menu_direction != S) {
        selected_mode = (selected_mode == GAME3_MODE_VERSUS) ? GAME3_MODE_ASTEROIDS : (Game3Mode_t)(selected_mode + 1U);
        start_beep(840, 35);
    }

    last_menu_direction = direction;
}

static void update_ship_selection(Direction direction)
{
    if (direction == W && last_menu_direction != W) {
        selected_ship = (selected_ship == SHIP_RAIL) ? SHIP_LANCE : (SpaceShipType_t)(selected_ship - 1U);
        start_beep(650, 35);
    } else if (direction == E && last_menu_direction != E) {
        selected_ship = (selected_ship == SHIP_LANCE) ? SHIP_RAIL : (SpaceShipType_t)(selected_ship + 1U);
        start_beep(760, 35);
    }

    last_menu_direction = direction;
}

static const char* mode_name(Game3Mode_t mode)
{
    if (mode == GAME3_MODE_STRIKER) {
        return "STRIKER";
    }
    if (mode == GAME3_MODE_VERSUS) {
        return "VERSUS";
    }
    return "ASTEROIDS";
}

static const char* ship_name(SpaceShipType_t ship)
{
    switch (ship) {
    case SHIP_SPREAD:
        return "SPREAD";
    case SHIP_LANCE:
        return "LANCE";
    case SHIP_RAIL:
    default:
        return "RAIL";
    }
}

static const char* ship_trait(SpaceShipType_t ship)
{
    switch (ship) {
    case SHIP_SPREAD:
        return "Wide multi-shot";
    case SHIP_LANCE:
        return "Piercing lance";
    case SHIP_RAIL:
    default:
        return "High damage beam";
    }
}

static const char* ship_detail(SpaceShipType_t ship, Game3Mode_t mode)
{
    if (mode == GAME3_MODE_ASTEROIDS) {
        switch (ship) {
        case SHIP_SPREAD:
            return "Close-range rocks";
        case SHIP_LANCE:
            return "Hits many rocks";
        case SHIP_RAIL:
        default:
            return "Fast rock breaker";
        }
    }
    if (mode == GAME3_MODE_VERSUS) {
        switch (ship) {
        case SHIP_SPREAD:
            return "Wide pressure";
        case SHIP_LANCE:
            return "Long poke";
        case SHIP_RAIL:
        default:
            return "Duel burst";
        }
    }

    switch (ship) {
    case SHIP_SPREAD:
        return "Best for crowds";
    case SHIP_LANCE:
        return "Cuts enemy lines";
    case SHIP_RAIL:
    default:
        return "Best for bosses";
    }
}

static uint8_t max_lives_for_difficulty(AsteroidsDifficulty_t difficulty)
{
    switch (difficulty) {
    case AST_DIFFICULTY_EASY:
        return 4;
    case AST_DIFFICULTY_HARD:
        return 2;
    case AST_DIFFICULTY_NORMAL:
    default:
        return 3;
    }
}

static uint8_t ship_accent(SpaceShipType_t ship)
{
    switch (ship) {
    case SHIP_SPREAD:
        return 14U;
    case SHIP_LANCE:
        return 11U;
    case SHIP_RAIL:
    default:
        return 2U;
    }
}

static uint16_t text_width_px(const char* text, uint8_t font_size)
{
    return (uint16_t)(strlen(text) * 6U * font_size);
}

static void draw_centered_text(const char* text, uint16_t y, uint8_t colour, uint8_t font_size)
{
    uint16_t width = text_width_px(text, font_size);
    uint16_t x = (width >= 240U) ? 0U : (uint16_t)((240U - width) / 2U);
    LCD_printString(text, x, y, colour, font_size);
}

static void draw_simple_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t accent, uint8_t filled)
{
    if (filled) {
        LCD_Draw_Rect((uint16_t)(x + 1U), (uint16_t)(y + 1U), (uint16_t)(width - 2U), (uint16_t)(height - 2U), 9, 1);
    }

    LCD_Draw_Rect(x, y, width, height, 13, 0);
    LCD_Draw_Line(x, y, (uint16_t)(x + 22U), y, accent);
    LCD_Draw_Line(x, y, x, (uint16_t)(y + 12U), accent);
    LCD_Draw_Line((uint16_t)(x + width - 23U), (uint16_t)(y + height - 1U),
                  (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U), accent);
    LCD_Draw_Line((uint16_t)(x + width - 1U), (uint16_t)(y + height - 13U),
                  (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U), accent);
}

static void draw_mode_card(uint16_t x, uint16_t y, Game3Mode_t mode, uint8_t selected)
{
    uint8_t accent = (mode == GAME3_MODE_ASTEROIDS) ? 14U : ((mode == GAME3_MODE_STRIKER) ? 11U : 2U);
    const char* title = (mode == GAME3_MODE_ASTEROIDS) ? "ASTEROIDS" : ((mode == GAME3_MODE_STRIKER) ? "STRIKER" : "VERSUS");
    const char* subtitle = (mode == GAME3_MODE_ASTEROIDS) ? "Free flight" : ((mode == GAME3_MODE_STRIKER) ? "Escape mission" : "Two-player duel");

    draw_simple_panel(x, y, 192, 42, selected ? accent : 13U, 1U);
    draw_mode_icon((uint16_t)(x + 24U), (uint16_t)(y + 21U), mode, selected ? accent : 13U);
    LCD_printString(selected ? ">" : " ", (uint16_t)(x + 12U), (uint16_t)(y + 11U), selected ? accent : 13U, 1);
    LCD_printString(title, (uint16_t)(x + 48U), (uint16_t)(y + 8U), selected ? 1U : 13U, 2);
    LCD_printString(subtitle, (uint16_t)(x + 48U), (uint16_t)(y + 28U), 13U, 1);
}

static void draw_ship_info(SpaceShipType_t ship, Game3Mode_t mode, uint8_t accent)
{
    const char* role = (ship == SHIP_RAIL) ? "Boss damage" : ((ship == SHIP_SPREAD) ? "Crowd clear" : "Line pierce");

    draw_simple_panel(22, 172, 196, 40, accent, 1U);
    LCD_printString("WEAPON:", 32, 181, 13, 1);
    LCD_printString(ship_trait(ship), 82, 181, 1, 1);
    LCD_printString("ROLE:", 32, 193, 13, 1);
    LCD_printString(role, 82, 193, 1, 1);
    LCD_printString("BEST:", 32, 205, 13, 1);
    LCD_printString(ship_detail(ship, mode), 82, 205, accent, 1);
}

static void draw_hud_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t accent)
{
    uint16_t right = (uint16_t)(x + width - 1U);
    uint16_t bottom = (uint16_t)(y + height - 1U);
    uint16_t notch = (width < 52U) ? 5U : 7U;

    LCD_Draw_Rect((uint16_t)(x + 1U), (uint16_t)(y + 1U), (uint16_t)(width - 2U), (uint16_t)(height - 2U), 9, 1);
    LCD_Draw_Rect((uint16_t)(x + notch), y, (uint16_t)(width - (notch * 2U)), height, 13, 0);
    LCD_Draw_Rect(x, (uint16_t)(y + notch), width, (uint16_t)(height - (notch * 2U)), 13, 0);

    LCD_Draw_Line((uint16_t)(x + notch), y, x, (uint16_t)(y + notch), 13);
    LCD_Draw_Line((uint16_t)(right - notch), y, right, (uint16_t)(y + notch), 13);
    LCD_Draw_Line(x, (uint16_t)(bottom - notch), (uint16_t)(x + notch), bottom, 13);
    LCD_Draw_Line(right, (uint16_t)(bottom - notch), (uint16_t)(right - notch), bottom, 13);

    LCD_Draw_Line((uint16_t)(x + notch + 1U), (uint16_t)(y + 2U),
                  (uint16_t)(right - notch - 1U), (uint16_t)(y + 2U), 0);
    LCD_Draw_Line((uint16_t)(x + notch + 2U), (uint16_t)(y + 3U),
                  (uint16_t)(right - notch - 2U), (uint16_t)(y + 3U), 0);

    LCD_Draw_Line((uint16_t)(x + notch + 2U), y, (uint16_t)(x + notch + 17U), y, accent);
    LCD_Draw_Line((uint16_t)(right - notch - 17U), y, (uint16_t)(right - notch - 2U), y, accent);
    LCD_Draw_Line((uint16_t)(x + notch + 2U), bottom, (uint16_t)(x + notch + 14U), bottom, accent);
    LCD_Draw_Line((uint16_t)(right - notch - 14U), bottom, (uint16_t)(right - notch - 2U), bottom, accent);

    LCD_Draw_Line(x, (uint16_t)(y + notch + 2U), x, (uint16_t)(bottom - notch - 2U), accent);
    LCD_Draw_Line(right, (uint16_t)(y + notch + 2U), right, (uint16_t)(bottom - notch - 2U), accent);

    LCD_Set_Pixel((uint16_t)(x + notch + 3U), (uint16_t)(y + 4U), 1);
    LCD_Set_Pixel((uint16_t)(right - notch - 3U), (uint16_t)(y + 4U), 1);
    LCD_Set_Pixel((uint16_t)(x + 2U), (uint16_t)(bottom - notch), 0);
    LCD_Set_Pixel((uint16_t)(right - 2U), (uint16_t)(bottom - notch), 0);
}

static void draw_health_bar(uint8_t lives, uint8_t max_lives)
{
    const uint16_t icon_x = 9;
    const uint16_t icon_y = 8;
    const uint16_t bar_x = 26;
    const uint16_t bar_y = 10;
    const uint16_t bar_w = 54;
    const uint16_t bar_h = 9;
    const uint16_t fill_max = bar_w - 4U;
    uint16_t fill_w;
    uint8_t fill_colour;
    uint8_t border_colour;
    uint8_t i;

    if (max_lives == 0U) {
        max_lives = 1U;
    }
    if (lives > max_lives) {
        lives = max_lives;
    }

    fill_w = (uint16_t)(((uint16_t)lives * fill_max) / max_lives);
    if (lives <= 1U && ((HAL_GetTick() / 140U) & 1U) != 0U) {
        fill_colour = 1U;
        border_colour = 2U;
    } else {
        fill_colour = (lives <= 1U) ? 2U : ((lives < max_lives) ? 6U : 14U);
        border_colour = 13U;
    }

    draw_hud_panel(4, 4, 82, 21, border_colour);

    LCD_Draw_Line((uint16_t)(icon_x + 5U), icon_y, (uint16_t)(icon_x + 10U), (uint16_t)(icon_y + 3U), border_colour);
    LCD_Draw_Line((uint16_t)(icon_x + 10U), (uint16_t)(icon_y + 3U), (uint16_t)(icon_x + 8U), (uint16_t)(icon_y + 11U), border_colour);
    LCD_Draw_Line((uint16_t)(icon_x + 8U), (uint16_t)(icon_y + 11U), (uint16_t)(icon_x + 5U), (uint16_t)(icon_y + 14U), border_colour);
    LCD_Draw_Line((uint16_t)(icon_x + 5U), (uint16_t)(icon_y + 14U), (uint16_t)(icon_x + 2U), (uint16_t)(icon_y + 11U), border_colour);
    LCD_Draw_Line((uint16_t)(icon_x + 2U), (uint16_t)(icon_y + 11U), icon_x, (uint16_t)(icon_y + 3U), border_colour);
    LCD_Draw_Line(icon_x, (uint16_t)(icon_y + 3U), (uint16_t)(icon_x + 5U), icon_y, border_colour);
    LCD_Draw_Rect((uint16_t)(icon_x + 3U), (uint16_t)(icon_y + 4U), 5, 6, fill_colour, 1);
    LCD_Set_Pixel((uint16_t)(icon_x + 5U), (uint16_t)(icon_y + 2U), 1);

    LCD_Draw_Rect(bar_x, bar_y, bar_w, bar_h, border_colour, 0);
    LCD_Draw_Rect((uint16_t)(bar_x + 1U), (uint16_t)(bar_y + 1U), (uint16_t)(bar_w - 2U), (uint16_t)(bar_h - 2U), 9, 1);
    LCD_Draw_Line((uint16_t)(bar_x + 2U), (uint16_t)(bar_y + bar_h - 3U),
                  (uint16_t)(bar_x + bar_w - 3U), (uint16_t)(bar_y + bar_h - 3U), 0);
    if (fill_w > 0U) {
        LCD_Draw_Rect((uint16_t)(bar_x + 2U), (uint16_t)(bar_y + 2U), fill_w, (uint16_t)(bar_h - 4U), fill_colour, 1);
        LCD_Draw_Line((uint16_t)(bar_x + 2U), (uint16_t)(bar_y + 2U), (uint16_t)(bar_x + 1U + fill_w), (uint16_t)(bar_y + 2U), 1);
        if (fill_w > 8U) {
            LCD_Set_Pixel((uint16_t)(bar_x + 4U), (uint16_t)(bar_y + 3U), 1);
            LCD_Set_Pixel((uint16_t)(bar_x + 5U), (uint16_t)(bar_y + 3U), 1);
        }
    }

    for (i = 1U; i < max_lives && max_lives <= 8U; i++) {
        uint16_t tick_x = (uint16_t)(bar_x + 2U + (((uint16_t)i * fill_max) / max_lives));
        LCD_Draw_Line(tick_x, (uint16_t)(bar_y + 2U), tick_x, (uint16_t)(bar_y + bar_h - 3U), 0);
        LCD_Set_Pixel(tick_x, (uint16_t)(bar_y + 1U), border_colour);
    }
}

static void draw_powerup_status(uint16_t weapon_timer, uint16_t magnet_timer, uint8_t shield)
{
    uint16_t weapon_w = (uint16_t)((weapon_timer > 600U) ? 24U : ((weapon_timer * 24U) / 600U));
    uint16_t magnet_w = (uint16_t)((magnet_timer > 600U) ? 20U : ((magnet_timer * 20U) / 600U));

    draw_simple_panel(6, 218, 56, 17, weapon_timer > 0U ? 14U : 13U, 1U);
    LCD_printString("2X", 13, 224, weapon_timer > 0U ? 1U : 13U, 1);
    LCD_Draw_Rect(31, 225, 24, 4, 13, 0);
    if (weapon_w > 0U) {
        LCD_Draw_Rect(32, 226, weapon_w, 2, 14, 1);
    }

    draw_simple_panel(70, 218, 70, 17, magnet_timer > 0U ? 11U : 13U, 1U);
    LCD_printString("MAG", 77, 224, magnet_timer > 0U ? 1U : 13U, 1);
    LCD_Draw_Rect(106, 225, 20, 4, 13, 0);
    if (magnet_w > 0U) {
        LCD_Draw_Rect(107, 226, magnet_w, 2, 11, 1);
    }

    draw_simple_panel(180, 218, 54, 17, shield ? 14U : 13U, 1U);
    LCD_printString("SHD", 194, 224, shield ? 1U : 13U, 1);
}

static void draw_damage_flash(void)
{
    uint32_t now = HAL_GetTick();

    if (damage_flash_until_tick == 0U || (int32_t)(now - damage_flash_until_tick) >= 0) {
        return;
    }

    if (((now / 45U) & 1U) != 0U) {
        return;
    }

    LCD_Draw_Rect(0, 0, 240, 240, 2, 0);
    LCD_Draw_Rect(1, 1, 238, 238, 2, 0);
    LCD_Draw_Line(0, 0, 18, 0, 1);
    LCD_Draw_Line(0, 0, 0, 18, 1);
    LCD_Draw_Line(239, 0, 221, 0, 1);
    LCD_Draw_Line(239, 0, 239, 18, 1);
    LCD_Draw_Line(0, 239, 18, 239, 1);
    LCD_Draw_Line(0, 239, 0, 221, 1);
    LCD_Draw_Line(239, 239, 221, 239, 1);
    LCD_Draw_Line(239, 239, 239, 221, 1);
}

static void draw_powerup_flash(void)
{
    uint32_t now = HAL_GetTick();

    if (powerup_flash_until_tick == 0U || (int32_t)(now - powerup_flash_until_tick) >= 0) {
        return;
    }

    if (((now / 55U) & 1U) != 0U) {
        return;
    }

    LCD_Draw_Rect(2, 2, 236, 236, 14, 0);
    LCD_Draw_Rect(3, 3, 234, 234, 1, 0);
    LCD_Draw_Line(20, 3, 58, 3, 14);
    LCD_Draw_Line(182, 3, 220, 3, 14);
    LCD_Draw_Line(20, 236, 58, 236, 14);
    LCD_Draw_Line(182, 236, 220, 236, 14);
}

static void draw_star_icon(uint16_t x, uint16_t y, uint8_t colour)
{
    LCD_Draw_Line(x, (uint16_t)(y - 5U), x, (uint16_t)(y + 5U), colour);
    LCD_Draw_Line((uint16_t)(x - 5U), y, (uint16_t)(x + 5U), y, colour);
    LCD_Draw_Line((uint16_t)(x - 3U), (uint16_t)(y - 3U), (uint16_t)(x + 3U), (uint16_t)(y + 3U), colour);
    LCD_Draw_Line((uint16_t)(x + 3U), (uint16_t)(y - 3U), (uint16_t)(x - 3U), (uint16_t)(y + 3U), colour);
    LCD_Set_Pixel(x, y, 1);
}

static void draw_crown_icon(uint16_t x, uint16_t y, uint8_t colour)
{
    LCD_Draw_Line(x, (uint16_t)(y + 9U), (uint16_t)(x + 28U), (uint16_t)(y + 9U), colour);
    LCD_Draw_Line(x, (uint16_t)(y + 9U), (uint16_t)(x + 5U), y, colour);
    LCD_Draw_Line((uint16_t)(x + 5U), y, (uint16_t)(x + 12U), (uint16_t)(y + 8U), colour);
    LCD_Draw_Line((uint16_t)(x + 12U), (uint16_t)(y + 8U), (uint16_t)(x + 14U), (uint16_t)(y - 2U), colour);
    LCD_Draw_Line((uint16_t)(x + 14U), (uint16_t)(y - 2U), (uint16_t)(x + 17U), (uint16_t)(y + 8U), colour);
    LCD_Draw_Line((uint16_t)(x + 17U), (uint16_t)(y + 8U), (uint16_t)(x + 24U), y, colour);
    LCD_Draw_Line((uint16_t)(x + 24U), y, (uint16_t)(x + 28U), (uint16_t)(y + 9U), colour);
    LCD_Set_Pixel((uint16_t)(x + 5U), y, 1);
    LCD_Set_Pixel((uint16_t)(x + 14U), (uint16_t)(y - 2U), 1);
    LCD_Set_Pixel((uint16_t)(x + 24U), y, 1);
}

static void draw_result_badge(uint16_t x, uint16_t y, uint8_t accent)
{
    LCD_Draw_Circle(x, y, 15, accent, 0);
    LCD_Draw_Circle(x, y, 12, 13, 0);
    draw_star_icon(x, y, accent);
    LCD_Draw_Line((uint16_t)(x - 6U), (uint16_t)(y + 15U), (uint16_t)(x - 12U), (uint16_t)(y + 28U), 2);
    LCD_Draw_Line((uint16_t)(x + 6U), (uint16_t)(y + 15U), (uint16_t)(x + 12U), (uint16_t)(y + 28U), 11);
}

static void draw_mode_icon(uint16_t x, uint16_t y, Game3Mode_t mode, uint8_t accent)
{
    if (mode == GAME3_MODE_ASTEROIDS) {
        LCD_Draw_Circle(x, y, 8, accent, 0);
        LCD_Draw_Circle((uint16_t)(x - 3U), (uint16_t)(y - 2U), 2, 13, 0);
        LCD_Draw_Circle((uint16_t)(x + 4U), (uint16_t)(y + 3U), 2, 13, 0);
        LCD_Set_Pixel((uint16_t)(x + 2U), (uint16_t)(y - 5U), 1);
        draw_star_icon((uint16_t)(x + 14U), (uint16_t)(y - 8U), 1U);
        return;
    }

    if (mode == GAME3_MODE_STRIKER) {
        LCD_Draw_Line(x, (uint16_t)(y - 10U), (uint16_t)(x - 9U), (uint16_t)(y + 8U), accent);
        LCD_Draw_Line(x, (uint16_t)(y - 10U), (uint16_t)(x + 9U), (uint16_t)(y + 8U), accent);
        LCD_Draw_Line((uint16_t)(x - 9U), (uint16_t)(y + 8U), x, (uint16_t)(y + 3U), accent);
        LCD_Draw_Line((uint16_t)(x + 9U), (uint16_t)(y + 8U), x, (uint16_t)(y + 3U), accent);
        LCD_Draw_Line(x, (uint16_t)(y - 7U), x, (uint16_t)(y + 9U), 1U);
        LCD_Draw_Line((uint16_t)(x - 4U), (uint16_t)(y + 12U), (uint16_t)(x - 2U), (uint16_t)(y + 17U), 14U);
        LCD_Draw_Line((uint16_t)(x + 4U), (uint16_t)(y + 12U), (uint16_t)(x + 2U), (uint16_t)(y + 17U), 14U);
        return;
    }

    LCD_Draw_Line((uint16_t)(x - 14U), (uint16_t)(y + 7U), (uint16_t)(x - 5U), (uint16_t)(y - 5U), 14U);
    LCD_Draw_Line((uint16_t)(x - 5U), (uint16_t)(y - 5U), (uint16_t)(x - 2U), (uint16_t)(y + 7U), 14U);
    LCD_Draw_Line((uint16_t)(x + 14U), (uint16_t)(y - 7U), (uint16_t)(x + 5U), (uint16_t)(y + 5U), 2U);
    LCD_Draw_Line((uint16_t)(x + 5U), (uint16_t)(y + 5U), (uint16_t)(x + 2U), (uint16_t)(y - 7U), 2U);
    LCD_Set_Pixel((uint16_t)(x - 1U), y, 1U);
    LCD_Set_Pixel(x, y, 1U);
    LCD_Set_Pixel((uint16_t)(x + 1U), y, 1U);
    LCD_Draw_Line((uint16_t)(x - 18U), (uint16_t)(y - 10U), (uint16_t)(x + 18U), (uint16_t)(y + 10U), 13U);
}

static const char* solo_result_title(Game3Mode_t mode)
{
    if (mode == GAME3_MODE_STRIKER) {
        if (StrikerEngine_IsMissionComplete(&striker_engine)) {
            return "SIGNAL ESCAPED";
        }
        if (StrikerEngine_GetSignalCore(&striker_engine) == 0U) {
            return "SIGNAL LOST";
        }
        return "SHIP DOWN";
    }

    if (AsteroidsEngine_GetMaxWaveReached(&asteroids_engine) >= 4U) {
        return "ROCK HUNTER";
    }
    if (AsteroidsEngine_GetRocksDestroyed(&asteroids_engine) >= 20U) {
        return "SHARP SHOT";
    }
    return "TRY AGAIN";
}

static void render_menu(void)
{
    char best_str[24];

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(4, 4, 232, 232, 9, 0);
    draw_centered_text("SPACE OPS", 10, 14, 3);
    draw_centered_text("SELECT MISSION", 38, 13, 1);

    draw_mode_card(24, 54, GAME3_MODE_ASTEROIDS, selected_mode == GAME3_MODE_ASTEROIDS);
    draw_mode_card(24, 101, GAME3_MODE_STRIKER, selected_mode == GAME3_MODE_STRIKER);
    draw_mode_card(24, 148, GAME3_MODE_VERSUS, selected_mode == GAME3_MODE_VERSUS);

    sprintf(best_str, "A BEST:%d", asteroids_high_score);
    draw_simple_panel(18, 195, 96, 18, 14, 1U);
    LCD_printString(best_str, 28, 201, 1, 1);
    sprintf(best_str, "S BEST:%d", striker_high_score);
    draw_simple_panel(126, 195, 96, 18, 11, 1U);
    LCD_printString(best_str, 136, 201, 1, 1);

    draw_centered_text("JOY UP/DOWN", 216, 1, 1);
    LCD_printString("BTN2 NEXT", 42, 222, 14, 1);
    LCD_printString("BTN3 BACK", 142, 222, 13, 1);
    LCD_Refresh(&cfg0);
}

static void render_ship_select(void)
{
    char line[32];
    uint8_t accent = ship_accent(selected_ship);

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(4, 4, 232, 232, 9, 0);
    if (game_state == GAME3_STATE_VERSUS_SHIP_SELECT_P1) {
        draw_centered_text("P1 SELECT", 14, 14, 3);
        sprintf(line, "VERSUS MODE");
    } else if (game_state == GAME3_STATE_VERSUS_SHIP_SELECT_P2) {
        draw_centered_text("P2 SELECT", 14, 2, 3);
        sprintf(line, "VERSUS MODE");
    } else {
        draw_centered_text("SELECT SHIP", 14, accent, 3);
        sprintf(line, "%s MODE", mode_name(active_mode));
    }
    draw_centered_text(line, 42, 13, 1);

    draw_simple_panel(72, 62, 96, 82, accent, 1U);
    LCD_Draw_Sprite(96, 78,
                    STRIKER_PLAYER_SHIP_H,
                    STRIKER_PLAYER_SHIP_W,
                    STRIKER_PLAYER_SHIPS[selected_ship]);
    LCD_printString("<", 38, 96, 13, 3);
    LCD_printString(">", 190, 96, 13, 3);

    sprintf(line, "%s", ship_name(selected_ship));
    draw_centered_text(line, 152, accent, 2);
    draw_ship_info(selected_ship, active_mode, accent);

    if (game_state == GAME3_STATE_VERSUS_SHIP_SELECT_P2) {
        LCD_printString("P2 L/R", 42, 222, 13, 1);
        LCD_printString("BTN9 START", 104, 222, 2, 1);
        draw_centered_text("BTN8 BACK", 233, 13, 1);
    } else {
        LCD_printString("JOY L/R", 44, 222, 13, 1);
        LCD_printString("BTN2 START", 104, 222, 14, 1);
        draw_centered_text("BTN3 BACK", 233, 13, 1);
    }
    LCD_Refresh(&cfg0);
}

static void render_asteroids(void)
{
    char info_str[40];

    LCD_Fill_Buffer(0);
    AsteroidsEngine_Draw(&asteroids_engine);
    draw_damage_flash();
    draw_powerup_flash();

    draw_health_bar(AsteroidsEngine_GetLives(&asteroids_engine), max_lives_for_difficulty(selected_difficulty));

    draw_hud_panel(91, 4, 48, 21, 14);
    sprintf(info_str, "W:%d", AsteroidsEngine_GetWave(&asteroids_engine));
    LCD_printString(info_str, 101, 11, 1, 1);

    draw_hud_panel(146, 4, 88, 21, 14);
    sprintf(info_str, "S:%d", AsteroidsEngine_GetScore(&asteroids_engine));
    LCD_printString(info_str, 154, 11, 1, 1);
    draw_powerup_status(AsteroidsEngine_GetWeaponTimer(&asteroids_engine),
                        AsteroidsEngine_GetMagnetTimer(&asteroids_engine),
                        AsteroidsEngine_HasShield(&asteroids_engine));

    LCD_Refresh(&cfg0);
}

static void render_striker(void)
{
    char info_str[40];

    LCD_Fill_Buffer(0);
    StrikerEngine_Draw(&striker_engine);
    draw_damage_flash();
    draw_powerup_flash();

    draw_health_bar(StrikerEngine_GetHp(&striker_engine), StrikerEngine_GetMaxHp(&striker_engine));

    draw_hud_panel(91, 4, 62, 21, 11);
    sprintf(info_str, "D:%d%%", StrikerEngine_GetEscapeProgress(&striker_engine));
    LCD_printString(info_str, 99, 11, 1, 1);

    draw_hud_panel(160, 4, 74, 21, StrikerEngine_GetSignalCore(&striker_engine) < 35U ? 2U : 14U);
    sprintf(info_str, "C:%d%%", StrikerEngine_GetSignalCore(&striker_engine));
    LCD_printString(info_str, 168, 11, 1, 1);
    draw_powerup_status(StrikerEngine_GetWeaponTimer(&striker_engine),
                        StrikerEngine_GetMagnetTimer(&striker_engine),
                        StrikerEngine_HasShield(&striker_engine));

    LCD_Refresh(&cfg0);
}

static void render_versus(void)
{
    LCD_Fill_Buffer(0);
    VersusEngine_Draw(&versus_engine);
    draw_damage_flash();
    draw_powerup_flash();
    LCD_Refresh(&cfg0);
}

static void render_success_animation(void)
{
    uint32_t elapsed = HAL_GetTick() - success_anim_start_tick;
    uint16_t ship_y;
    uint8_t blink = (uint8_t)((elapsed / 120U) & 1U);
    uint8_t accent = ship_accent(selected_ship);
    uint8_t i;

    if (elapsed > GAME3_SUCCESS_ANIM_MS) {
        elapsed = GAME3_SUCCESS_ANIM_MS;
    }

    LCD_Fill_Buffer(0);

    LCD_Draw_Rect(8, 8, 224, 224, 9U, 0);
    LCD_Draw_Line(8, 8, 42, 8, accent);
    LCD_Draw_Line(8, 8, 8, 42, accent);
    LCD_Draw_Line(198, 8, 232, 8, accent);
    LCD_Draw_Line(232, 8, 232, 42, accent);
    LCD_Draw_Line(8, 198, 8, 232, accent);
    LCD_Draw_Line(8, 232, 42, 232, accent);
    LCD_Draw_Line(198, 232, 232, 232, accent);
    LCD_Draw_Line(232, 198, 232, 232, accent);

    for (i = 0; i < 34U; i++) {
        uint16_t x = (uint16_t)((i * 37U + elapsed / 11U) % 240U);
        uint16_t y = (uint16_t)((i * 19U + elapsed / 7U) % 240U);
        LCD_Set_Pixel(x, y, (i & 1U) ? 13U : 1U);
        if ((i % 5U) == 0U && y < 236U) {
            LCD_Set_Pixel(x, (uint16_t)(y + 1U), accent);
        }
    }

    if (elapsed < 900U) {
        uint16_t radius = (uint16_t)(12U + (elapsed / 35U));
        LCD_Draw_Circle(120, 58, radius, blink ? 14U : 2U, 0);
        LCD_Draw_Circle(120, 58, (uint16_t)(radius + 8U), 13U, 0);
        LCD_Draw_Circle(95, 68, (uint16_t)(8U + elapsed / 60U), 2U, 0);
        LCD_Draw_Circle(145, 68, (uint16_t)(8U + elapsed / 60U), 14U, 0);
        LCD_Draw_Line(82, 58, 158, 58, blink ? 1U : 14U);
        LCD_Draw_Line(120, 28, 120, 88, blink ? 1U : 2U);
        LCD_Draw_Sprite(104, 44,
                        AST_EXPLOSION_SPRITE_H,
                        AST_EXPLOSION_SPRITE_W,
                        AST_EXPLOSION_SPRITES[(elapsed / 120U) % AST_EXPLOSION_SPRITE_COUNT]);
    }

    ship_y = (uint16_t)(164U - ((elapsed > 1800U ? 1800U : elapsed) / 16U));
    if (elapsed > 500U) {
        uint16_t gate_r = (uint16_t)(24U + ((elapsed - 500U) / 35U));
        if (gate_r < 70U) {
            LCD_Draw_Circle(120, 112, gate_r, 11U, 0);
            LCD_Draw_Circle(120, 112, (uint16_t)(gate_r + 8U), 13U, 0);
        }
    }
    LCD_Draw_Line(78, 228, 108, (uint16_t)(ship_y + 40U), 11U);
    LCD_Draw_Line(162, 228, 132, (uint16_t)(ship_y + 40U), 11U);
    LCD_Draw_Line(98, 230, 114, (uint16_t)(ship_y + 44U), 14U);
    LCD_Draw_Line(142, 230, 126, (uint16_t)(ship_y + 44U), 14U);
    LCD_Draw_Sprite(96, ship_y,
                    STRIKER_PLAYER_SHIP_H,
                    STRIKER_PLAYER_SHIP_W,
                    STRIKER_PLAYER_SHIPS[selected_ship]);
    LCD_Draw_Line(108, (uint16_t)(ship_y + 46U), 96, 226, 11U);
    LCD_Draw_Line(132, (uint16_t)(ship_y + 46U), 144, 226, 11U);
    LCD_Draw_Line(120, (uint16_t)(ship_y + 48U), 120, 232, blink ? 1U : 14U);

    if (elapsed > 900U) {
        uint16_t scan_y = (uint16_t)(88U + ((elapsed - 900U) / 18U) % 58U);
        LCD_Draw_Line(38, scan_y, 202, scan_y, 11U);
        LCD_Draw_Line(54, (uint16_t)(scan_y + 8U), 186, (uint16_t)(scan_y + 8U), 14U);
        draw_simple_panel(24, 68, 192, 88, blink ? 14U : 11U, 1U);
        LCD_Draw_Line(34, 78, 206, 78, accent);
        LCD_Draw_Line(34, 146, 206, 146, accent);
        draw_centered_text("SIGNAL SECURED", 86, 1U, 1);
        draw_centered_text("ESCAPE COMPLETE", 108, 14U, 1);
        LCD_printString("CORE", 54, 132, 13U, 1);
        LCD_Draw_Rect(92, 132, 82, 8, 11U, 0);
        LCD_Draw_Rect(94, 134, (uint16_t)(78U * StrikerEngine_GetSignalCore(&striker_engine) / 100U), 4, 14U, 1);
        LCD_printString("OK", 184, 132, blink ? 14U : 1U, 1);
    }

    if (elapsed > 1900U) {
        draw_centered_text("MISSION COMPLETE", 18, 14U, 2);
        draw_centered_text("SIGNAL ESCAPED", 196, 1U, 1);
        draw_centered_text("REPORT INCOMING", 214, 13U, 1);
    }

    LCD_Refresh(&cfg0);
}

static void render_game_over(void)
{
    char score_str[32];
    char high_score_str[32];
    char time_str[32];
    char stat_a_str[32];
    char stat_b_str[32];
    char core_str[32];
    char dist_str[32];
    uint16_t seconds;

    LCD_Fill_Buffer(0);
    LCD_Draw_Rect(4, 4, 232, 232, 9, 0);
    if (active_mode == GAME3_MODE_VERSUS) {
        char line[32];
        VersusWinner_t winner = VersusEngine_GetWinner(&versus_engine);
        SpaceShipType_t winner_ship = (winner == VERSUS_WINNER_P1) ? versus_p1_ship : versus_p2_ship;
        uint8_t accent = (winner == VERSUS_WINNER_P1) ? 14U : 2U;

        draw_centered_text("BATTLE RESULT", 12, accent, 2);
        draw_star_icon(28, 24, 14U);
        draw_star_icon(212, 24, 2U);
        draw_simple_panel(24, 38, 192, 64, accent, 1U);
        draw_crown_icon(54, 48, accent);
        LCD_Draw_Sprite(52, 54,
                        STRIKER_PLAYER_SHIP_H,
                        STRIKER_PLAYER_SHIP_W,
                        STRIKER_PLAYER_SHIPS[winner_ship]);
        LCD_Draw_Sprite(156, 56,
                        AST_EXPLOSION_SPRITE_H,
                        AST_EXPLOSION_SPRITE_W,
                        AST_EXPLOSION_SPRITES[AST_EXPLOSION_SPRITE_COUNT - 1U]);
        LCD_printString("WIN", 68, 91, accent, 1);
        LCD_printString("BOOM", 158, 91, 2, 1);

        draw_simple_panel(34, 112, 172, 84, accent, 1U);
        sprintf(line, "WINNER: P%d", (winner == VERSUS_WINNER_P1) ? 1 : 2);
        draw_centered_text(line, 124, 1, 1);
        sprintf(line, "P1 SHIP:%s", ship_name(versus_p1_ship));
        LCD_printString(line, 54, 142, 14, 1);
        sprintf(line, "P2 SHIP:%s", ship_name(versus_p2_ship));
        LCD_printString(line, 54, 156, 2, 1);
        sprintf(line, "P1 HP:%d", VersusEngine_GetP1Hp(&versus_engine));
        LCD_printString(line, 54, 170, 1, 1);
        sprintf(line, "P2 HP:%d", VersusEngine_GetP2Hp(&versus_engine));
        LCD_printString(line, 126, 170, 1, 1);
        sprintf(line, "HITS %d / %d", VersusEngine_GetP1Hits(&versus_engine), VersusEngine_GetP2Hits(&versus_engine));
        draw_centered_text(line, 184, 13, 1);
        draw_centered_text("BTN2/BTN8: MENU", 207, 14, 1);
        LCD_Refresh(&cfg0);
        return;
    }

    if (active_mode == GAME3_MODE_STRIKER) {
        draw_centered_text(StrikerEngine_IsMissionComplete(&striker_engine) ? "MISSION COMPLETE" : "MISSION FAILED",
                           12,
                           StrikerEngine_IsMissionComplete(&striker_engine) ? 14U : 2U,
                           2);
    } else {
        draw_centered_text("MISSION END", 12, 2, 2);
    }
    draw_star_icon(24, 24, 14U);
    draw_star_icon(216, 24, 11U);
    if (active_mode == GAME3_MODE_STRIKER) {
        uint16_t kills = StrikerEngine_GetEnemyKills(&striker_engine);
        uint16_t bosses = StrikerEngine_GetBossKills(&striker_engine);

        seconds = StrikerEngine_GetSurvivalSeconds(&striker_engine);
        sprintf(score_str, "%d", StrikerEngine_GetScore(&striker_engine));
        sprintf(high_score_str, "%d", striker_high_score);
        sprintf(stat_a_str, "%d", kills);
        sprintf(stat_b_str, "%d", bosses);
        sprintf(core_str, "%d%%", StrikerEngine_GetSignalCore(&striker_engine));
        sprintf(dist_str, "%d%%", StrikerEngine_GetEscapeProgress(&striker_engine));
    } else {
        uint16_t rocks = AsteroidsEngine_GetRocksDestroyed(&asteroids_engine);
        uint8_t wave = AsteroidsEngine_GetMaxWaveReached(&asteroids_engine);

        seconds = AsteroidsEngine_GetSurvivalSeconds(&asteroids_engine);
        sprintf(score_str, "%d", AsteroidsEngine_GetScore(&asteroids_engine));
        sprintf(high_score_str, "%d", asteroids_high_score);
        sprintf(stat_a_str, "%d", rocks);
        sprintf(stat_b_str, "%d", wave);
        core_str[0] = '\0';
        dist_str[0] = '\0';
    }
    sprintf(time_str, "%02d:%02d", seconds / 60U, seconds % 60U);

    draw_result_badge(120, 54, ship_accent(selected_ship));
    draw_centered_text(solo_result_title(active_mode), 86, ship_accent(selected_ship), 1);

    if (active_mode == GAME3_MODE_STRIKER) {
        draw_simple_panel(34, 98, 172, 106, StrikerEngine_IsMissionComplete(&striker_engine) ? 14U : 2U, 1U);
        LCD_printString("MODE:", 50, 108, 13, 1);
        LCD_printString(mode_name(active_mode), 104, 108, 1, 1);
        LCD_printString("SHIP:", 50, 122, 13, 1);
        LCD_printString(ship_name(selected_ship), 104, 122, ship_accent(selected_ship), 1);
        LCD_printString("SCORE:", 50, 136, 13, 1);
        LCD_printString(score_str, 104, 136, 1, 1);
        LCD_printString("BEST:", 50, 150, 13, 1);
        LCD_printString(high_score_str, 104, 150, 1, 1);
        LCD_printString("TIME:", 50, 164, 13, 1);
        LCD_printString(time_str, 104, 164, 1, 1);
        LCD_printString("CORE:", 50, 178, 13, 1);
        LCD_printString(core_str, 104, 178, StrikerEngine_GetSignalCore(&striker_engine) == 0U ? 2U : 1U, 1);
        LCD_printString("DIST:", 138, 178, 13, 1);
        LCD_printString(dist_str, 174, 178, 1, 1);
        LCD_printString("K/B:", 50, 192, 13, 1);
        LCD_printString(stat_a_str, 86, 192, 1, 1);
        LCD_printString("/", 122, 192, 13, 1);
        LCD_printString(stat_b_str, 134, 192, 1, 1);
    } else {
        draw_simple_panel(34, 102, 172, 94, 2, 1U);
        LCD_printString("MODE:", 50, 114, 13, 1);
        LCD_printString(mode_name(active_mode), 104, 114, 1, 1);
        LCD_printString("SHIP:", 50, 128, 13, 1);
        LCD_printString(ship_name(selected_ship), 104, 128, ship_accent(selected_ship), 1);
        LCD_printString("SCORE:", 50, 142, 13, 1);
        LCD_printString(score_str, 104, 142, 1, 1);
        LCD_printString("BEST:", 50, 156, 13, 1);
        LCD_printString(high_score_str, 104, 156, 1, 1);
        LCD_printString("TIME:", 50, 170, 13, 1);
        LCD_printString(time_str, 104, 170, 1, 1);
        LCD_printString("ROCKS:", 50, 184, 13, 1);
        LCD_printString(stat_a_str, 104, 184, 1, 1);
        LCD_printString("WAVE:", 138, 184, 13, 1);
        LCD_printString(stat_b_str, 174, 184, 1, 1);
    }
    draw_centered_text("BTN2/BTN3: MENU", 207, 14, 1);
    LCD_Refresh(&cfg0);
}

