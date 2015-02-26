#include <stdio.h>
#include "btevent.h"
#include "lcd.h"
#include "ui.h"

int main (int argc, const char *argv[]) {
    lcd_init();
    button_manager_init();
    ui_main();
    return 0;
}
