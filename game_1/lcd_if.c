#include "lcd_if.h"

#include "LCD.h"

extern ST7789V2_cfg_t cfg0;

#define LCD_IF_WIDTH 240U
#define LCD_IF_HEIGHT 240U

static uint8_t rgb565_to_palette(uint16_t color)
{
    uint8_t r = (uint8_t)((color >> 11) & 0x1F);
    uint8_t g = (uint8_t)((color >> 5) & 0x3F);
    uint8_t b = (uint8_t)(color & 0x1F);

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

void ILI9341_Init(void)
{
}

void ILI9341_WriteCommand(uint8_t cmd)
{
    (void)cmd;
}

void ILI9341_WriteData(const uint8_t *data, uint16_t size)
{
    (void)data;
    (void)size;
}

void LCD_Clear(uint16_t color)
{
    LCD_Fill_Buffer(rgb565_to_palette(color));
}

void LCD_DrawBitmap(int16_t x, int16_t y, const uint16_t *data, uint16_t w, uint16_t h)
{
    uint16_t row;
    uint16_t col;

    if (data == 0 || w == 0U || h == 0U) {
        return;
    }

    for (row = 0U; row < h; row++) {
        int16_t dy = (int16_t)(y + (int16_t)row);
        if (dy < 0 || dy >= (int16_t)LCD_IF_HEIGHT) {
            continue;
        }

        for (col = 0U; col < w; col++) {
            int16_t dx = (int16_t)(x + (int16_t)col);
            if (dx < 0 || dx >= (int16_t)LCD_IF_WIDTH) {
                continue;
            }
            LCD_Set_Pixel((uint16_t)dx, (uint16_t)dy, rgb565_to_palette(data[((uint32_t)row * w) + col]));
        }
    }

    LCD_Refresh(&cfg0);
}

void LCD_DrawBitmapRegion(int16_t x, int16_t y,
                          const uint16_t *data, uint16_t src_w, uint16_t src_h,
                          uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h)
{
    uint16_t row;
    uint16_t col;

    if (data == 0 || src_w == 0U || src_h == 0U || w == 0U || h == 0U) {
        return;
    }

    for (row = 0U; row < h; row++) {
        uint16_t sy = (uint16_t)(src_y + row);
        int16_t dy = (int16_t)(y + (int16_t)row);
        if (sy >= src_h || dy < 0 || dy >= (int16_t)LCD_IF_HEIGHT) {
            continue;
        }

        for (col = 0U; col < w; col++) {
            uint16_t sx = (uint16_t)(src_x + col);
            int16_t dx = (int16_t)(x + (int16_t)col);
            if (sx >= src_w || dx < 0 || dx >= (int16_t)LCD_IF_WIDTH) {
                continue;
            }
            LCD_Set_Pixel((uint16_t)dx, (uint16_t)dy, rgb565_to_palette(data[((uint32_t)sy * src_w) + sx]));
        }
    }

    LCD_Refresh(&cfg0);
}

void LCD_DrawIndexedBuffer4BPP(const uint8_t *indices, uint16_t width, uint16_t height, const uint16_t *palette)
{
    uint16_t row;
    uint16_t col;

    if (indices == 0 || palette == 0 || width == 0U || height == 0U) {
        return;
    }

    for (row = 0U; row < height && row < LCD_IF_HEIGHT; row++) {
        for (col = 0U; col < width && col < LCD_IF_WIDTH; col++) {
            uint32_t pixel_index = ((uint32_t)row * width) + col;
            uint8_t packed = indices[pixel_index / 2U];
            uint8_t palette_index = (uint8_t)(((pixel_index & 1U) == 0U) ? (packed >> 4) : (packed & 0x0FU));
            LCD_Set_Pixel(col, row, rgb565_to_palette(palette[palette_index & 0x0FU]));
        }
    }

    LCD_Refresh(&cfg0);
}

void LCD_DrawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    int16_t x0 = x;
    int16_t y0 = y;
    int16_t x1 = (int16_t)(x + (int16_t)w - 1);
    int16_t y1 = (int16_t)(y + (int16_t)h - 1);

    if (w == 0U || h == 0U) {
        return;
    }

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= (int16_t)LCD_IF_WIDTH) {
        x1 = (int16_t)LCD_IF_WIDTH - 1;
    }
    if (y1 >= (int16_t)LCD_IF_HEIGHT) {
        y1 = (int16_t)LCD_IF_HEIGHT - 1;
    }
    if (x1 < x0 || y1 < y0) {
        return;
    }

    LCD_Draw_Rect((uint16_t)x0,
                  (uint16_t)y0,
                  (uint16_t)(x1 - x0 + 1),
                  (uint16_t)(y1 - y0 + 1),
                  rgb565_to_palette(color),
                  1U);
}

void LCD_DrawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t scale)
{
    char text[2];
    text[0] = c;
    text[1] = '\0';
    if (x >= 0 && y >= 0) {
        LCD_printString(text, (uint16_t)x, (uint16_t)y, rgb565_to_palette(color), scale);
    }
}

void LCD_DrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t scale)
{
    if (str != 0 && x >= 0 && y >= 0) {
        LCD_printString((char const*)str, (uint16_t)x, (uint16_t)y, rgb565_to_palette(color), scale);
    }
}
