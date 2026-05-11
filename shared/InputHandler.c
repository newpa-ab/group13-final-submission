#include "InputHandler.h"
#include "main.h"

// Global input state
InputState current_input = {0};

// Track button presses in interrupt
static volatile uint8_t btn2_raw_press = 0;
static volatile uint8_t btn3_raw_press = 0;
static volatile uint8_t btn4_raw_press = 0;
static volatile uint8_t btn5_raw_press = 0;
static volatile uint8_t btn6_raw_press = 0;
static volatile uint8_t btn7_raw_press = 0;
static volatile uint8_t btn8_raw_press = 0;
static volatile uint8_t btn9_raw_press = 0;
static volatile uint8_t b1_raw_press = 0;

#define BUTTON_DEBOUNCE_MS 50U
#define JOYSTICK_BUTTON_DEBOUNCE_MS 300U

void Input_Init(void) {
    // GPIO and EXTI already initialized by MX_GPIO_Init() in main.c
    // Just reset the state
    current_input.btn2_pressed = 0;
    current_input.btn3_pressed = 0;
    current_input.btn4_pressed = 0;
    current_input.btn5_pressed = 0;
    current_input.btn6_pressed = 0;
    current_input.btn7_pressed = 0;
    current_input.btn8_pressed = 0;
    current_input.btn9_pressed = 0;
    current_input.b1_pressed = 0;
    btn2_raw_press = 0;
    btn3_raw_press = 0;
    btn4_raw_press = 0;
    btn5_raw_press = 0;
    btn6_raw_press = 0;
    btn7_raw_press = 0;
    btn8_raw_press = 0;
    btn9_raw_press = 0;
    b1_raw_press = 0;
}

void Input_Read(void) {
    // Copy the button press flags from interrupt to current input state
    // This is read once per frame by the main loop
    current_input.btn2_pressed = btn2_raw_press;
    current_input.btn3_pressed = btn3_raw_press;
    current_input.btn4_pressed = btn4_raw_press;
    current_input.btn5_pressed = btn5_raw_press;
    current_input.btn6_pressed = btn6_raw_press;
    current_input.btn7_pressed = btn7_raw_press;
    current_input.btn8_pressed = btn8_raw_press;
    current_input.btn9_pressed = btn9_raw_press;
    current_input.b1_pressed = b1_raw_press;
    
    // Reset the flags after reading so they only trigger once
    btn2_raw_press = 0;
    btn3_raw_press = 0;
    btn4_raw_press = 0;
    btn5_raw_press = 0;
    btn6_raw_press = 0;
    btn7_raw_press = 0;
    btn8_raw_press = 0;
    btn9_raw_press = 0;
    b1_raw_press = 0;
}

void Input_Clear(void) {
    current_input.btn2_pressed = 0;
    current_input.btn3_pressed = 0;
    current_input.btn4_pressed = 0;
    current_input.btn5_pressed = 0;
    current_input.btn6_pressed = 0;
    current_input.btn7_pressed = 0;
    current_input.btn8_pressed = 0;
    current_input.btn9_pressed = 0;
    current_input.b1_pressed = 0;
    btn2_raw_press = 0;
    btn3_raw_press = 0;
    btn4_raw_press = 0;
    btn5_raw_press = 0;
    btn6_raw_press = 0;
    btn7_raw_press = 0;
    btn8_raw_press = 0;
    btn9_raw_press = 0;
    b1_raw_press = 0;
}

// ===== INTERRUPT CALLBACK FOR BUTTONS =====
// Called by hardware when button is pressed
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    static uint32_t last_btn2_interrupt = 0;
    static uint32_t last_btn3_interrupt = 0;
    static uint32_t last_btn4_interrupt = 0;
    static uint32_t last_btn5_interrupt = 0;
    static uint32_t last_btn6_interrupt = 0;
    static uint32_t last_btn7_interrupt = 0;
    static uint32_t last_btn8_interrupt = 0;
    static uint32_t last_btn9_interrupt = 0;
    static uint32_t last_b1_interrupt = 0;
    uint32_t current_time = HAL_GetTick();
    
    // Handle BT2
    if (GPIO_Pin == BTN2_Pin) {
        // Software debouncing
        if ((current_time - last_btn2_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn2_interrupt = current_time;
            
            // Toggle LED to indicate button press
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            
            // Set flag indicating button was pressed
            btn2_raw_press = 1;
        }
    }
    
    // Handle BT3 (joystick button)
    if (GPIO_Pin == BTN3_Pin) {
        // Software debouncing
        if ((current_time - last_btn3_interrupt) > JOYSTICK_BUTTON_DEBOUNCE_MS) {
            last_btn3_interrupt = current_time;
            
            // Toggle LED to indicate button press
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            
            // Set flag indicating button was pressed
            btn3_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN8_Pin) {
        if ((current_time - last_btn8_interrupt) > JOYSTICK_BUTTON_DEBOUNCE_MS) {
            last_btn8_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn8_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN9_Pin) {
        if ((current_time - last_btn9_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn9_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn9_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN4_Pin) {
        if ((current_time - last_btn4_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn4_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn4_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN5_Pin) {
        if ((current_time - last_btn5_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn5_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn5_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN6_Pin) {
        if ((current_time - last_btn6_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn6_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn6_raw_press = 1;
        }
    }

    if (GPIO_Pin == BTN7_Pin) {
        if ((current_time - last_btn7_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_btn7_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            btn7_raw_press = 1;
        }
    }

    if (GPIO_Pin == B1_Pin) {
        if ((current_time - last_b1_interrupt) > BUTTON_DEBOUNCE_MS) {
            last_b1_interrupt = current_time;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            b1_raw_press = 1;
        }
    }
}
