#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// INPUT SYSTEM - Simple State
// ========================================

/**
 * @brief Simple input state structure
 * 
 * Store the current input state. Main loop checks this to determine
 * what happened this frame.
 */
typedef struct {
    uint8_t btn2_pressed;  // 1 if BT2 was pressed this frame, 0 otherwise
    uint8_t btn3_pressed;  // 1 if BT3 was pressed this frame, 0 otherwise
    uint8_t btn4_pressed;  // 1 if BT4 was pressed this frame, 0 otherwise
    uint8_t btn5_pressed;  // 1 if BT5 was pressed this frame, 0 otherwise
    uint8_t btn6_pressed;  // 1 if BT6 was pressed this frame, 0 otherwise
    uint8_t btn7_pressed;  // 1 if BT7 was pressed this frame, 0 otherwise
    uint8_t btn8_pressed;  // 1 if BT8 was pressed this frame, 0 otherwise
    uint8_t btn9_pressed;  // 1 if BT9 was pressed this frame, 0 otherwise
    uint8_t b1_pressed;    // 1 if onboard B1 was pressed this frame, 0 otherwise
} InputState;

// Global input state (read by menu/games in their Update functions)
extern InputState current_input;

/**
 * @brief Initialize button input system
 * Must be called in main() after GPIO initialization
 */
void Input_Init(void);

/**
 * @brief Read current input state
 * Called once per frame by main loop before Update
 */
void Input_Read(void);

/**
 * @brief Clear any pending one-shot button events.
 * Useful when moving between menu and games to prevent accidental carry-over.
 */
void Input_Clear(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_HANDLER_H
