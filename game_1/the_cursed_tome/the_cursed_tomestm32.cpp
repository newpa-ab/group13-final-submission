#include "the_cursed_tomestm32.h"

#include "Game_1.h"
#include "Platform.h"
#include "Defines.h"
#include "FixedMath.h"
#include "SceneRenderer.h"

extern "C" {
#include "adc.h"
#include "rng.h"
#include "tim.h"
#include "main.h"
#include "lcd_if.h"
#include "LCD.h"
#include "Joystick.h"
#include "Buzzer.h"

extern ST7789V2_cfg_t cfg0;
}

#include <stddef.h>
#include <string.h>

namespace {

constexpr uint16_t kMonoWidth = DISPLAY_WIDTH;
constexpr uint16_t kMonoHeight = DISPLAY_HEIGHT;
constexpr uint16_t kMonoPages = DISPLAY_HEIGHT / 8U;
constexpr uint16_t kPanelWidth = 240;
constexpr uint16_t kPanelHeight = 240;
constexpr uint16_t kScaledWidth = 240;
constexpr uint16_t kScaledHeight = 120;
constexpr int16_t kBlitX = (kPanelWidth - kScaledWidth) / 2;
constexpr int16_t kBlitY = (kPanelHeight - kScaledHeight) / 2;
constexpr uint16_t kWhite565 = 0xFFFF;
constexpr uint16_t kBlack565 = 0x0000;
constexpr uint16_t kCurseGreen565 = 0x3666;
constexpr uint16_t kCurseGlow565 = 0x7FE0;
constexpr uint16_t kBloodRed565 = 0xD104;
constexpr uint16_t kManaCyan565 = 0x3DFF;
constexpr uint16_t kTitleGold565 = 0xE6E0;
constexpr uint16_t kSpriteWhite565 = 0xFFFF;
// Compensated for this panel's apparent red/blue channel ordering in bitmap blits.
constexpr uint16_t kTorchOrange565 = 0x05FF;
constexpr uint16_t kCeilingBlue565 = 0x2965;
constexpr uint16_t kFloorBrown565 = 0x8A85;
constexpr uint16_t kWallStone565 = 0xBDF7;
constexpr uint16_t kWoodBrown565 = 0x8A85;
constexpr uint16_t kClayBrown565 = 0xA3A0;
constexpr uint16_t kParchment565 = 0xEED8;
constexpr uint16_t kTonesEnd = 0x8000;
constexpr uint32_t kFrameDurationMs = 1000U / TARGET_FRAMERATE;

uint8_t g_screenBuffer[kMonoWidth * kMonoPages];
uint8_t g_tintBuffer[kMonoWidth * kMonoHeight];
uint32_t g_lastTimingSample = 0U;
int32_t g_tickAccum = 0;
bool g_audioEnabled = true;
bool g_portReady = false;
uint8_t g_activeTint = 0U;
uint16_t g_sessionSeed = 0xACE1U;

constexpr size_t kJoystickConfigsCount = 3U;
Joystick_cfg_t g_joystickCfgs[kJoystickConfigsCount] = {};
Joystick_t g_joystickData[kJoystickConfigsCount] = {};

Buzzer_cfg_t g_buzzerCfg = {};

uint32_t g_soundStopTick = 0U;

void beginGameSession(void)
{
    memset(g_screenBuffer, 0x00, sizeof(g_screenBuffer));
    memset(g_tintBuffer, 0x00, sizeof(g_tintBuffer));
    LCD_Clear(kBlack565);
    buzzer_off(&g_buzzerCfg);
    g_soundStopTick = 0U;
    Platform::FillScreen(COLOUR_BLACK);
    Game1_Init();
    g_lastTimingSample = HAL_GetTick();
    g_tickAccum = 0;
}

inline void setPixelInBuffer(uint8_t x, uint8_t y, bool on)
{
    if (x >= kMonoWidth || y >= kMonoHeight) {
        return;
    }

    uint16_t index = (uint16_t)x + (((uint16_t)y >> 3U) * kMonoWidth);
    uint8_t mask = (uint8_t)(1U << (y & 7U));

    if (on) {
        g_screenBuffer[index] |= mask;
    } else {
        g_screenBuffer[index] &= (uint8_t)~mask;
    }
}

inline bool getPixelFromBuffer(uint8_t x, uint8_t y)
{
    uint16_t index = (uint16_t)x + (((uint16_t)y >> 3U) * kMonoWidth);
    uint8_t mask = (uint8_t)(1U << (y & 7U));
    return (g_screenBuffer[index] & mask) != 0U;
}

inline void setTintInBuffer(uint8_t x, uint8_t y, uint8_t tint)
{
    if (x >= kMonoWidth || y >= kMonoHeight) {
        return;
    }

    g_tintBuffer[(uint16_t)y * kMonoWidth + x] = tint;
}

inline uint8_t getTintFromBuffer(uint8_t x, uint8_t y)
{
    return g_tintBuffer[(uint16_t)y * kMonoWidth + x];
}

inline uint16_t tintToRgb565(uint8_t tint)
{
    switch (tint) {
    case 1:
        return kCurseGreen565;
    case 2:
        return kCurseGlow565;
    case 3:
        return kBloodRed565;
    case 4:
        return kManaCyan565;
    case 5:
        return kTitleGold565;
    case 6:
        return kCeilingBlue565;
    case 7:
        return kFloorBrown565;
    case 8:
        return kWallStone565;
    case 9:
        return kSpriteWhite565;
    case 10:
        return kTorchOrange565;
    case 11:
        return kWoodBrown565;
    case 12:
        return kClayBrown565;
    case 13:
        return kParchment565;
    default:
        return kWhite565;
    }
}

inline uint8_t rgb565ToLcdColour(uint16_t color)
{
    uint8_t r = static_cast<uint8_t>((color >> 11) & 0x1F);
    uint8_t g = static_cast<uint8_t>((color >> 5) & 0x3F);
    uint8_t b = static_cast<uint8_t>(color & 0x1F);

    if (r < 3U && g < 6U && b < 3U) {
        return 0U;
    }
    if (r > 26U && g > 54U && b > 26U) {
        return 1U;
    }
    if (r > 20U && g > 38U && b < 12U) {
        return 6U;
    }
    if (r > 18U && g < 20U && b < 12U) {
        return 2U;
    }
    if (r < 12U && g > 28U && b < 14U) {
        return 3U;
    }
    if (r < 12U && g > 30U && b > 18U) {
        return 14U;
    }
    if (r > 18U && g < 24U && b > 16U) {
        return 7U;
    }
    if (r > 12U && g > 22U && b > 12U) {
        return 9U;
    }

    return 13U;
}

inline uint16_t classifySceneRgb565(uint8_t x, uint8_t y)
{
    int16_t horizon = SceneRenderer::GetHorizon(x);
    int16_t wallExtent = SceneRenderer::wBuffer[x];
    int16_t wallTop = horizon - wallExtent;
    int16_t wallBottom = horizon + wallExtent;

    if (wallExtent > 0 && y >= wallTop && y <= wallBottom) {
        return kWhite565;
    }

    if (y < horizon) {
        return kCeilingBlue565;
    }

    return kFloorBrown565;
}

void drawBitmapInternal(int16_t x, int16_t y, const uint8_t *bitmap, bool clearBackground)
{
    if (bitmap == NULL) {
        return;
    }

    uint8_t width = pgm_read_byte(&bitmap[0]);
    uint8_t height = pgm_read_byte(&bitmap[1]);
    uint8_t pages = (uint8_t)((height + 7U) / 8U);
    const uint8_t *data = bitmap + 2;

    for (uint8_t col = 0; col < width; ++col) {
        for (uint8_t page = 0; page < pages; ++page) {
            uint8_t bits = pgm_read_byte(&data[(uint16_t)page * width + col]);

            for (uint8_t bit = 0; bit < 8U; ++bit) {
                uint8_t py = (uint8_t)(page * 8U + bit);
                if (py >= height) {
                    break;
                }

                bool on = (bits & (uint8_t)(1U << bit)) != 0U;
                if (on || clearBackground) {
                    int16_t outX = (int16_t)(x + col);
                    int16_t outY = (int16_t)(y + py);
                    if (outX >= 0 && outX < kMonoWidth && outY >= 0 && outY < kMonoHeight) {
                        setPixelInBuffer((uint8_t)outX, (uint8_t)outY, on);
                    }
                }
            }
        }
    }
}

void drawSpriteFrame(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
    if (bitmap == NULL) {
        return;
    }

    uint8_t width = pgm_read_byte(&bitmap[0]);
    uint8_t height = pgm_read_byte(&bitmap[1]);
    uint8_t pages = (uint8_t)((height + 7U) / 8U);
    uint16_t bytePairsPerFrame = (uint16_t)width * pages;
    uint16_t frameSize = (uint16_t)(bytePairsPerFrame * 2U);
    const uint8_t *framePtr = bitmap + 2U + ((uint16_t)frame * frameSize);

    for (uint8_t col = 0; col < width; ++col) {
        for (uint8_t page = 0; page < pages; ++page) {
            uint16_t offset = (uint16_t)page * width + col;
            uint16_t pairIndex = (uint16_t)(offset * 2U);
            uint8_t bits = pgm_read_byte(&framePtr[pairIndex]);
            uint8_t maskBits = pgm_read_byte(&framePtr[pairIndex + 1U]);

            for (uint8_t bit = 0; bit < 8U; ++bit) {
                uint8_t py = (uint8_t)(page * 8U + bit);
                if (py >= height) {
                    break;
                }

                if ((maskBits & (uint8_t)(1U << bit)) != 0U) {
                    int16_t outX = (int16_t)(x + col);
                    int16_t outY = (int16_t)(y + py);
                    if (outX >= 0 && outX < kMonoWidth && outY >= 0 && outY < kMonoHeight) {
                        setPixelInBuffer((uint8_t)outX,
                                         (uint8_t)outY,
                                         (bits & (uint8_t)(1U << bit)) != 0U);
                    }
                }
            }
        }
    }
}

void drawExternalMaskedSpriteFrame(int16_t x, int16_t y,
                                   const uint8_t *bitmap, const uint8_t *mask,
                                   uint8_t frame, uint8_t maskFrame)
{
    if (bitmap == NULL || mask == NULL) {
        return;
    }

    uint8_t width = pgm_read_byte(&bitmap[0]);
    uint8_t height = pgm_read_byte(&bitmap[1]);
    uint8_t pages = (uint8_t)((height + 7U) / 8U);
    uint16_t frameSize = (uint16_t)width * pages;
    const uint8_t *bitmapFrame = bitmap + 2U + ((uint16_t)frame * frameSize);
    const uint8_t *maskFramePtr = mask + ((uint16_t)maskFrame * frameSize);

    for (uint8_t col = 0; col < width; ++col) {
        for (uint8_t page = 0; page < pages; ++page) {
            uint8_t bits = pgm_read_byte(&bitmapFrame[(uint16_t)page * width + col]);
            uint8_t maskBits = pgm_read_byte(&maskFramePtr[(uint16_t)page * width + col]);

            for (uint8_t bit = 0; bit < 8U; ++bit) {
                uint8_t py = (uint8_t)(page * 8U + bit);
                if (py >= height) {
                    break;
                }

                if ((maskBits & (uint8_t)(1U << bit)) != 0U) {
                    int16_t outX = (int16_t)(x + col);
                    int16_t outY = (int16_t)(y + py);
                    if (outX >= 0 && outX < kMonoWidth && outY >= 0 && outY < kMonoHeight) {
                        setPixelInBuffer((uint8_t)outX,
                                         (uint8_t)outY,
                                         (bits & (uint8_t)(1U << bit)) != 0U);
                    }
                }
            }
        }
    }
}

void blitMonoToLcd(void)
{
    bool applyScenePalette = Game1_IsInWorldView() != 0;

    LCD_Fill_Buffer(0);

    for (uint16_t y = 0; y < kScaledHeight; ++y) {
        uint16_t srcY = (uint16_t)((uint32_t)y * kMonoHeight / kScaledHeight);

        for (uint16_t x = 0; x < kScaledWidth; ++x) {
            uint16_t srcX = (uint16_t)((uint32_t)x * kMonoWidth / kScaledWidth);
            uint16_t rgb = kBlack565;

            if (getPixelFromBuffer((uint8_t)srcX, (uint8_t)srcY)) {
                uint8_t tint = getTintFromBuffer((uint8_t)srcX, (uint8_t)srcY);
                if (tint != 0U) {
                    rgb = tintToRgb565(tint);
                } else if (applyScenePalette) {
                    rgb = classifySceneRgb565((uint8_t)srcX, (uint8_t)srcY);
                } else {
                    rgb = kWhite565;
                }
            }

            LCD_Set_Pixel((uint16_t)(kBlitX + x), (uint16_t)(kBlitY + y), rgb565ToLcdColour(rgb));
        }
    }

    LCD_Refresh(&cfg0);
}

uint8_t readButtons(void)
{
    uint8_t input = 0U;
    float bestMagnitude = 0.0f;
    Direction bestDirection = CENTRE;

    for (size_t i = 0; i < kJoystickConfigsCount; ++i) {
        Joystick_Read(&g_joystickCfgs[i], &g_joystickData[i]);
        if (g_joystickData[i].magnitude > bestMagnitude) {
            bestMagnitude = g_joystickData[i].magnitude;
            bestDirection = g_joystickData[i].direction;
        }
    }

    switch (bestDirection) {
    case N:
    case NE:
    case NW:
        input |= INPUT_UP;
        break;
    case S:
    case SE:
    case SW:
        input |= INPUT_DOWN;
        break;
    default:
        break;
    }

    switch (bestDirection) {
    case E:
    case NE:
    case SE:
        input |= INPUT_RIGHT;
        break;
    case W:
    case NW:
    case SW:
        input |= INPUT_LEFT;
        break;
    default:
        break;
    }

    if (HAL_GPIO_ReadPin(BTN8_GPIO_Port, BTN8_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_UP;
    }

    if (HAL_GPIO_ReadPin(BTN9_GPIO_Port, BTN9_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_DOWN;
    }

    if (HAL_GPIO_ReadPin(BTN4_GPIO_Port, BTN4_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_LEFT;
    }

    if (HAL_GPIO_ReadPin(BTN5_GPIO_Port, BTN5_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_RIGHT;
    }

    if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_A;
    }

    if (HAL_GPIO_ReadPin(BTN3_GPIO_Port, BTN3_Pin) == GPIO_PIN_RESET) {
        input |= INPUT_B;
    }

    return input;
}

void updateAudio(void)
{
    if (g_soundStopTick != 0U && HAL_GetTick() >= g_soundStopTick) {
        buzzer_off(&g_buzzerCfg);
        g_soundStopTick = 0U;
    }
}

} /* namespace */

uint8_t Platform::GetInput(void)
{
    return readButtons();
}

uint16_t Platform::GetSessionSeed()
{
    return g_sessionSeed;
}

void Platform::PlaySound(const uint16_t *audioPattern)
{
    if (!g_audioEnabled || audioPattern == NULL) {
        return;
    }

    uint16_t freq = pgm_read_word(&audioPattern[0]);
    uint16_t duration = pgm_read_word(&audioPattern[1]);

    if (freq == kTonesEnd || duration == kTonesEnd) {
        return;
    }

    if (freq == 0U) {
        buzzer_off(&g_buzzerCfg);
    } else {
        buzzer_tone(&g_buzzerCfg, freq, 35U);
    }

    g_soundStopTick = HAL_GetTick() + (uint32_t)duration * 8U;
}

void Platform::SetLED(uint8_t, uint8_t, uint8_t)
{
}

void Platform::PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
    setPixelInBuffer(x, y, colour != 0U);
    setTintInBuffer(x, y, colour != 0U ? g_activeTint : 0U);
}

void Platform::SetDrawTint(uint8_t tint)
{
    g_activeTint = tint;
}

uint8_t Platform::GetDrawTint()
{
    return g_activeTint;
}

void Platform::SetPixelTint(uint8_t x, uint8_t y, uint8_t tint)
{
    setTintInBuffer(x, y, tint);
}

void Platform::TintRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t tint)
{
    uint16_t xEnd = (uint16_t)x + w;
    uint16_t yEnd = (uint16_t)y + h;

    if (x >= kMonoWidth || y >= kMonoHeight) {
        return;
    }

    if (xEnd > kMonoWidth) {
        xEnd = kMonoWidth;
    }

    if (yEnd > kMonoHeight) {
        yEnd = kMonoHeight;
    }

    for (uint16_t py = y; py < yEnd; ++py) {
        for (uint16_t px = x; px < xEnd; ++px) {
            if (getPixelFromBuffer((uint8_t)px, (uint8_t)py)) {
                setTintInBuffer((uint8_t)px, (uint8_t)py, tint);
            }
        }
    }
}

void Platform::DrawVLine(uint8_t x, int8_t y0_, int8_t y1_, uint8_t pattern)
{
    if (x >= kMonoWidth || y1_ < y0_ || y1_ < 0 || y0_ >= (int8_t)kMonoHeight) {
        return;
    }

    int16_t y0 = y0_;
    int16_t y1 = y1_;
    if (y0 < 0) {
        y0 = 0;
    }
    if (y1 >= (int16_t)kMonoHeight) {
        y1 = (int16_t)kMonoHeight - 1;
    }

    for (int16_t y = y0; y <= y1; ++y) {
        bool on = (pattern & (uint8_t)(1U << (y & 7))) != 0U;
        setPixelInBuffer(x, (uint8_t)y, on);
    }
}

uint8_t *Platform::GetScreenBuffer()
{
    return g_screenBuffer;
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
    drawSpriteFrame(x, y, bitmap, frame);
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap,
                          const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
    drawExternalMaskedSpriteFrame(x, y, bitmap, mask, frame, mask_frame);
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
    drawBitmapInternal(x, y, bitmap, false);
}

void Platform::DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
    drawBitmapInternal(x, y, bitmap, true);
}

