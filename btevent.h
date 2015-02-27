#ifndef _BTEVENT_H
#define _BTEVENT_H 1

#include <stdbool.h>
#include <stdint.h>
#include "thread.h"

/* =============================== TYPES =============================== */

#define BTID_LEFT       0
#define BTID_RIGHT      1
#define BTID_MINUS      2
#define BTID_PLUS       3
#define BTID_SHIFT      4
#define BTID_STK_LEFT   5
#define BTID_STK_CLICK  6
#define BTID_STK_RIGHT  7

#define BTMASK_LEFT      (1 << BTID_LEFT)
#define BTMASK_RIGHT     (1 << BTID_RIGHT)
#define BTMASK_MINUS     (1 << BTID_MINUS)
#define BTMASK_PLUS      (1 << BTID_PLUS)
#define BTMASK_SHIFT     (1 << BTID_SHIFT)
#define BTMASK_STK_LEFT  (1 << BTID_STK_LEFT)
#define BTMASK_STK_RIGHT (1 << BTID_STK_RIGHT)
#define BTMASK_STK_CLICK (1 << BTID_STK_CLICK)

#define BTMASK_MDIN_ON   0xf0
#define BTMASJ_MDIN_OFF  0xf1
#define BTMASK_MDOUT_ON  0xf2
#define BTMASK_MDOUT_OFF 0xf3

/** Event flowing out of the button manager. Represents a specific
    configuration of buttons pressed. */
typedef struct button_event_s {
    struct button_event_s   *next; /**< List-link for the queue */
    uint8_t                  buttons; /**< Button state */
    bool                     isrepeat; /**< true if it's a repetition */
} button_event;

/** Represents the state of a single button */
typedef struct button_state_s {
    bool        pressed; /**< true if button is pressed */
    uint64_t    lastchange; /**< tick number of last state change */
} button_state;

/** Class object for the button manager */
typedef struct button_manager_s {
    thread           super; /**< The thread this all runs on */
    button_state     states[8]; /**< State of buttons during this tick */
    uint64_t         tick; /**< Current tick number */
    uint64_t         tick_midi_in; /**< Tick of last MIDI event */
    uint64_t         tick_midi_out; /**< Tick of last MIDI out event */
    conditional      eventcond; /**< Conditional for new events */
    button_event    *first; /**< Linked-list head */
    button_event    *last; /**< Linked-list tail */
    bool             useshift; /**< True if we should treat shift as shift */
    pthread_mutex_t  lock; /**< Mutex for the event queue */
} button_manager;

/* ============================== GLOBALS ============================== */

extern button_manager BT;

/* ============================= FUNCTIONS ============================= */

void             button_manager_init (void);
void             button_manager_main (thread *);
void             button_manager_flash_midi_in (void);
void             button_manager_flash_midi_out (void);
void             button_manager_add_event (uint8_t, bool);
button_event    *button_manager_wait_event (bool useshift);

void             button_event_free (button_event *);

#endif
