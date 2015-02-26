#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ui.h"
#include "lcd.h"
#include "presets.h"
#include "btevent.h"

/** Usable character set for preset names */
const char *CSET = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "0123456789./-!@";

/** Main runner. Jumpst into ui_performance(), then follows the trail left
  * by returns.
  */
void ui_main (void) {
    uifunc call = ui_performance;
    uifunc ncall = NULL;
    lcd_init();
    while (1) {
        ncall = call();
        if (ncall) call = ncall;
    }
}

/** Generic multi-selector.
  * \param curval Current value.
  * \param xpos X position of value display
  * \param ypos Y position of value display
  * \param len Length of value display
  * \param count Number of possible values
  * \param names Array of value names
  * \param values Array of result values for said names
  * \return The ultimately selected value.
  */
int ui_select (int curval, int xpos, int ypos, int len,
               int count, const char *names[], int values[]) {
    char fmtstr[8];
    int valpos = 0;
    while (valpos < count) {
        if (values[valpos] == curval) break;
        valpos++;
    }
    if (valpos == count) valpos = 0;
    sprintf (fmtstr,"%%-%is", len);
    while (1) {
        lcd_hidecursor();
        lcd_setpos (xpos, ypos);
        lcd_printf (fmtstr, names[valpos]);
        lcd_setpos (xpos, ypos);
        lcd_showcursor();
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_MINUS:
            case BTMASK_STK_LEFT:
                if (valpos>0) valpos--;
                break;
            
            case BTMASK_PLUS:
            case BTMASK_STK_RIGHT:
                if ((valpos+1)<len) valpos++;
                break;
            
            case BTMASK_SHIFT:
            case BTMASK_STK_CLICK:
                button_event_free (e);
                lcd_hidecursor();
                return values[valpos];
        } 
        button_event_free (e);
    }
}

/** Implements a generic LCD menu page that shows a multi-choice
  * value that can be edited.
  * \param curval The current real selected value
  * \param prompt The prefix prompt (e.g., "Casualties:")
  * \param count The number of options
  * \param writeto Where to write the choice back to
  * \param names Array of option names
  * \param values Array of option values
  * \param leftresult Which routine to jump to when navigating left.
  * \param rightresult Which routine to jump to when navigating right.
  * \param upresult The routine to jup to when navigating up the menu tree.
  */
void *ui_generic_choice_menu (int curval,
                              const char *prompt,
                              int count,
                              int *writeto,
                              const char *names[],
                              int values[],
                              void *leftresult,
                              void *rightresult,
                              void *upresult) {
    lcd_setpos (0, 1);
    lcd_printf ("%s ", prompt);
    int x = strlen (prompt) +1;
    int len = 16 - x;
    int valpos = 0;
    while (valpos < count) {
        if (values[valpos] == curval) break;
        valpos++;
    }
    if (valpos == count) valpos = 0;
    char fmtstr[8];
    sprintf (fmtstr,"%%-%is", len);
    lcd_setpos (x, 1);
    lcd_printf (fmtstr, names[valpos]);
    lcd_setpos (x,1);
    lcd_hidecursor();
    
    while (1) {
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_MINUS:
            case BTMASK_PLUS:
            case BTMASK_STK_CLICK:
                button_manager_add_event (e->buttons, 0);
                button_event_free (e);
                *writeto = ui_select (curval, x, 1, len, count, names, values);
                break;
                
            case BTMASK_LEFT:
            case BTMASK_STK_LEFT:
                if (leftresult) {
                    button_event_free (e);
                    return leftresult;
                }
                break;
                
            case BTMASK_RIGHT:
            case BTMASK_STK_RIGHT:
                if (rightresult) {
                    button_event_free (e);
                    return rightresult;
                }
                break;
            
            case BTMASK_SHIFT:
                if (upresult) {
                    button_event_free (e);
                    return upresult;
                }
                break;
        }
        button_event_free (e);
    }
}

void* ui_edit_global (void) {
    return ui_performance;
}

/** Note names */
const char *TB_NOTES[12] = {"C-","C#","D-","D#","E-","F-",
                            "F#","G-","G#","A","A#","B-"};

