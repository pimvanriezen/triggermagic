#include "lcd.h"
#include <pifacecad.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

/** Initialize the piface LCD subsystem, upload custom characters */
void lcd_init (void) {
    uint8_t sym_div[] = {8,0,8,0,8,0,8,0};
    uint8_t sym_qnote[] = {2,2,2,2,14,30,12,0};
    uint8_t sym_velo[] = {4,4,4,4,21,14,4,0};
    uint8_t sym_spk1[] = {1,3,31,31,31,3,1,0};
    uint8_t sym_spk2[] = {	2,9,5,21,5,9,2,0};
    uint8_t sym_midi_in[] = {	0,0,0,0,0,31,14,4};
    uint8_t sym_midi_out[] = {  0,0,0,0,0,4,14,31};   
    pifacecad_open();
    lcd_backlight_on();
    pifacecad_lcd_store_custom_bitmap (0, sym_div);
    pifacecad_lcd_store_custom_bitmap (1, sym_qnote);
    pifacecad_lcd_store_custom_bitmap (2, sym_velo);
    pifacecad_lcd_store_custom_bitmap (3, sym_spk1);
    pifacecad_lcd_store_custom_bitmap (4, sym_spk2);
    pifacecad_lcd_store_custom_bitmap (5, sym_midi_in);
    pifacecad_lcd_store_custom_bitmap (6, sym_midi_out);
}

/** Turn on LCD backlight */
void lcd_backlight_on (void) {
    pifacecad_lcd_backlight_on();
}

/** Turn off LCD backlight */
void lcd_backlight_off (void) {
    pifacecad_lcd_backlight_off();
}

/** Turn on visible cursor */
void lcd_showcursor (void) {
    pifacecad_lcd_cursor_on();
    pifacecad_lcd_blink_on();
}

/** Turn off visible cursor */
void lcd_hidecursor (void) {
    pifacecad_lcd_cursor_off();
    pifacecad_lcd_blink_off();
}

/** Move LCD cursor to home position */
void lcd_home (void) {
    pifacecad_lcd_home();
}

/** Move LCD cursor to absolut position */
void lcd_setpos (int x, int y) {
    pifacecad_lcd_set_cursor (x, y);
}

/** Write a formatted string to the LCD. Also replaces all pipe-
  * symbols "|" into a custom separator, and the control characters
  * 1 through 7 to their respective numbered custom symbols */
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
