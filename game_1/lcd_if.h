#ifndef LCD_IF_H
#define LCD_IF_H

#include <stdint.h>

/* ILI9341 low-level API */
void ILI9341_Init(void);
void ILI9341_WriteCommand(uint8_t cmd);
void ILI9341_WriteData(const uint8_t *data, uint16_t size);

/* TFT LCD (ILI9341 over SPI) abstraction layer */
void LCD_Clear(uint16_t color);
void LCD_DrawBitmap(int16_t x, int16_t y, const uint16_t *data, uint16_t w, uint16_t h);
void LCD_DrawBitmapRegion(int16_t x, int16_t y,
                          const uint16_t *data, uint16_t src_w, uint16_t src_h,
                          uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h);
void LCD_DrawIndexedBuffer4BPP(const uint8_t *indices, uint16_t width, uint16_t height, const uint16_t *palette);
void LCD_DrawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawChar(int16_t x, int16_t y, char c, uint16_t color, uint8_t scale);
void LCD_DrawString(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t scale);

extern const uint8_t LCD_FONT_5X7[95][5];

#endif /* LCD_IF_H */
