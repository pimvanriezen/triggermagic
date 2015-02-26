#include <stdio.h>
#include <string.h>
#include "btevent.h"
#include "lcd.h"
#include "ui.h"
#include "presets.h"

context_global CTX;

void context_init (void) {
    memset (&CTX, 0, sizeof (CTX));
    strcpy (CTX.presets[1].name, "Rendez-vous    ");
    CTX.presets[1].tempo = 125;
    CTX.presets[1].triggers[0].notes[0] = 48;
    CTX.presets[1].triggers[1].notes[0] = 53;
    CTX.presets[1].triggers[2].notes[0] = 55;
    CTX.presets[1].triggers[3].notes[0] = 56;
    CTX.presets[1].triggers[4].notes[0] = 58;
    CTX.presets[1].triggers[5].notes[0] = 59;
    CTX.presets[1].triggers[6].notes[0] = 60;
    CTX.presets[1].triggers[7].notes[0] = 62;
    CTX.presets[1].triggers[8].notes[0] = 63;
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
    context_init();
    lcd_init();
    button_manager_init();
    ui_main();
    return 0;
}
