#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "ui.h"
#include "lcd.h"
#include "presets.h"
#include "btevent.h"
#include "midi.h"

/** Usable character set for preset names */
const char *CSET = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "0123456789./-!@";

static void *last_edit_page = ui_edit_tr_notecount;

/** Main runner. Jumpst into ui_performance(), then follows the trail left
  * by returns.
  */
void ui_main (void) {
    uifunc call = ui_splash;
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
                if ((valpos+1)<count) valpos++;
                break;
            
            case BTMASK_SHIFT:
            case BTMASK_STK_CLICK:
                button_event_free (e);
                lcd_hidecursor();
                return values[valpos];
                
            case BTMASK_LEFT:
            case BTMASK_RIGHT:
                /* Interpret these lateral moves as a desire to stop
                   editing and continue navigating. So jump back
                   and reinsert the events */
                button_manager_add_event (e->buttons, 0);
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
                              void *upresult,
                              uifunc cb) {
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
    lcd_hidecursor();
    
    while (1) {
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_MINUS:
            case BTMASK_PLUS:
            case BTMASK_STK_CLICK:
                button_manager_add_event (e->buttons, 0);
                *writeto = ui_select (curval, x, 1, len, count, names, values);
                if (cb) return cb();
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

void *ui_save_global (void) {
    context_write_global();
    return ui_edit_main;
}

void *ui_edit_global_sync (void) {
    lcd_home();
    lcd_printf ("System Setup       \n");
    return ui_generic_choice_menu ((int) CTX.ext_sync,
                                   "Ext Sync:",
                                   2,
                                   (int*) &CTX.ext_sync,
                                   (const char *[]){"Off","On"},
                                   (int []){0,1},
                                   ui_edit_global_channel,
                                   NULL,
                                   ui_save_global,
                                   NULL);
}

void *ui_edit_global_channel (void) {
    lcd_home();
    lcd_printf ("System Setup       \n");
    return ui_generic_choice_menu ((int) CTX.send_channel,
                                   "Out Channel:",
                                   16,
                                   (int*) &CTX.send_channel,
                                   (const char *[]){
                                    "1","2","3","4","5","6","7","8","9",
                                    "10","11","12","13","14","15","16"
                                   },
                                   (int []){
                                     0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
                                   },
                                   ui_edit_global_triggertype,
                                   ui_edit_global_sync,
                                   ui_save_global,
                                   NULL);
}


void *ui_edit_global_triggertype (void) {
        lcd_home();
        lcd_printf ("System Setup       \n");
    return ui_generic_choice_menu ((int) CTX.trigger_type,
                                   "Type:",
                                   6,
                                   (int*) &CTX.trigger_type,
                                   (const char *[]){
                                        "Roland TR8",
                                        "Laserharp8",
                                        "Laserharp9",
                                        "Laserhar10",
                                        "Chromatic",
                                        "Pedals 7"
                                   },
                                   (int []){
                                        TYPE_ROLAND_TR8,
                                        TYPE_LASERHARP_8,
                                        TYPE_LASERHARP_9,
                                        TYPE_LASERHARP_10,
                                        TYPE_CHROMATIC,
                                        TYPE_PEDALS_7
                                   },
                                   ui_edit_global,
                                   ui_edit_global_channel,
                                   ui_save_global,
                                   NULL);
}

/** Placeholder */
void* ui_edit_global (void) {
    while (1) {
        lcd_home();
        lcd_printf ("System Setup       \n%-16s",   
                    "Firmware v1.0.1");
                    
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_STK_RIGHT:
            case BTMASK_RIGHT:
                return ui_edit_global_triggertype;
            
            case BTMASK_STK_LEFT:
            case BTMASK_LEFT:
                break;
            
            case BTMASK_STK_CLICK:
            case BTMASK_PLUS:
            case BTMASK_MINUS:
                break;
            
            case BTMASK_SHIFT:
                button_event_free (e);
                return ui_save_global;
        }
        button_event_free (e);
    }
}

/** Note names */
const char *TB_NOTES[12] = {"C-","C#","D-","D#","E-","F-",
                            "F#","G-","G#","A-","A#","B-"};

/** Write a note value to the LCD.
  * \param notenr The MIDI note number
  */
void ui_write_note (char notenr) {
    if (notenr == 0) lcd_printf ("--- ");
    else {
        lcd_printf (TB_NOTES[notenr%12]);
        lcd_printf ("%i ", notenr/12);
    }
}

void *ui_edit_prevfrom_tr_copy (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->send == SEND_SEQUENCE) return ui_edit_tr_seq_move;
    else return ui_edit_tr_notes_mode;
}

