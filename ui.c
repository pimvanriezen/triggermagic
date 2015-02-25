/** Usable character set for preset names */
const char *CSET = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "0123456789./-!@";

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
    lcd_showcursor();
    
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
                         

/** Note names */
const char *TB_NOTES[12] = {"C-","C#","D-","D#","E-","F-",
                            "F#","G-","G#","A","A#","B-"}

/** Write a note value to the LCD.
  * \param notenr The MIDI note number
  */
ui_write_note (char notenr) {
    if (notenr == 0) lcd_printf ("---");
    else {
        lcd_printf (TB_NOTES[notenr%12]);
        lcd_printf ("%i ", notenr/12)
    }
}



void *ui_edit_nextfrom_tr_velocity_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    if (tpreset->vconf == VELO_INDIVIDUAL) return ui_edit_tr_velocities;
    return ui_edit_tr_sendconfig;
}

void *ui_edit_tr_velocity_mode (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    return ui_generic_choice_menu (tpreset->vconf,
                                   "Velocity:",
                                   6,
                                   &tpreset->vconf,
                                   {"Copy","Indiv","RndWid",
                                    "RndNrw","Fix64",
                                    "Fix100"},
                                   {VELO_COPY, VELO_INDIVIDUAL,
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
        for (int i=0; i<=tpreset->lastnote; ++i) {
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
            case BTMASK_STK_LEFT;
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
        }
    }
    
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
    return ui_edit_tre_notes;
}

/** Displays the number of notes in the chord/sequence for editing */
void *ui_edit_tr_notecount (void) {
    triggerpreset *tpreset = CTX.preset.triggers + CTX.trigger_nr;
    lcd_home();
    lcd_printf ("Trigger: %02i      \n", CTX.trigger_nr+1);
    return ui_generic_choice_menu (tpreset->lastnote,
                                   "# Notes:",
                                   8,
                                   &tpreset->lastnote,
                                   {"1","2","3","4","5","6","7","8"},
                                   {0,1,2,3,4,5,6,7},
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
                if (CTX.preset_name[crsr] != ' ') {
                    const char *pos = strchr (CSET, CTX.preset_name[crsr]);
                    if (! pos) {
                        CTX.preset_name[crsr] = ' ';
                        break;
                    }
                    CTX.preset_name[crsr] = *(pos-1);
                }
                break;
            
            case BTMASK_STK_RIGHT:
            case BTMASK_PLUS:
                if (CTX.prset_name[crsr] != '@') {
                    const char *pos = strchr (CSET, CTX.preset_name[crsr]);
                    if (! pos) {
                        CTX.preset_name[crsr] = '@';
                        break;
                    }
                    CTX.preset_name[crsr] = *(pos+1);
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
    ui_handler ch_jump[3] = {ui_edit_name, ui_edit_trig, ui_edit_global};
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
                (CTX.transpose<-9||CTX.transpose>9)?"":" ";
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
            if (CTX.prset.tempo > 41) {
                CTX.preset.tempo--;
            }
            break;
        
        case BTMASK_STK_CLICK:
            button_event_free (e);
            return ui_edit_main;
            break;
    }
    
    button_event_free (e);
    return ui_performance;
}
