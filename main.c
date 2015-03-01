#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    CTX.trigger_type = TYPE_ROLAND_TR8;
    context_load_preset (1);
    
    for (int cpre=2; cpre<100; ++cpre) {
        strcpy (CTX.presets[cpre].name, "Init         ");
        CTX.presets[cpre].tempo = 125;
        for (int i=0; i<12; ++i) {
            triggerpreset *tp = &CTX.presets[cpre].triggers[i];
            tp->slen = 8;
            tp->move = MOVE_LOOP_UP;
            tp->notes[0] = 48+i;
        }
    }
    
    FILE *pst = fopen ("/boot/tmpreset.dat","r");
    if (pst) {
        fread (CTX.presets, sizeof(preset), 100, pst);
        fclose (pst);
    }
    
    pst = fopen ("/boot/tmglobal.dat","r");
    if (pst) {
        char buf[1024];
        while (! feof (pst)) {
            buf[0] = 0;
            fgets (buf, 255, pst);
            buf[255] = 0;
            if (*buf == 0) continue;
            char *l = buf + strlen(buf)-1;
            if (*l == '\n') *l = 0;
            
            if (strncmp (buf, "inport:", 7) == 0) {
                strcpy (CTX.portname_midi_in, buf+7);
            }
            else if (strncmp (buf, "outport:", 8) == 0) {
                strcpy (CTX.portname_midi_out, buf+8);
            }
            else if (strncmp (buf, "triggertype:", 12) == 0) {
                CTX.trigger_type = (triggertype) atoi (buf+12);
            }
            else if (strncmp (buf, "sendchannel:", 12) == 0) {
                CTX.send_channel = atoi (buf+12) - 1;
            }
        }
        fclose (pst);
    }
}

void contex_write_global (void) {
    FILE *f = fopen ("/boot/tmglobal.new","w");
    if (! f) return;
    fprintf (f, "inport:%s\n", CTX.portname_midi_in);
    fprintf (f, "outport:%s\n", CTX.portname_midi_out);
    fprintf (f, "triggertype:%i\n", (int) CTX.trigger_type);
    fprintf (f, "sendchannel:%i\n", CTX.send_channel);
    fclose (f);
    rename ("/boot/tmglobal.new","/boot/tmglobal.dat");
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
            CTX.preset.triggers[i].slen = 8;
        }
    }
}

void context_store_preset (void) {
    if (CTX.preset_nr < 1 || CTX.preset_nr > 99) return;
    memcpy (CTX.presets + CTX.preset_nr, &CTX.preset, sizeof (preset));
    FILE *pst = fopen ("/boot/tmpreset.new","w");
    size_t res = fwrite (CTX.presets, sizeof(preset), 100, pst);
    fclose (pst);
    if (res == 100) rename ("/boot/tmpreset.new",
                            "/boot/tmpreset.dat");
}

int main (int argc, const char *argv[]) {
    context_init();
    lcd_init();
    button_manager_init();
    ui_main();
    return 0;
}
