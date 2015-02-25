const char *CSET = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "0123456789./-!@";

void *ui_edit_name (void) {
    uint8_t crsr = 0;
    while (1) {
        lcd_home();
        lcd_printf ("%-13s   \n[+] ins  [-] del", CTX.preset.name);
        lcd_setpos (crsr,0);
        lcd_showcursor ();
        
        button_event *e = button_manager_wait_event (0);
        switch (e->buttons) {
            case BTMASK_LEFT:
                if (crsr) crsr--;
                break;
            
            case BTMASK_RIGHT:
                if (crsr<12) crsr++;
                break;
            
            case BTMASK_STK_LEFT:
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
                if (CTX.prset_name[crsr] != '@') {
                    const char *pos = strchr (CSET, CTX.preset_name[crsr]);
                    if (! pos) {
                        CTX.preset_name[crsr] = '@';
                        break;
                    }
                    CTX.preset_name[crsr] = *(pos+1);
                }
                break;
            
            case BTMASK_PLUS:
                if (crsr < 12) {
                    memmove (CTX.preset_name+crsr+1,
                             CTX.preset_name+crsr,
                             12-crsr);
                }
                break;
            
            case BTMASK_MINUS:
                memmove (CTX.preset_name+crsr,
                         CTX.preset_name+crsr+1,
                         12-crsr);
                CTX.preset_name[12] = ' ';
                break;
                
            case BTMASK_SHIFT:
                button_event_free (e);
                lcd_hidecursor();
                return ui_edit_main;
        }
        button_event_free (e);
    }
}

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
                button_event_free (e);
                return ch_jump[choice];
            
            case BTMASK_SHIFT:
                button_event_free (e);
                return ui_performance;
        }
        button_event_free (e);
    }
}

void *ui_performance (void) {
    lcd_home();
    lcd_printf ("%02i|%-13s\n  |\001 %3i     %c%i",   
                CTX.preset_nr,
                CTX.preset.name,
                CTX.preset.tempo,
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
