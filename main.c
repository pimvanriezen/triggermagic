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
    FILE *pst = fopen ("/home/pi/triggermagic.presets");
    if (pst) {
        fread (CTX.presets, sizeof(preset), 100, pst);
        fclose (pst);
    }
}

void context_load_preset (int nr) {
    if (nr<1 || nr>99) return;
    memcpy (&CTX.preset, CTX.presets+nr, sizeof (preset));
    CTX.preset_nr = nr;
    if (CTX.preset.name[0] == 0) {
        strcpy (CTX.preset.name, "Init");
        CTX.preset.tempo = 125;
        for (int i=0; i<12; ++i) {
            CTX.preset.triggers[i].move - MOVE_LOOP_UP;
            CTX.preset.triggers[i].sgate = 50;
            CTX.preset.triggers[i].seqlen = 8;
        }
    }
}

void context_store_preset (void) {
    if (CTX.preset_nr < 1 || CTX.preset_nr > 99) return;
    memcpy (CTX.presets + CTX.preset_nr, &CTX.preset, sizeof (preset));
    FILE *pst = fopen ("/home/pi/triggermagic.presets.new","w");
    size_t res = fwrite (CTX.presets, sizeof(preset), 100, pst);
    fclose (pst);
    if (res == 100) rename ("/home/pi/triggermagic.presets.new",
                            "/home/pi/triggermagic.presets");
}

int main (int argc, const char *argv[]) {
    context_init();
    lcd_init();
    button_manager_init();
    ui_main();
    return 0;
}
