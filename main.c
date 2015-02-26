#include <stdio.h>
#include <string.h>
#include "btevent.h"
#include "lcd.h"
#include "ui.h"

context_global CTX;

void context_init (void) {
    memset (&CTX, 0, sizeof (CTX));
    strcpy (CTX.presets[1].name, "Rendez-vous    ");
    CTX.presets[1].tempo = 125;
    CTX.presets[1].triggers[0].notes[0] = 48;
    context_load_preset (1);
}

void context_load_preset (int nr) {
    if (nr<1 || nr>99) return;
    memcpy (&CTX.preset, CTX.presets+nr, sizeof (preset));
    CTX.preset_nr = nr;
}

void context_store_preset (void) {
    if (CTX.preset_nr < 1 || CTX.preset_nr > 99) return;
    memcpy (CTX.presets + CTX.preset_nr, &CTX.preset, sizeof (preset));
}

int main (int argc, const char *argv[]) {
    lcd_init();
    button_manager_init();
    ui_main();
    return 0;
}