/** Write a note value to the LCD.
  * \param notenr The MIDI note number
  */
void ui_write_note (char notenr) {
    if (notenr == 0) lcd_printf ("---");
    else {
        lcd_printf (TB_NOTES[notenr%12]);
        lcd_printf ("%i ", notenr/12);
    }
}

void *ui_edit_tr_seq_move (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i  Seq\n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int) tpreset->range,
                                   "Move:",
                                   7,
                                   (int*) &tpreset->range,
                                   (const char *[]){
                                        "Single",
                                        "Up",
                                        "Down",
                                        "Up/Down",
                                        "Step Up",
                                        "Step Down",
                                        "Random"
                                   },
                                   (int []){
                                        MOVE_SINGLE,
                                        MOVE_LOOP_UP,
                                        MOVE_LOOP_DOWN,
                                        MOVE_LOOP_UPDOWN,
                                        MOVE_LOOP_STEPUP,
                                        MOVE_LOOP_STEPDOWN,
                                        MOVE_LOOP_RANDOM
                                   },
                                   ui_edit_tr_seq_range,
                                   ui_edit_tr_seq_move,
                                   ui_edit_trig);
}

void *ui_edit_tr_seq_range (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i  Seq\n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int) tpreset->range,
                                   "Range:",
                                   3,
                                   (int *) &tpreset->range,
                                   (const char *[]){
                                        "Orig",
                                        "+1 Oct",
                                        "+2 Oct"
                                   },
                                   (int[]){
                                        RANGE_ORIG,
                                        RANGE_1_OCT,
                                        RANGE_2_OCT
                                   },
                                   ui_edit_tr_seq_gate,
                                   ui_edit_tr_seq_move,
                                   ui_edit_trig);
}

void *ui_edit_tr_seq_gate (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i  Seq\n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int) tpreset->sgate,
                                   "Gate:",
                                   7,
                                   (int *) &tpreset->sgate,
                                   (const char *[]){
                                        "12%",
                                        "25%",
                                        "50%",
                                        "75%",
                                        "100%",
                                        "Rnd Wide",
                                        "Rnd Narrow"
                                   },
                                   (int[]){
                                        12,
                                        25,
                                        50,
                                        75,
                                        100,
                                        SGATE_RND_WIDE,
                                        SGATE_RND_NARROW
                                   },
                                   ui_edit_tr_seq_length,
                                   ui_edit_tr_seq_range,
                                   ui_edit_trig);
}

void *ui_edit_tr_seq_length (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i  Seq\n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->nmode,
                                   "Length:",
                                   4,
                                   (int *)&tpreset->nmode,
                                   (const char *[]){
                                        "1/2",
                                        "1/4",
                                        "1/8",
                                        "1/16"
                                   },
                                   (int[]){
                                        2,
                                        4,
                                        8,
                                        16
                                   },
                                   ui_edit_tr_sendconfig,
                                   ui_edit_tr_seq_gate,
                                   ui_edit_trig);
}

void *ui_edit_tr_notes_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i Note\n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->nmode,
                                   "Mode:",
                                   6,
                                   (int*)&tpreset->nmode,
                                   (const char *[]){
                                        "Gate",
                                        "Fixed 1/2",
                                        "Fixed 1/4",
                                        "Fixed 1/8",
                                        "Fixed 1/16",
                                        "Legato"
                                   },
                                   (int[]){
                                        NMODE_GATE,
                                        NMODE_FIXED_2,
                                        NMODE_FIXED_4,
                                        NMODE_FIXED_8,
                                        NMODE_FIXED_16,
                                        NMODE_LEGATO
                                   },
                                   ui_edit_tr_sendconfig,
                                   ui_edit_tr_notes_mode,
                                   ui_edit_trig);
}

void *ui_edit_prevfrom_tr_sendconfig (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->vconf == VELO_INDIVIDUAL) return ui_edit_tr_velocities;
    return ui_edit_tr_velocity_mode;
}

void *ui_edit_nextfrom_tr_sendconfig (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->send == SEND_NOTES) return ui_edit_tr_notes_mode;
    return ui_edit_tr_seq_length;
}

