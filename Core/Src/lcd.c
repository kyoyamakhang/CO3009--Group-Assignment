/**
 * @file lcd.c
 * @brief PCF8574 LCD driver, number formatting, and line caching.
 * @details Uses 4-bit transfers and skips rows whose cached text is unchanged.
 */

#include "lcd.h"
#include "i2c_bus.h"
#include <string.h>

/* Each command or character uses one four-byte I2C transaction:
 * [high | enable], [high], [low | enable], [low].
 */

/* PCF8574 control bits. */
#define LCD_RS          0x01u
#define LCD_RW          0x02u   /* Always clear: write-only operation. */
#define LCD_EN          0x04u

static uint8_t lcd_address = LCD_I2C_ADDRESS; /* Active 7-bit address. */
static uint8_t lcd_backlight_state = LCD_BACKLIGHT_ON;
static uint8_t lcd_ready = 0u;
static uint8_t lcd_cursor_row = 0u; /* Current cursor row. */
static char lcd_shadow[LCD_ROWS][LCD_COLS]; /* Last successfully displayed text. */
static uint8_t lcd_shadow_valid[LCD_ROWS]; /* Set when the cached row matches the LCD. */

static void lcd_delay_us(uint32_t duration_us)
{
    volatile uint32_t iteration_count = duration_us * (SystemCoreClock / 4000000u) + 1u;

    while (iteration_count > 0u)
    {
        iteration_count--;
    }
}

static void lcd_bus_write(const uint8_t *buffer, uint16_t len)
{
    if (i2c_bus_write(lcd_address, buffer, len, LCD_I2C_TIMEOUT) != I2C_BUS_OK)
    {
        lcd_ready = 0u; /* Mark the LCD unavailable after a failed write. */
    }
}

static void lcd_invalidate(void)
{
    uint8_t row;

    for (row = 0u; row < LCD_ROWS; row++)
    {
        lcd_shadow_valid[row] = 0u;
    }
}

/* Latch one nibble during the initialization sequence. */
static void lcd_write_nibble(uint8_t nibble)
{
    uint8_t data = (uint8_t)(((nibble & 0x0Fu) << 4) | lcd_backlight_state);
    uint8_t buffer[2];

    buffer[0] = (uint8_t)(data | LCD_EN);
    buffer[1] = data;
    lcd_bus_write(buffer, 2u);
}

/* Send one command or data byte as two enable pulses. */
static void lcd_write_byte(uint8_t value, uint8_t register_select)
{
    uint8_t control_bits = (uint8_t)(lcd_backlight_state | ((register_select != 0u) ? LCD_RS : 0u));
    uint8_t high_nibble = (uint8_t)((value & 0xF0u) | control_bits);
    uint8_t low_nibble = (uint8_t)(((uint8_t)(value << 4) & 0xF0u) | control_bits);
    uint8_t buffer[4];

    buffer[0] = (uint8_t)(high_nibble | LCD_EN);
    buffer[1] = high_nibble;
    buffer[2] = (uint8_t)(low_nibble | LCD_EN);
    buffer[3] = low_nibble;
    lcd_bus_write(buffer, 4u);
}

static void lcd_cmd(uint8_t command)
{
    lcd_write_byte(command, 0u);
    if ((command == 0x01u) || (command == 0x02u))
    {
        lcd_delay_us(2000u); /* Clear and return-home commands need an extended delay. */
    }
}

static void lcd_data(uint8_t data)
{
    lcd_write_byte(data, 1u);
}