static int ui_edit_tr_copyfrom = -1;

void *ui_handle_tr_copy (void) {
    int copyfrom = ui_edit_tr_copyfrom;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (copyfrom >= 0) {
        memcpy (tpreset, CTX.preset.triggers + copyfrom,
                sizeof (triggerpreset));
        lcd_setpos (0,1);
        lcd_printf ("Trigger copied..");
        sleep (1);
    }
    return NULL;
}

void *ui_edit_tr_copy (void) {
    ui_edit_tr_copyfrom = -1;
    void *rval = NULL;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i       \n", CTX.trigger_nr+1);
    rval = ui_generic_choice_menu (ui_edit_tr_copyfrom,
                                   "Copy From:",
                                   13,
                                   &ui_edit_tr_copyfrom,
                                   (const char *[]){
                                        "","1","2","3","4","5","6","7","8",
                                        "9","10","11","12"
                                   },
                                   (int []){
                                        -1,0,1,2,3,4,5,6,7,8,9,10,11,12
                                   },
                                   ui_edit_prevfrom_tr_copy,
                                   NULL,
                                   ui_edit_trig,
                                   ui_handle_tr_copy);
}

/** Menu for the sequence move parameter */
void *ui_edit_tr_seq_move (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i    ", CTX.trigger_nr+1);
    lcd_setpos (11,0);
    lcd_printf ("[seq]\n");
    return ui_generic_choice_menu ((int) tpreset->move,
                                   "Move:",
                                   7,
                                   (int*) &tpreset->move,
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
                                   ui_edit_tr_copy,
                                   ui_edit_trig,
                                   NULL);
}

/** Menu for the sequence range parameter */
void *ui_edit_tr_seq_range (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i    ", CTX.trigger_nr+1);
    lcd_setpos (11,0);
    lcd_printf ("[seq]\n");
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
                                   ui_edit_trig,
                                   NULL);
}

/** Menu for the sequence gate parameter */
void *ui_edit_tr_seq_gate (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i    ", CTX.trigger_nr+1);
    lcd_setpos (11,0);
    lcd_printf ("[seq]\n");
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
                                   ui_edit_trig,
                                   NULL);
}

/** Menu for the sequence note length parameter */
void *ui_edit_tr_seq_length (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i    ", CTX.trigger_nr+1);
    lcd_setpos (11,0);
    lcd_printf ("[seq]\n");
    return ui_generic_choice_menu ((int)tpreset->slen,
                                   "Length:",
                                   4,
                                   (int *)&tpreset->slen,
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
                                   ui_edit_trig,
                                   NULL);
}

/** Menu for the note trigger mode parameter */
void *ui_edit_tr_notes_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i    ", CTX.trigger_nr+1);
    lcd_setpos (10,0);
    lcd_printf ("[note]\n");
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
                                   ui_edit_tr_copy,
                                   ui_edit_trig,
                                   NULL);
}

/** Determine previous menu from trigger send config. Depends on whether
  * individual velocities should be shown. */
void *ui_edit_prevfrom_tr_sendconfig (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->vconf == VELO_INDIVIDUAL) return ui_edit_tr_velocities;
    return ui_edit_tr_velocity_mode;
}

/** Determines next menu from trigger send config. Depends on whether
  * configured to send notes or a sequence. */
void *ui_edit_nextfrom_tr_sendconfig (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->send == SEND_NOTES) return ui_edit_tr_notes_mode;
    return ui_edit_tr_seq_length;
}

/** Menu for the note/sequence send configuration menu. */
void *ui_edit_tr_sendconfig (void) {
    last_edit_page = ui_edit_tr_sendconfig;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i          ", CTX.trigger_nr+1);
    return ui_generic_choice_menu ((int)tpreset->send,
                                   "Send:",
                                   2,
                                   (int*)&tpreset->send,
                                   (const char *[]){"Notes","Sequence"},
                                   (int[]){SEND_NOTES,SEND_SEQUENCE},
                                   ui_edit_prevfrom_tr_sendconfig,
                                   ui_edit_nextfrom_tr_sendconfig,
                                   ui_edit_trig,
                                   NULL);
}