void *ui_edit_tr_sendconfig (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->send,
                                   "Send:",
                                   2,
                                   (int*)&tpreset->send,
                                   (const char *[]){"Notes","Sequence"},
                                   (int[]){SEND_NOTES,SEND_SEQUENCE},
                                   ui_edit_prevfrom_tr_sendconfig,
                                   ui_edit_nextfrom_tr_sendconfig,
                                   ui_edit_trig);
}



void *ui_edit_velocities (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    int ncursor = 0;

    while (1) {    
        lcd_home();
        int i;
        for (i=0; i<=tpreset->lastnote; ++i) {
            lcd_printf ("%3i ", tpreset->velocities[i]);
            if (i==3) lcd_printf ("\n");
        }
        for (; i<8; ++i) {
            lcd_printf ("    ");
            if (i==3) lcd_printf ("\n");
        }
        lcd_setpos (4*(ncursor&3),ncursor/4);
        lcd_showcursor ();
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_LEFT:
                if (ncursor>0) ncursor--;
                break;
            
            case BTMASK_RIGHT:
                if (ncursor < tpreset->lastnote) ncursor++;
                break;
                
            case BTMASK_MINUS:
            case BTMASK_STK_LEFT:
                if (tpreset->velocities[ncursor]>0) {
                    tpreset->velocities[ncursor]--;
                }
                break;
                
            case BTMASK_PLUS:
            case BTMASK_STK_RIGHT:
                if (tpreset->velocities[ncursor]<120) {
                    tpreset->velocities[ncursor]++;
                }
                break;
                
            case BTMASK_SHIFT:
                button_event_free (e);
                return ui_edit_tr_velocities;
                break;
        }
        button_event_free (e);
        
    }
}

void *ui_edit_tr_velocities (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    lcd_printf ("\002 %3i", tpreset->velocities[0]);
    if (tpreset->lastnote) {
        lcd_printf ("%3i ", tpreset->velocities[1]);
        if (tpreset->lastnote > 1) {
            lcd_printf ("%3i ", tpreset->velocities[2]);
            if (tpreset->lastnote > 2) {
                lcd_printf ("..");
            }
        }
    }
    
    button_event *e = button_manager_wait_event (0);
    switch (e->buttons) {
        case BTMASK_LEFT:
            button_event_free (e);
            return ui_edit_tr_velocity_mode;
        
        case BTMASK_RIGHT:
            button_event_free (e);
            return ui_edit_tr_sendconfig;
        
        case BTMASK_PLUS:
        case BTMASK_MINUS:
        case BTMASK_STK_CLICK:
            button_event_free (e);
            return ui_edit_velocities;
        
        case BTMASK_SHIFT:
            button_event_free (e);
            return ui_edit_trig;
    }
    
    button_event_free (e);
    return ui_edit_tr_notes;
}



/** Determines what menu is to the right of 'velocity mode',
  * depending on the selected mode. If it is set to
  * VELO_INDIVIDUAL, a menu should appear for editing the
  * individual velocities. Otherwise we should skip stright
  * to the send configuration.
  */
void *ui_edit_nextfrom_tr_velocity_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->vconf == VELO_INDIVIDUAL) return ui_edit_tr_velocities;
    return ui_edit_tr_sendconfig;
}

/** Menu for selecting the velocity mode */
void *ui_edit_tr_velocity_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->vconf,
                                   "Velocity:",
                                   6,
                                   (int*)&tpreset->vconf,
                                   (const char *[]){"Copy","Indiv","RndWid",
                                    "RndNrw","Fix64",
                                    "Fix100"},
                                   (int[]){VELO_COPY, VELO_INDIVIDUAL,
                                    VELO_RND_WIDE, VELO_RND_NARROW,
                                    VELO_FIXED_64, VELO_FIXED_100},
                                   ui_edit_tr_notes,
                                   ui_edit_nextfrom_tr_velocity_mode,
                                   ui_edit_trig);
}

/** The chord/sequence note editor. Edits as many notes as configured
  * in the preset under triggerpreset::lastnote.
  */