/* Each initialization probes the configured addresses, then the fallback addresses. */
void lcd_init(void)
{
    /* Probe the LCD backpack ranges, excluding the DHT20 address at 0x38. */
    static const uint8_t candidates[] = {
        LCD_I2C_ADDRESS, LCD_I2C_ADDRESS_ALT,
        0x26u, 0x25u, 0x24u, 0x23u, 0x22u, 0x21u, 0x20u,
        0x3Eu, 0x3Du, 0x3Cu, 0x3Bu, 0x3Au, 0x39u
    };
    uint8_t index;
    uint8_t found = 0u;

    lcd_backlight_state = LCD_BACKLIGHT_ON;
    lcd_ready = 0u;
    lcd_invalidate();
    lcd_cursor_row = 0u;

    for (index = 0u; index < (uint8_t)sizeof(candidates); index++)
    {
        if (i2c_bus_probe_fast(candidates[index]) == I2C_BUS_OK)
        {
            lcd_address = candidates[index];
            found = 1u;
            break;
        }
    }
    if (found == 0u)
    {
        return; /* Leave the LCD unavailable if no address responds. */
    }
    lcd_ready = 1u;

    HAL_Delay(50); /* Allow the LCD supply to settle. */

    /* Enter HD44780 4-bit mode using the existing initialization sequence. */
    lcd_write_nibble(0x03u);
    HAL_Delay(5);
    lcd_write_nibble(0x03u);
    HAL_Delay(2);
    lcd_write_nibble(0x03u);
    HAL_Delay(2);
    lcd_write_nibble(0x02u);
    HAL_Delay(2); /* Select 4-bit mode. */

    lcd_cmd(0x28u); /* 4-bit interface, two lines, 5 x 8 font. */
    lcd_cmd(0x0Cu); /* Display on, cursor off. */
    lcd_cmd(0x06u); /* Increment the address without shifting the display. */
    lcd_cmd(0x01u); /* Clear the display. */
}

uint8_t lcd_is_present(void)
{
    return (i2c_bus_probe_fast(lcd_address) == I2C_BUS_OK) ? 1u : 0u;
}

uint8_t lcd_is_ready(void)
{
    return lcd_ready;
}

uint8_t lcd_get_address(void)
{
    return lcd_address;
}

void lcd_backlight(uint8_t on)
{
    uint8_t v;

    lcd_backlight_state = (on != 0u) ? LCD_BACKLIGHT_ON : LCD_BACKLIGHT_OFF;
    v = lcd_backlight_state;
    if (lcd_ready != 0u)
    {
        lcd_bus_write(&v, 1u);
    }
}

void lcd_clear(void)
{
    if (lcd_ready == 0u)
    {
        return;
    }
    lcd_cmd(0x01u);
    lcd_invalidate();
    lcd_cursor_row = 0u;
}

void lcd_goto_XY(uint8_t row, uint8_t col)
{
    uint8_t addr;

    if (lcd_ready == 0u)
    {
        return;
    }
    if (row >= LCD_ROWS)
    {
        row = 0u;
    }
    if (col >= LCD_COLS)
    {
        col = (uint8_t)(LCD_COLS - 1u);
    }

    addr = (row == 0u) ? col : (uint8_t)(0x40u + col);
    lcd_cmd((uint8_t)(0x80u | addr)); /* Set DDRAM address */
    lcd_cursor_row = row;
}

/* Direct character writes invalidate the cached content of the cursor row. */
void lcd_putchar(char character)
{
    if (lcd_ready == 0u)
    {
        return;
    }
    lcd_shadow_valid[lcd_cursor_row] = 0u;
    lcd_data((uint8_t)character);
}

void lcd_puts(const char *string)
{
    while ((string != 0) && (*string != '\0'))
    {
        lcd_putchar(*string);
        string++;
    }
}