/** Editor for individual velocities */
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

/** Display menu for individual velocities */
void *ui_edit_tr_velocities (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i          ", CTX.trigger_nr+1);
    lcd_printf ("\002 %3i", tpreset->velocities[0]);
    if (tpreset->lastnote) {
        lcd_printf ("%3i ", tpreset->velocities[1]);
        if (tpreset->lastnote > 1) {
            lcd_printf ("%3i ", tpreset->velocities[2]);
            if (tpreset->lastnote > 2) {
                lcd_printf ("..");
            }
            else lcd_printf ("  ");
        }
        else lcd_printf ("      ");
    }
    else lcd_printf ("          ");
    
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
    last_edit_page = ui_edit_tr_velocity_mode;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i          ", CTX.trigger_nr+1);
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
                                   ui_edit_trig,
                                   NULL);
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
            
            case BTMASK_MINUS | BTMASK_SHIFT:
                if (tpreset->notes[ncursor]>11) {
                    tpreset->notes[ncursor] -= 11;
                }
                break;
            
            case BTMASK_PLUS:
            case BTMASK_STK_RIGHT:
                if (tpreset->notes[ncursor]<120) {
                    tpreset->notes[ncursor]++;
                }
                break;
                
            case BTMASK_PLUS | BTMASK_SHIFT:
                if (tpreset->notes[ncursor] < 115) {
                    tpreset->notes[ncursor] += 11;
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
    last_edit_page = ui_edit_tr_notes;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i          \n", CTX.trigger_nr+1);
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
        else lcd_printf ("      ");
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
    last_edit_page = ui_edit_tr_notecount;
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger %i          ", CTX.trigger_nr+1);
    int old_lastnote = tpreset->lastnote;
    void *res;
    res = ui_generic_choice_menu ((int)tpreset->lastnote,
                                   "Notes:",
                                   8,
                                   (int*)&tpreset->lastnote,
                                   (const char *[])
                                        {"1","2","3","4","5","6","7","8"},
                                   (int[]){0,1,2,3,4,5,6,7},
                                   NULL,
                                   ui_edit_tr_notes,
                                   ui_edit_trig,
                                   NULL);

    /* If the sequence length increased, copy note values from the
     * formerly last note of the sequence. */
    if (tpreset->lastnote > old_lastnote) {
        int x = old_lastnote+1;
        while (x <= tpreset->lastnote) {
            if (! tpreset->notes[x]) {
                tpreset->notes[x] = tpreset->notes[old_lastnote];
            }
            ++x;
        }
    } 
    
    return res;
}