void *ui_edit_notes (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    int ncursor = 0;

    while (1) {    
        lcd_home();
        int i;
        for (i=0; i<=tpreset->lastnote; ++i) {
            ui_write_note (tpreset->notes[i]);
            if (i==3) lcd_printf ("\n");
        }
        for (; i<8; ++i) {
            lcd_printf ("    ");
            if (i==3) lcd_printf ("\n");
        }
        lcd_setpos (4*(ncursor&3),ncursor/4);
        lcd_showcursor ();
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_LEFT:
                if (ncursor>0) ncursor--;
                break;
            
            case BTMASK_RIGHT:
                if (ncursor < tpreset->lastnote) ncursor++;
                break;
                
            case BTMASK_MINUS:
            case BTMASK_STK_LEFT:
                if (tpreset->notes[ncursor]>0) {
                    tpreset->notes[ncursor]--;
                }
                break;
                
            case BTMASK_PLUS:
            case BTMASK_STK_RIGHT:
                if (tpreset->notes[ncursor]<120) {
                    tpreset->notes[ncursor]++;
                }
                break;
                
            case BTMASK_SHIFT:
                button_event_free (e);
                return ui_edit_tr_notes;
                break;
        }
        button_event_free (e);
        
    }
}

/** Displayes up to the first three notes of the chord / sequence.
  * Editing is relayed to ui_edit_notes().
  */                          
void *ui_edit_tr_notes (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    lcd_printf ("\001 ");
    ui_write_note (tpreset->notes[0]);
    if (tpreset->lastnote) {
        ui_write_note (tpreset->notes[1]);
        if (tpreset->lastnote > 1) {
            ui_write_note (tpreset->notes[2]);
            if (tpreset->lastnote > 2) {
                lcd_printf ("..");
            }
            else lcd_printf ("  ");
        }
        else lcd.printf ("      ");
    }
    else lcd_printf ("          ");
    lcd_hidecursor();
    
    button_event *e = button_manager_wait_event (0);
    switch (e->buttons) {
        case BTMASK_LEFT:
            button_event_free (e);
            return ui_edit_tr_notecount;
        
        case BTMASK_RIGHT:
            button_event_free (e);
            return ui_edit_tr_velocity_mode;
        
        case BTMASK_PLUS:
        case BTMASK_MINUS:
        case BTMASK_STK_CLICK:
            button_event_free (e);
            return ui_edit_notes;
        
        case BTMASK_SHIFT:
            button_event_free (e);
            return ui_edit_trig;
    }
    
    button_event_free (e);
    return ui_edit_tr_notes;
}

/** Displays the number of notes in the chord/sequence for editing */
void *ui_edit_tr_notecount (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->lastnote,
                                   "# Notes:",
                                   8,
                                   (int*)&tpreset->lastnote,
                                   (const char *[])
                                        {"1","2","3","4","5","6","7","8"},
                                   (int[]){0,1,2,3,4,5,6,7},
                                   NULL,
                                   ui_edit_tr_notes,
                                   ui_edit_trig);
}

/** Trigger selection menu. */
void *ui_edit_trig (void) {
    lcd_home();
    lcd_printf ("Trigger: %02i      \n                ", CTX.trigger_nr+1);
    
    button_event *e = button_manager_wait_event (0);
    switch (e->buttons) {
        case BTMASK_MINUS:
        case BTMASK_STK_LEFT:
            if (CTX.trigger_nr>0) CTX.trigger_nr--;
            break;
        
        case BTMASK_PLUS:
        case BTMASK_STK_RIGHT:
            if (CTX.trigger_nr<11) CTX.trigger_nr++;
            break;
        
        case BTMASK_RIGHT:
            button_event_free (e);
            return ui_edit_tr_notes;
            
        case BTMASK_SHIFT:
            button_event_free (e);
            return ui_edit_main;
    }
    
    button_event_free (e);
    return ui_edit_trig;
}

