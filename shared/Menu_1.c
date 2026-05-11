#include "Menu.h"
#include "LCD.h"
#include "InputHandler.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>

extern ST7789V2_cfg_t cfg0;  // LCD configuration from main.c
extern Joystick_cfg_t joystick_cfg;  // Joystick configuration
extern Joystick_t joystick_data;     // Current joystick readings

// Menu options are framed as one connected three-stage mission.
static const char* mission_titles[] = {
    "MISSION 1",
    "MISSION 2",
    "MISSION 3"
};

static const char* mission_names[] = {
    "LOCATE",
    "STEAL",
    "ESCAPE"
};

static const char* mission_desc[] = {
    "Find signal source",
    "Steal signal core",
    "Escape with signal"
};
#define NUM_MENU_OPTIONS 3

// Frame rate for menu (in milliseconds)
#define MENU_FRAME_TIME_MS 30  // ~33 FPS
#define MENU_ENTRY_LOCK_MS 250U

static uint16_t menu_centered_x(const char* text, uint8_t size)
{
    uint16_t chars = 0;
    uint16_t width = 0;

    while (text[chars] != '\0') {
        chars++;
    }

    width = (uint16_t)(chars * 6U * size);
    if (width >= 240U) {
        return 0;
    }
    return (uint16_t)((240U - width) / 2U);
}

static void menu_print_centered(const char* text, uint16_t y, uint8_t colour, uint8_t size)
{
    LCD_printString((char*)text, menu_centered_x(text, size), y, colour, size);
}

/**
 * @brief Render the home menu screen
 */
static void render_home_menu(MenuSystem* menu) {
    LCD_Fill_Buffer(0);

    LCD_Draw_Rect(8, 8, 224, 224, 6, 0);
    menu_print_centered("MISSION CONSOLE", 18, 1, 2);
    menu_print_centered("FIND -> STEAL -> ESCAPE", 42, 6, 1);
    LCD_Draw_Line(28, 62, 212, 62, 6);

    for (int i = 0; i < NUM_MENU_OPTIONS; i++) {
        uint16_t y_pos = (uint16_t)(78 + (i * 42));
        uint8_t selected = (uint8_t)(i == menu->selected_option);
        uint8_t frame_colour = selected ? 6 : 9;
        uint8_t title_colour = selected ? 6 : 1;
        uint8_t name_colour = selected ? 1 : 6;

        LCD_Draw_Rect(20, (uint16_t)(y_pos - 8), 200, 34, frame_colour, selected ? 0 : 1);
        if (i == menu->selected_option) {
            LCD_printString(">", 28, y_pos, 6, 2);
        }

        LCD_printString((char*)mission_titles[i], 48, (uint16_t)(y_pos - 2), title_colour, 1);
        LCD_printString((char*)mission_names[i], 122, (uint16_t)(y_pos - 4), name_colour, 2);
        LCD_printString((char*)mission_desc[i], 48, (uint16_t)(y_pos + 14), 1, 1);
    }

    menu_print_centered("JOYSTICK UP/DOWN", 210, 1, 1);
    menu_print_centered("BT2/BT3 SELECT", 224, 6, 1);

    LCD_Refresh(&cfg0);
}

// ==============================================
// PUBLIC API IMPLEMENTATION
// ==============================================

void Menu_Init(MenuSystem* menu) {
    menu->selected_option = 0;
}

MenuState Menu_Run(MenuSystem* menu) {
    static Direction last_direction = CENTRE;  // Track last direction for debouncing
    MenuState selected_game = MENU_STATE_HOME;  // Which game was selected
    uint32_t unlock_tick = HAL_GetTick() + MENU_ENTRY_LOCK_MS;

    Input_Clear();
    
    // Menu's own loop - runs until game is selected
    while (1) {
        uint32_t frame_start = HAL_GetTick();
        
        // Read input
        Input_Read();
        
        // Read current joystick position
        Joystick_Read(&joystick_cfg, &joystick_data);
        
        // Handle joystick navigation (up/down to select option)
        Direction current_direction = joystick_data.direction;
        
        if (current_direction == S && last_direction != S) {  // Joystick pushed DOWN
            // Move selection down
            menu->selected_option++;
            if (menu->selected_option >= NUM_MENU_OPTIONS) {
                menu->selected_option = 0;  // Wrap around
            }
        } 
        else if (current_direction == N && last_direction != N) {  // Joystick pushed UP
            // Move selection up
            if (menu->selected_option == 0) {
                menu->selected_option = NUM_MENU_OPTIONS - 1;  // Wrap around
            } else {
                menu->selected_option--;
            }
        }
        
        last_direction = current_direction;
        
        // Handle button press to select current option
        if ((HAL_GetTick() >= unlock_tick) &&
            (current_input.btn2_pressed || current_input.btn3_pressed)) {
            // User pressed button - select the highlighted option
            if (menu->selected_option == 0) {
                selected_game = MENU_STATE_GAME_1;
            } else if (menu->selected_option == 1) {
                selected_game = MENU_STATE_GAME_2;
            } else if (menu->selected_option == 2) {
                selected_game = MENU_STATE_GAME_3;
            }
            break;  // Exit menu loop - game selected!
        }
        
        // Render menu
        render_home_menu(menu);
        
        // Frame timing - wait for remainder of frame time
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < MENU_FRAME_TIME_MS) {
            HAL_Delay(MENU_FRAME_TIME_MS - frame_time);
        }
    }
    
    return selected_game;  // Return which game was selected
}
