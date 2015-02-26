#include "lcd.h"
#include <pifacecad.h>

void lcd_init (void) {
    pifacecad_open();
    lcd_backlight_on();
}

void lcd_backlight_on (void) {
    pifacecad_lcd_backlight_on();
}

void lcd_backlight_off (void) {
    pifacecad_lcd_backlight_off();
}

void lcd_showcursor (void) {
    pifacecad_lcd_cursor_on();
}

void lcd_hidecursor (void) {
    pifacecad_lcd_cursor_off();
}

void lcd_home (void) {
    pifacecad_lcd_home();
}

void lcd_setpos (int x, int y) {
    pifacecad_lcd_set_cursor (x, y);
}

void lcd_printf (const char *fmt, ...) {
    char out[2];
    char buffer[4096];
    buffer[0] = 0;
    out[1] = 0;

    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    char *crsr = buffer;
    while (*crsr) {
        if (*crsr == '|') {
            pifacecad_lcd_write_custom_bitmap (0);
        }
        else if (*crsr < 8) {
            pifacecad_lcd_write_custom_bitmap (*crsr);
        }
        else {
            out[0] = *crsr;
            pifacecad_lcd_write (out);
        }
        crsr++;
    }
}