/** Preset name editor */
void *ui_edit_name (void) {
    uint8_t crsr = 0;
    while (1) {
        lcd_home();
        lcd_printf ("%-13s   \n                ", CTX.preset.name);
        lcd_setpos (crsr,0);
        lcd_showcursor ();
        
        if (! CTX.preset.name[crsr]) {
            CTX.preset.name[crsr] = ' ';
            CTX.preset.name[crsr+1] = 0;
        }
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_LEFT:
                if (crsr>0) crsr--;
                break;
            
            case BTMASK_RIGHT:
                if (crsr<12) crsr++;
                break;
            
            case BTMASK_STK_LEFT:
            case BTMASK_MINUS:
                if (CTX.preset.name[crsr] != ' ') {
                    const char *pos = strchr (CSET, CTX.preset.name[crsr]);
                    if (! pos) {
                        CTX.preset.name[crsr] = ' ';
                        break;
                    }
                    CTX.preset.name[crsr] = *(pos-1);
                }
                break;
            
            case BTMASK_STK_RIGHT:
            case BTMASK_PLUS:
                if (CTX.preset.name[crsr] != '@') {
                    const char *pos = strchr (CSET, CTX.preset.name[crsr]);
                    if (! pos) {
                        CTX.preset.name[crsr] = '@';
                        break;
                    }
                    CTX.preset.name[crsr] = *(pos+1);
                }
                break;
            
            case BTMASK_SHIFT:
                button_event_free (e);
                lcd_hidecursor();
                return ui_edit_main;
        }
        button_event_free (e);
    }
}

/** Edit main menu */
void *ui_edit_main (void) {
    uint8_t choice = 0;
    const char *ch_name[3] = {"Edit name","Triggers","Global"};
    uifunc ch_jump[3] = {ui_edit_name, ui_edit_trig, ui_edit_global};
    while (1) {
        lcd_home();
        lcd_printf ("%02i|%-13s\n  |%-13s",   
                    CTX.preset_nr,
                    CTX.preset.name,
                    ch_name[choice]);
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_STK_RIGHT:
            case BTMASK_RIGHT:
                choice = choice+1;
                if (choice>2) choice = 0;
                break;
            
            case BTMASK_STK_LEFT:
            case BTMASK_LEFT:
                choice = choice-1;
                if (choice<0) choice = 2;
                break;
            
            case BTMASK_STK_CLICK:
            case BTMASK_PLUS:
            case BTMASK_MINUS:
                button_event_free (e);
                return ch_jump[choice];
            
            case BTMASK_SHIFT:
                button_event_free (e);
                context_store_preset();
                return ui_performance;
        }
        button_event_free (e);
    }
}

/** Main performance menu */
void *ui_performance (void) {
    lcd_home();
    lcd_printf ("%02i|%-13s\n  |\001 %3i     %s%c%i",   
                CTX.preset_nr,
                CTX.preset.name,
                CTX.preset.tempo,
                (CTX.transpose<-9||CTX.transpose>9)?"":" ",
                CTX.transpose<0?'-':'+',
                CTX.transpose<0?-CTX.transpose:CTX.transpose);
    
    button_event *e = button_manager_wait_event (1);
    switch (e->buttons) {
        case BTMASK_SHIFT | BTMASK_RIGHT:
            if (CTX.transpose < 25) {
                CTX.transpose++;
            }
            break;

        case BTMASK_SHIFT | BTMASK_LEFT:
            if (CTX.transpose > -25) {
                CTX.transpose--;
            }
            break;
        
        case BTMASK_STK_RIGHT:
        case BTMASK_RIGHT:
            if (CTX.preset_nr < 99) {
                context_load_preset (CTX.preset_nr + 1);
            }
            break;
        
        case BTMASK_STK_LEFT:
        case BTMASK_LEFT:
            if (CTX.preset_nr > 1) {
                context_load_preset (CTX.preset_nr - 1);
            }
            break;
            
        case BTMASK_PLUS:
            if (CTX.preset.tempo < 257) {
                CTX.preset.tempo++;
            }
            break;
        
        case BTMASK_MINUS:
            if (CTX.preset.tempo > 41) {
                CTX.preset.tempo--;
            }
            break;
        
        case BTMASK_STK_CLICK:
        case BTMASK_SHIFT | BTMASK_MINUS:
            button_event_free (e);
            return ui_edit_main;
            break;
    }
    
    button_event_free (e);
    return ui_performance;
}
