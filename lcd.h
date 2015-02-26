#ifndef _LCD_H
#define _LCD_H 1

/* ============================= FUNCTIONS ============================= */

void     lcd_init (void);
void     lcd_backlight_on (void);
void     lcd_backlight_off (void);
void     lcd_showcursor (void);
void     lcd_hidecursor (void);
void     lcd_home (void);
void     lcd_setpos (int, int);
void     lcd_printf (const char *, ...);

#endif
