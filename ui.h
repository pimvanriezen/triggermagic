#ifndef _UI_H
#define _UI_H 1

#include "lcd.h"

/* =============================== TYPES =============================== */

typedef void *(*uifunc)(void);

/* ============================= FUNCTIONS ============================= */

void     ui_main (void);
int      ui_select (int cv, int x, int y, int l, int s, 
                 const char *n[], int v[]);
void    *ui_generic_choice_menu (int cv, const char *pr, int cnt,
                                 int *writeto, const char *n[], int v[],
                                 void *lr, void *rr, void *ur);
void     ui_write_note (char);
void    *ui_edit_tr_seq_move (void);
void    *ui_edit_tr_seq_range (void);
void    *ui_edit_tr_seq_gate (void);
void    *ui_edit_tr_seq_length (void);
void    *ui_edit_tr_notes_mode (void);
void    *ui_edit_prevfrom_tr_sendconfig (void);
void    *ui_edit_nextfrom_tr_sendconfig (void);
void    *ui_edit_tr_sendconfig (void);
void    *ui_edit_velocities (void);
void    *ui_edit_tr_velocities (void);
void    *ui_edit_nextfrom_tr_velocity_mode (void);
void    *ui_edit_tr_velocity_mode (void);
void    *ui_edit_notes (void);
void    *ui_edit_tr_notes (void);
void    *ui_edit_tr_notecount (void);
void    *ui_edit_trig (void);
void    *ui_edit_name (void);
void    *ui_edit_main (void);
void    *ui_performance (void);
void    *ui_waitmidi (void);
void    *ui_splash (void);

#endif
