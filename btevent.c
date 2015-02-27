#include "btevent.h"
#include <unistd.h>
#include <pifacecad.h>
#include <stdlib.h>
#include <time.h>

button_manager BT;

/** Microsleeper. If someone can explain why gcc hates both usleep() and
  * nanosleep() under C99 and not sound like a douche, they win a prize.
  */
int musleep (uint64_t useconds) {
    struct timespec ts = {
        .tv_sec = (long int) (useconds / 1000000),
        .tv_nsec = (long int) (useconds % 1000000) * 1000ul
    };
    
    return nanosleep (&ts, NULL);
}

/** Initializes and spawns the button manager. Presumes the piface
  * has already been initialized through lcd_init().
  */
void button_manager_init (void) {
    for (int i=0; i<8; ++i) {
        BT.states[i].pressed = false;
        BT.states[i].lastchange = 0;
    }
    BT.tick = 0;
    BT.tick_midi_in = 0;
    BT.tick_midi_out = 0;
    conditional_init (&BT.eventcond);
    BT.first = BT.last = NULL;
    BT.useshift = true;
    pthread_mutex_init (&BT.lock, NULL);
    thread_init (&BT.super, button_manager_main, NULL);
}

/** Thread loop for the button manager. Reads the button state about every
  * 50ms. Sends it out if something changed. Also handles key repeats.
  */
void button_manager_main (thread *t) {
    uint64_t lastchange = 0;
    bool midi_in = false;
    bool midi_out = false;
    
    while (1) {
        uint8_t buttons = pifacecad_read_switches() ^ 0xff;
        bool changed = false;
        int pressedcount = 0;
        for (int i=0; i<8; ++i) {
            uint8_t mask = 1<<i;
            bool pressed = (buttons & mask) ? true : false;
            if (pressed != BT.states[i].pressed) {
                changed = true;
                BT.states[i].pressed = pressed;
                BT.states[i].lastchange = BT.tick;
                lastchange = BT.tick;
            }
            if (pressed) pressedcount++;
        }
        if (changed && pressedcount) {
            if ((!BT.useshift) || (buttons != BTMASK_SHIFT)) {
                /* If shift is to be treated as 'just a key', reject it
                 * if the only reason it is pressed is in the aftermath
                 * of using it in a combination. So only propagate
                 * it if it's state of being pressed happened during
                 * the last overall state change. */
                if ((buttons != BTMASK_SHIFT) ||
                    (BT.states[BTID_SHIFT].lastchange == BT.tick)) {
                    button_manager_add_event (buttons, false);
                }
            }
        }
        else if (pressedcount && buttons != BTMASK_SHIFT) {
            if (BT.tick - lastchange > 20) {
                if ((BT.tick & 1) == 0) {
                    button_manager_add_event (buttons, true);
                }
            }
        }
        
        if ((!midi_in) && (BT.tick - BT.tick_midi_in < 2)) {
            midi_in = true;
            button_manager_add_event (BTMASK_MDIN_ON, false);
        }
        else if (midi_in && (BT.tick - BT.tick_midi_in >4)) {
            midi_in = false;
            button_manager_add_event (BTMASK_MDIN_OFF, false);
        }
        if ((!midi_out) && (BT.tick - BT.tick_midi_out < 2)) {
            midi_out = true;
            button_manager_add_event (BTMASK_MDOUT_ON, false);
        }
        else if (midi_out && (BT.tick - BT.tick_midi_out >4)) {
            midi_out = false;
            button_manager_add_event (BTMASK_MDOUT_OFF, false);
        }
        
        musleep (50000);
        BT.tick++;
    }
}

void button_manager_flash_midi_in (void) {
    BT.tick_midi_in = BT.tick;
}

void button_manager_flash_midi_out (void) {
    BT.tick_midi_out = BT.tick;
}

/** Adds a button event to the queue and signals any consumers.
  * \param buttons Bitmask of the buttons pressed.
  * \param isrepeat True if this is a key repeat event.
  */
void button_manager_add_event (uint8_t buttons, bool isrepeat) {
    button_event *e = (button_event *) malloc (sizeof (button_event));
    e->next = NULL;
    e->buttons = buttons;
    e->isrepeat = isrepeat;
    pthread_mutex_lock (&BT.lock);
    if (BT.last) {
        BT.last->next = e;
        BT.last = e;
    }
    else {
        BT.first = BT.last = e;
    }
    pthread_mutex_unlock (&BT.lock);
    conditional_signal (&BT.eventcond);
}

/** Waits for a button_event to enter the queue, takes it
  * off and returns it.
  * \param useshift If true, shift key on its own spawns no events.
  */
button_event *button_manager_wait_event (bool useshift) {
    BT.useshift = useshift;
    button_event *e = NULL;
    while (1) {
        if (BT.first) {
            pthread_mutex_lock (&BT.lock);
            e = BT.first;
            if (e) {
                if (e->next) {
                    BT.first = e->next;
                    e->next = NULL;
                }
                else {
                    BT.first = BT.last = NULL;
                }
            }
            pthread_mutex_unlock (&BT.lock);
            if (e) return e;
        }
        conditional_wait (&BT.eventcond);
    }
}

/** Free event memory */
void button_event_free (button_event *e) {
    free (e);
}