void Platform::FillScreen(uint8_t colour)
{
    memset(g_screenBuffer, colour ? 0xFF : 0x00, sizeof(g_screenBuffer));
    memset(g_tintBuffer, 0x00, sizeof(g_tintBuffer));
    g_activeTint = 0U;
}

bool Platform::IsAudioEnabled()
{
    return g_audioEnabled;
}

void Platform::SetAudioEnabled(bool isEnabled)
{
    g_audioEnabled = isEnabled;
    if (!g_audioEnabled) {
        buzzer_off(&g_buzzerCfg);
        g_soundStopTick = 0U;
    }
}

void Platform::ExpectLoadDelay()
{
    g_lastTimingSample = HAL_GetTick();
}

void Platform::DrawBackground()
{
}

extern "C" void CursedTomePort_Init(void)
{
    uint32_t seed = 0U;

    if (g_portReady) {
        beginGameSession();
        return;
    }

    LCD_Clear(kBlack565);
    g_joystickCfgs[0].adc = &hadc1;
    g_joystickCfgs[0].x_channel = ADC_CHANNEL_1;
    g_joystickCfgs[0].y_channel = ADC_CHANNEL_2;
    g_joystickCfgs[0].sampling_time = ADC_SAMPLETIME_6CYCLES_5;
    g_joystickCfgs[0].center_x = JOYSTICK_DEFAULT_CENTER_X;
    g_joystickCfgs[0].center_y = JOYSTICK_DEFAULT_CENTER_Y;
    g_joystickCfgs[0].deadzone = JOYSTICK_DEADZONE;
    g_joystickCfgs[0].setup_done = 0U;

    g_joystickCfgs[1].adc = &hadc1;
    g_joystickCfgs[1].x_channel = ADC_CHANNEL_5;
    g_joystickCfgs[1].y_channel = ADC_CHANNEL_6;
    g_joystickCfgs[1].sampling_time = ADC_SAMPLETIME_6CYCLES_5;
    g_joystickCfgs[1].center_x = JOYSTICK_DEFAULT_CENTER_X;
    g_joystickCfgs[1].center_y = JOYSTICK_DEFAULT_CENTER_Y;
    g_joystickCfgs[1].deadzone = JOYSTICK_DEADZONE;
    g_joystickCfgs[1].setup_done = 0U;

    g_joystickCfgs[2].adc = &hadc1;
    g_joystickCfgs[2].x_channel = ADC_CHANNEL_9;
    g_joystickCfgs[2].y_channel = ADC_CHANNEL_15;
    g_joystickCfgs[2].sampling_time = ADC_SAMPLETIME_6CYCLES_5;
    g_joystickCfgs[2].center_x = JOYSTICK_DEFAULT_CENTER_X;
    g_joystickCfgs[2].center_y = JOYSTICK_DEFAULT_CENTER_Y;
    g_joystickCfgs[2].deadzone = JOYSTICK_DEADZONE;
    g_joystickCfgs[2].setup_done = 0U;

    g_buzzerCfg.htim = &htim2;
    g_buzzerCfg.channel = TIM_CHANNEL_3;
    g_buzzerCfg.tick_freq_hz = 1000000U;
    g_buzzerCfg.min_freq_hz = 40U;
    g_buzzerCfg.max_freq_hz = 12000U;
    g_buzzerCfg.setup_done = 0U;
    g_buzzerCfg.pwm_started = 0U;

    for (size_t i = 0; i < kJoystickConfigsCount; ++i) {
        Joystick_Init(&g_joystickCfgs[i]);
        Joystick_Calibrate(&g_joystickCfgs[i]);
    }

    buzzer_init(&g_buzzerCfg);

    if (HAL_RNG_GenerateRandomNumber(&hrng, &seed) == HAL_OK) {
        g_sessionSeed = (uint16_t)(seed & 0xFFFFU);
        SeedRandom(g_sessionSeed);
    } else {
        g_sessionSeed = (uint16_t)HAL_GetTick();
        SeedRandom(g_sessionSeed);
    }

    beginGameSession();
    g_portReady = true;
}

extern "C" void CursedTomePort_Tick(void)
{
    if (!g_portReady) {
        return;
    }

    uint32_t timingSample = HAL_GetTick();
    bool advancedFrame = false;
    g_tickAccum += (int32_t)(timingSample - g_lastTimingSample);
    g_lastTimingSample = timingSample;

    while (g_tickAccum >= (int32_t)kFrameDurationMs) {
        Game1_Tick();
        g_tickAccum -= (int32_t)kFrameDurationMs;
        advancedFrame = true;
    }

    if (advancedFrame) {
        memset(g_tintBuffer, 0x00, sizeof(g_tintBuffer));
        g_activeTint = 0U;
        Game1_Draw();
        blitMonoToLcd();
    }

    updateAudio();
}

extern "C" void CursedTomePort_Stop(void)
{
    buzzer_off(&g_buzzerCfg);
    g_soundStopTick = 0U;
}