/** Trigger selection menu. */
void *ui_edit_trig (void) {
    lcd_home();
    lcd_printf ("Trigger %i       \n<> Nav  -+ Edit ", CTX.trigger_nr+1);
        
    button_event *e = button_manager_wait_event (0);
    switch (e->buttons) {
        case BTMASK_LEFT:
        case BTMASK_STK_LEFT:
            if (CTX.trigger_nr>0) CTX.trigger_nr--;
            break;
        
        case BTMASK_RIGHT:
        case BTMASK_STK_RIGHT:
            if (CTX.trigger_nr<11) CTX.trigger_nr++;
            break;
        
        case BTMASK_PLUS:
        case BTMASK_MINUS:
            button_event_free (e);
            return last_edit_page;
            
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
        lcd_setpos (3,0);
        lcd_printf ("%-13s", CTX.preset.name);
        lcd_setpos (crsr+3,0);
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
                if (crsr && CTX.preset.name[crsr] == ' ' &&
                    CTX.preset.name[crsr-1] != ' ') {
                    CTX.preset.name[crsr] = CTX.preset.name[crsr-1];
                }
                else if (CTX.preset.name[crsr] != ' ') {
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
                if (crsr && CTX.preset.name[crsr] == ' ' &&
                    CTX.preset.name[crsr-1] != ' ') {
                    CTX.preset.name[crsr] = CTX.preset.name[crsr-1];
                }
                else if (CTX.preset.name[crsr] != '@') {
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

static uint8_t main_menu_pos = 0;

/** Edit main menu */
void *ui_edit_main (void) {
    uint8_t choice = main_menu_pos;
    const char *ch_name[3] = {"Edit Name","Edit Triggers","System Setup"};
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
                main_menu_pos = choice;
                break;
            
            case BTMASK_STK_LEFT:
            case BTMASK_LEFT:
                if (choice) choice = choice-1;
                else choice = 2;
                main_menu_pos = choice;
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

/** Status indicator for midi receive */
static bool light_midi_in = false;

/** Status indicator for midi send */
static bool light_midi_out = false;

/** Main performance menu */
void *ui_performance (void) {
    lcd_home();
    int tempo = CTX.ext_sync ? CTX.ext_tempo : CTX.preset.tempo;
    char s_tempo[8];
    sprintf (s_tempo, "%3i", tempo);
    if (! tempo) strcpy (s_tempo, "EXT");
    lcd_printf ("%02i|%-13s\n%c%c|\001 %s     %s%c%i",   
                CTX.preset_nr,
                CTX.preset.name,
                light_midi_in ? '\005' : ' ',
                light_midi_out ? '\006' : ' ',
                s_tempo,
                (CTX.transpose<-9||CTX.transpose>9)?"":" ",
                CTX.transpose<0?'-':'+',
                CTX.transpose<0?-CTX.transpose:CTX.transpose);
    
    button_event *e = button_manager_wait_event (1);
    switch (e->buttons) {
        case BTMASK_MDIN_ON: light_midi_in = true; break;
        case BTMASK_MDIN_OFF: light_midi_in = false; break;
        case BTMASK_MDOUT_ON: light_midi_out = true; break;
        case BTMASK_MDOUT_OFF: light_midi_out = false; break;
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
                midi_stop_sequencer();
                context_load_preset (CTX.preset_nr + 1);
            }
            break;
        
        case BTMASK_STK_LEFT:
        case BTMASK_LEFT:
            if (CTX.preset_nr > 1) {
                midi_stop_sequencer();
                context_load_preset (CTX.preset_nr - 1);
            }
            break;
            
        case BTMASK_PLUS:
            if (CTX.preset.tempo < 257) {
                if (! CTX.ext_sync) CTX.preset.tempo++;
            }
            break;
        
        case BTMASK_MINUS:
            if (CTX.preset.tempo > 41) {
                if (! CTX.ext_sync) CTX.preset.tempo--;
            }
            break;
        
        case BTMASK_STK_CLICK:
        case BTMASK_SHIFT | BTMASK_MINUS:
            button_event_free (e);
            return ui_edit_main;
            break;
            
        case BTMASK_SHIFT | BTMASK_PLUS:
            midi_panic();
            break;
    }
    
    button_event_free (e);
    return ui_performance;
}

/** Start the MIDI show, then jump to performance menu */
void *ui_startmidi (void) {
    midi_init();
    midi_check_ports();
    return ui_performance;
}

/** Polls ALSA for an available MIDI port prior to starting the
  * MIDI-related threads.
  */
void *ui_waitmidi (void) {
    if (midi_available()) return ui_startmidi;
    lcd_setpos (0,1);
    lcd_printf ("Plug in USB-MIDI");
    sleep (1);
    if (midi_available()) return ui_startmidi;
    sleep (1);
    if (midi_available()) return ui_startmidi;
    lcd_setpos (0,1);
    lcd_printf ("                ");
    sleep (1);
    return ui_waitmidi;
}

/** Show splash screen, then jump to performance menu */
void *ui_splash (void) {
    lcd_home();
    char tmagic[] = "  triggermagic  ";
    lcd_printf ("   \003\004 midilab    \n"
                "  triggermagic  ");
    
    for (int i=0; i<24; ++i) {
        musleep (5000000/64);
        int pos = rand() & 15;
        tmagic[pos] = toupper (tmagic[pos]);
        lcd_setpos (0,1);
        lcd_printf (tmagic);
        tmagic[pos] = tolower (tmagic[pos]);
    }
    lcd_setpos (0,1);
    lcd_printf (tmagic);
    sleep (1);
    return ui_waitmidi;
}
