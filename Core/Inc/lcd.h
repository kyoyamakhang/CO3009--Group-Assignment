#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdint.h>

#define LCD_ROWS            2u
#define LCD_COLS            16u
#define LCD_I2C_ADDRESS     0x27u // PCF8574  (shifted left by 1 bit in i2c_bus)
#define LCD_I2C_ADDRESS_ALT 0x3Fu // PCF8574A (lcd_init tries both addresses)
#define LCD_I2C_TIMEOUT     100u

#define LCD_BACKLIGHT_ON    0x08u
#define LCD_BACKLIGHT_OFF   0x00u

typedef struct {
    char buf[LCD_COLS + 1];
    uint8_t len;
} LcdLine_t;

void lcd_init(void);               /* finds the address automatically; can be called again to retry */
uint8_t lcd_is_present(void);
uint8_t lcd_is_ready(void);
uint8_t lcd_get_address(void);     /* 7-bit address in use */
void lcd_backlight(uint8_t on);
void lcd_clear(void);
void lcd_goto_XY(uint8_t row, uint8_t col);
void lcd_putchar(char c);
void lcd_puts(const char *s);
void lcd_put_num(int32_t num);
void lcd_put_num_pad(int32_t num, uint8_t width);
void lcd_put_float(float value, uint8_t decimals);
void lcd_line_begin(LcdLine_t *line);
void lcd_line_char(LcdLine_t *line, char c);
void lcd_line_str(LcdLine_t *line, const char *s);
void lcd_line_int(LcdLine_t *line, int32_t num, uint8_t width, char pad);
void lcd_line_float(LcdLine_t *line, float value, uint8_t decimals);
void lcd_fill_line(uint8_t row, const char *s);
void lcd_show_line(uint8_t row, const LcdLine_t *line);

#endif /* INC_LCD_H_ */
