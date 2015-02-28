#ifndef _MIDI_H
#define _MIDI_H 1

#include <portmidi.h>
#include <stdbool.h>

#include "thread.h"

bool midi_available (void);
void midi_panic (void);
void midi_stop_sequencer (void);
void midi_init (void);
void midi_check_ports (void);

#endif