/* Format into a buffer of at least 24 bytes; width includes a minus sign. */
static uint8_t lcd_fmt_int(char *destination, int32_t number, uint8_t width, char pad)
{
    char digits[11];
    uint8_t digit_count = 0u;
    uint8_t len = 0u;
    uint8_t is_negative = (number < 0) ? 1u : 0u;
    uint8_t total_width;
    uint32_t magnitude = (is_negative != 0u) ? ((uint32_t)0u - (uint32_t)number) : (uint32_t)number;

    do
    {
        digits[digit_count++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while ((magnitude > 0u) && (digit_count < 10u));

    total_width = (uint8_t)(digit_count + is_negative);

    if (pad == '0')
    {
        /* Place the sign before zero padding, for example -007. */
        if (is_negative != 0u)
        {
            destination[len++] = '-';
        }
        for (; total_width < width; total_width++)
        {
            destination[len++] = '0';
        }
    }
    else
    {
        /* Place the sign after space padding. */
        for (; total_width < width; total_width++)
        {
            destination[len++] = pad;
        }
        if (is_negative != 0u)
        {
            destination[len++] = '-';
        }
    }
    while (digit_count > 0u)
    {
        destination[len++] = digits[--digit_count];
    }
    destination[len] = '\0';
    return len;
}

static uint8_t lcd_fmt_float(char *destination, float value, uint8_t decimals)
{
    int32_t scale = 1;
    int32_t scaled;
    int32_t magnitude;
    uint8_t index;
    uint8_t len = 0u;

    if (decimals > 3u)
    {
        decimals = 3u;
    }
    for (index = 0u; index < decimals; index++)
    {
        scale *= 10;
    }

    /* Limit the magnitude before conversion to a scaled integer. */
    if (value > 999999.0f)
    {
        value = 999999.0f;
    }
    if (value < -999999.0f)
    {
        value = -999999.0f;
    }

    scaled = (int32_t)((value * (float)scale) + ((value >= 0.0f) ? 0.5f : -0.5f));
    if (scaled < 0)
    {
        destination[len++] = '-';
        magnitude = -scaled;
    }
    else
    {
        magnitude = scaled;
    }

    len = (uint8_t)(len + lcd_fmt_int(&destination[len], magnitude / scale, 0u, ' '));
    if (decimals > 0u)
    {
        destination[len++] = '.';
        len = (uint8_t)(len + lcd_fmt_int(&destination[len], magnitude % scale, decimals, '0'));
    }
    destination[len] = '\0';
    return len;
}

void lcd_put_num(int32_t number)
{
    char text[24];

    (void)lcd_fmt_int(text, number, 0u, ' ');
    lcd_puts(text);
}

void lcd_put_num_pad(int32_t number, uint8_t width)
{
    char text[24];

    if (width > LCD_COLS)
    {
        width = LCD_COLS;
    }
    (void)lcd_fmt_int(text, number, width, '0');
    lcd_puts(text);
}

void lcd_put_float(float value, uint8_t decimals)
{
    char text[24];

    (void)lcd_fmt_float(text, value, decimals);
    lcd_puts(text);
}

void lcd_line_begin(LcdLine_t *line)
{
    line->len = 0u;
    line->buf[0] = '\0';
}

void lcd_line_char(LcdLine_t *line, char character)
{
    if (line->len < LCD_COLS)
    {
        line->buf[line->len] = character;
        line->len++;
        line->buf[line->len] = '\0';
    }
}

void lcd_line_str(LcdLine_t *line, const char *string)
{
    while ((string != 0) && (*string != '\0'))
    {
        lcd_line_char(line, *string);
        string++;
    }
}

void lcd_line_int(LcdLine_t *line, int32_t number, uint8_t width, char pad)
{
    char text[24];

    if (width > LCD_COLS)
    {
        width = LCD_COLS;
    }
    (void)lcd_fmt_int(text, number, width, pad);
    lcd_line_str(line, text);
}

void lcd_line_float(LcdLine_t *line, float value, uint8_t decimals)
{
    char text[24];

    (void)lcd_fmt_float(text, value, decimals);
    lcd_line_str(line, text);
}

void lcd_fill_line(uint8_t row, const char *string)
{
    char text[LCD_COLS];
    uint8_t index;

    if ((lcd_ready == 0u) || (row >= LCD_ROWS))
    {
        return;
    }

    for (index = 0u; index < LCD_COLS; index++)
    {
        /* Clip or space-pad the text to the LCD width. */
        if ((string != 0) && (*string != '\0'))
        {
            text[index] = *string;
            string++;
        }
        else
        {
            text[index] = ' ';
        }
    }

    if ((lcd_shadow_valid[row] != 0u) && (memcmp(text, lcd_shadow[row], LCD_COLS) == 0))
    {
        return; /* Skip a row that already matches the display. */
    }

    lcd_goto_XY(row, 0u);
    for (index = 0u; index < LCD_COLS; index++)
    {
        lcd_data((uint8_t)text[index]);
    }
    if (lcd_ready != 0u)
    {
        /* Update the cache only when all writes have succeeded. */
        memcpy(lcd_shadow[row], text, LCD_COLS);
        lcd_shadow_valid[row] = 1u;
    }
}

void lcd_show_line(uint8_t row, const LcdLine_t *line)
{
    lcd_fill_line(row, line->buf);
}
