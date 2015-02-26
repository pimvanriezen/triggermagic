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
void    *ui_edit_nextfrom_tr_velocity_mode (void);
void    *ui_edit_tr_velocity_mode (void);
void    *ui_edit_notes (void);
void    *ui_edit_tr_notes (void);
void    *ui_edit_tr_notecount (void);
void    *ui_edit_trig (void);
void    *ui_edit_name (void);
void    *ui_edit_main (void);
void    *ui_performance (void);

#endif
