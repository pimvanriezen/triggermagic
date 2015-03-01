#include "midi.h"
#include "presets.h"
#include "thread.h"
#include "btevent.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/** Global initialization state */
static bool initialized = false;

/** State of an individual trigger */
typedef struct triggerstate_s {
    bool             gate; /**< True if key is down */
    uint64_t         ts; /**< time of noteon */
    char             seqpos; /**< Current position in the sequence */
    uint64_t         looppos; /**< Number of steps since first trigger */
    char             velocity; /**< Recorded trigger velocity */
    char             gateperc; /**< Determined gate length% if applicable */
} triggerstate;

/** State of the MIDI system */
static struct midistate {
    thread          *receive_thread; /**< MIDI receive loop */
    thread          *send_thread; /**< MIDI send loop for gates/sequences */
    bool             open; 
    pthread_mutex_t  in_lock; /**< Lock on input stream */
    pthread_mutex_t  out_lock; /**< Lock on output stream */
    pthread_mutex_t  seq_lock; /**< Lock on sequencer state */
    PortMidiStream  *in; /**< MIDI input stream */
    PortMidiStream  *out; /**< MIDI output stream */
    char             in_devicename[256]; /**< Current MIDI device name */
    char             out_devicename[256]; /**< Current MIDI device name */
    int              current; /**< Currently active trigger */
    triggerstate     trig[12]; /**< State for all triggers */
    bool             noteon[128]; /**< Note-on states of MIDI output */
} self;

/** Return the current time in milliseconds since epoch */
uint64_t getclock (void) {
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC_RAW, &ts);
    return ((ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL));
}

/** Poll ALSA for an available MIDI port */
bool midi_available (void) {
    /* Don't use portmidi yet, or it will be bound to the non-working
       situation */
    
    if (system ("/usr/bin/amidi -l | grep -q ^IO")) return false;
    return true;
}

/** Send a Note On message to the MIDI output */
void midi_send_noteon (char note, char velocity) {
    if (! note) return;
    char channel = CTX.send_channel;
    long msg = 0x90 | channel | ((long) note << 8) | (long) velocity << 16;
    pthread_mutex_lock (&self.out_lock);
    if (! self.noteon[note]) {
        self.noteon[note] = true;
        Pm_WriteShort (self.out, 0, msg);
    }
    pthread_mutex_unlock (&self.out_lock);
    
#ifdef DEBUG_MIDI
    printf ("NoteOn %i %i\n", note, velocity);
#endif

    button_manager_flash_midi_out();
}

/** Send a Note Off message to the MIDI output */
void midi_send_noteoff (char note) {
    if (! note) return;
    char channel = CTX.send_channel;
    long msg = 0x90 | channel | ((long) note << 8);
    pthread_mutex_lock (&self.out_lock);
    Pm_WriteShort (self.out, 0, msg);
    self.noteon[note] = false;
    pthread_mutex_unlock (&self.out_lock);
    
#ifdef DEBUG_MIDI
    printf ("NoteOff %i\n", note);
#endif
}

/** Send a MIDI panic out */
void midi_panic (void) {
    for (char i=1; i<128; ++i) {
        midi_send_noteoff (i);
    }
}

void midi_stop_sequencer (void) {
    pthread_mutex_lock (&self.seq_lock);
    if (self.current>=0) self.trig[self.current].ts = getclock() + 5000;
    self.current = -1;
    midi_panic();
    pthread_mutex_unlock (&self.seq_lock);
}

/** Perform a sequencer step, then advance it to the next note.
  * \param ti The selected trigger
  */
void midi_send_sequencer_step (int ti) {
    triggerpreset *T = &CTX.preset.triggers[ti];

    if (T->move == MOVE_SINGLE) {
        if (self.trig[ti].looppos > T->lastnote) {
            if (self.trig[ti].looppos == (T->lastnote+1)) {
                midi_send_noteoff (T->notes[T->lastnote]);
            }
            self.trig[ti].looppos++;
            return;
        }
    }

    if (self.trig[ti].looppos) {
        char oldnote = T->notes[self.trig[ti].seqpos];
        if (self.noteon[oldnote]) midi_send_noteoff (oldnote);
    
        switch (T->sgate) {
            case SGATE_RND_NARROW:
                self.trig[ti].gateperc = 25 + (rand() % 50);
                break;
            
            case SGATE_RND_WIDE:
                self.trig[ti].gateperc = 5 + (rand() % 90);
                break;
                
            default:
                self.trig[ti].gateperc = (int) T->sgate;
                break;
        }
        
        printf ("move %i ->", self.trig[ti].seqpos);
    
        switch (T->move) {
            case MOVE_SINGLE:
            case MOVE_LOOP_UP:
                self.trig[ti].seqpos++;
                break;
        
            case MOVE_LOOP_DOWN:
                self.trig[ti].seqpos--;
                break;
        
            case MOVE_LOOP_UPDOWN:
                if (((self.trig[ti].looppos-1)/(T->lastnote?T->lastnote:1))&1) {
                    self.trig[ti].seqpos--;
                }
                else self.trig[ti].seqpos++;
                break;
        
            case MOVE_LOOP_STEPUP:
                if (self.trig[ti].looppos % 3 == 2) {
                    self.trig[ti].seqpos--;
                }
                else self.trig[ti].seqpos++;
                break;
        
            case MOVE_LOOP_STEPDOWN:
                if (self.trig[ti].looppos % 3 == 2) {
                    self.trig[ti].seqpos++;
                }
                else self.trig[ti].seqpos--;
                break;
                
            case MOVE_LOOP_RANDOM:
                self.trig[ti].seqpos = rand() % (T->lastnote + 1);
                break;
        }
        
        printf ("%i ->", self.trig[ti].seqpos);
        
        if ((self.trig[ti].seqpos < 0) ||
            (self.trig[ti].seqpos == 255)) {
            self.trig[ti].seqpos = T->lastnote;
        }
        else if (self.trig[ti].seqpos > T->lastnote) {
            self.trig[ti].seqpos = 0;
        }
   
       printf ("%i\n", self.trig[ti].seqpos);
 }
    else {
        switch (T->move) {
            case MOVE_LOOP_DOWN:
                self.trig[ti].seqpos = T->lastnote;
                break;

            case MOVE_LOOP_RANDOM:
                self.trig[ti].seqpos = rand() % (T->lastnote + 1);
                break;
        }                
    }
    
    self.trig[ti].looppos++;
    int i = self.trig[ti].seqpos;
    int ntcount = T->lastnote+1;
    char velocity = 0;
    
    switch (T->vconf) {
        case VELO_COPY:
            velocity = self.trig[ti].velocity;
            break;
            
        case VELO_INDIVIDUAL:
            velocity = T->velocities[i];
            break;
        
        case VELO_RND_WIDE:
            velocity = (rand() % 126) + 1;
            break;
            
        case VELO_RND_NARROW:
            velocity = (rand() % 50) + 70;
            break;
        
        case VELO_FIXED_64:
            velocity = 64;
            break;
        
        case VELO_FIXED_100:
            velocity = 100;
            break;
    }

    if (self.current == ti) midi_send_noteon (T->notes[i], velocity);
}

/** Respond to a Note Off event on the MIDI input. Only triggers that
  * are configured as SEND_NOTES with the mode set to NMODE_GATE will
  * need to respond to these. In other situations, the gate is
  * controlled by the sequencer.
  */
void midi_noteoff_response (int trig) {
    triggerpreset *T = &CTX.preset.triggers[trig];
    if (T->send == SEND_NOTES && T->nmode == NMODE_GATE) {
        for (int i=0; i<=T->lastnote; ++i) {
            char note = T->notes[i];
            if (self.noteon[note]) midi_send_noteoff (note);
        }
        printf ("gate\n");
        self.trig[trig].gate = false;
    }
}

/** Respond to a Note On event on the MIDI input. Either plays the
  * direct note or chords, or sets up the trigger state for the
  * sequencer to pick up. */
void midi_noteon_response (int trig, char velo) {
    int i;
    for (i=0; i<128; ++i) {
        if (self.noteon[i]) midi_send_noteoff (i);
    }
    
    uint64_t last_ts = 0;
    if (self.current >= 0) {
        if (CTX.preset.triggers[self.current].send == SEND_SEQUENCE) {
            last_ts = self.trig[self.current].ts;
        }
    }

    triggerpreset *T = &CTX.preset.triggers[trig];
    
    pthread_mutex_lock (&self.seq_lock);

    self.current = trig;
    self.trig[trig].ts = getclock();
    self.trig[trig].gate = true;
    self.trig[trig].velocity = velo;
    self.trig[trig].seqpos = self.trig[trig].looppos = 0;
    
    /** Quantize a jump from one sequence into another */
    if (last_ts && T->send == SEND_SEQUENCE) {
        uint64_t qnote = 60000 / CTX.preset.tempo;
        uint64_t tsdif = self.trig[trig].ts - last_ts;
        tsdif = (tsdif + (qnote/2)) / qnote;
        tdifs *= qnote;
        self.trig[trig].ts = last_ts + tsdif;
    }
    
    pthread_mutex_unlock (&self.seq_lock);

    int ntcount = T->lastnote+1;
    if (T->send != SEND_SEQUENCE) {
        for (i=0; i<ntcount; ++i) {
            char velocity = 0;
            
            switch (T->vconf) {
                case VELO_COPY:
                    velocity = velo;
                    break;
                    
                case VELO_INDIVIDUAL:
                    velocity = T->velocities[i];
                    break;
                
                case VELO_RND_WIDE:
                    velocity = (rand() % 126) + 1;
                    break;
                    
                case VELO_RND_NARROW:
                    velocity = (rand() % 50) + 70;
                    break;
                
                case VELO_FIXED_64:
                    velocity = 64;
                    break;
                
                case VELO_FIXED_100:
                    velocity = 100;
                    break;
            }
            
            if (self.current == trig) {
                midi_send_noteon (T->notes[i], velocity);
                self.trig[trig].ts = getclock();
            }
        }
    }
}

char match_tr8[12]      = {0x24,0x26,0x2b,0x2f,0x32,0x25,0x27,0x2a,
                           0x2e,0x31,0x33,0x34};
char match_laser8[12]   = {0,2,4,5,6,7,9,11,1,3,8,10};
char match_laser9[12]   = {0,1,2,4,5,6,7,9,11,3,8,10};
char match_laser10[12]  = {0,1,2,4,5,6,7,8,9,11,3,10};
char match_pedals7[12]  = {0,2,4,5,7,9,11,1,3,6,8,10};

int midi_match_trigger (char note) {
    char in_note = note;
    if (CTX.trigger_type != TYPE_ROLAND_TR8) in_note = note % 12;
    for (int i=0; i<12; ++i) {
        char match_note;
        switch (CTX.trigger_type) {
            case TYPE_ROLAND_TR8:
                match_note = match_tr8[i];
                break;
            
            case TYPE_LASERHARP_8:
                match_note = match_laser8[i];
                break;
            
            case TYPE_LASERHARP_9:
                match_note = match_laser9[i];
                break;

            case TYPE_LASERHARP_10:
                match_note = match_laser10[i];
                break;

            case TYPE_CHROMATIC:
                match_note = i;
                break;

            case TYPE_PEDALS_7:
                match_note = match_pedals7[i];
                break;
        }
        if (match_note == in_note) return i;
    }
    return -1;
}

/** Thread that polls the incoming MIDI port, dispatching Note On and
  * Off messages further into the system */
void midi_receive_thread (thread *t) {
    /* TR-8
    char trigmatch[12] = {0x24,0x26,0x2b,0x2f,0x32,0x25,0x27,0x2a,
                          0x2e,0x31,0x33,0x34}; */
    
    
    char trigmatch[12] = {48,49,50,52,53,54,55,57,59,51,56,58};
                          
    PmEvent buffer[128];
    int count;
    while (1) {
        pthread_mutex_lock (&self.in_lock);
        if (self.in) {
            if (Pm_Poll (self.in) == TRUE) {
                count = Pm_Read (self.in, buffer, 128);
                if (count) {
                    for (int i=0; i<count; ++i) {
                        long msg = buffer[i].message;
                        bool noteon;
                        char note = ((msg & 0x7f00) >> 8);
                        char vel = ((msg & 0x7f0000) >> 16);
                        
                        if ((msg & 0xe0) == 0x80) {
                            noteon = false;
                            if ((msg & 0xf0) == 0x90 && vel) {
                                noteon = true;
                            }
                        }
                        
                        int n = midi_match_trigger (note);
                        if (n>=0) {
                            if (noteon) midi_noteon_response (n, vel);
                            else midi_noteoff_response (n);
                        }
                    }
                    button_manager_flash_midi_in();
                }
            }
            else musleep (50000);
            pthread_mutex_unlock (&self.in_lock);
        }
        else {
            pthread_mutex_unlock (&self.in_lock);
            sleep (1);
        }
    }
}

/** Thread that handles the programmed gate and sequencer. */
void midi_send_thread (thread *t) {
    while (1) {
        uint64_t qnote = 60000 / CTX.preset.tempo;
        uint64_t now = getclock();
        int c = self.current;
        if (c>=0) {
            triggerpreset *T = CTX.preset.triggers + c;
            uint64_t dif = now - self.trig[c].ts;
            if ((T->send == SEND_NOTES) && (T->nmode != NMODE_GATE) &&
                (T->nmode != NMODE_LEGATO)) {
                if (self.trig[c].gate) {
                    uint64_t notelen = qnote;
                    switch (T->nmode) {
                        case NMODE_FIXED_2:
                            notelen *= 2;
                            break;
                        
                        case NMODE_FIXED_8:
                            notelen /= 2;
                            break;
                        
                        case NMODE_FIXED_16:
                            notelen /= 4;
                            break;
                    }
                    if (dif >= notelen) {
                        for (int i=0; i<127; ++i) {
                            if (self.noteon[i]) midi_send_noteoff (i);
                        }
                        self.trig[c].gate = false;
                    }
                }
            }
            else if (T->send == SEND_SEQUENCE) {
                pthread_mutex_lock (&self.seq_lock);
                uint64_t notelen = qnote;
                uint64_t gatelen;
                char note = T->notes[self.trig[c].seqpos];
                
                switch (T->slen) {
                    case 2: notelen *= 2; break;
                    case 8: notelen /= 2; break;
                    case 16: notelen /=4; break;
                }
                
                uint64_t next_offs = notelen * (self.trig[c].looppos);

                gatelen = (notelen * (100-self.trig[c].gateperc)) / 100ULL;
                if (self.noteon[note]) {
                    if (dif >= (next_offs - gatelen)) {
                        for (int i=0; i<127; ++i) {
                            if (self.noteon[i]) midi_send_noteoff (i);
                        }
                    }
                }
                
                if (dif >= next_offs) {
                    midi_send_sequencer_step (c);
                }
                pthread_mutex_unlock (&self.seq_lock);
            }
        }
    }
}

/** Initialize internal information and start threads */
void midi_init (void) {
    if (! initialized) {
        for (int i=0; i<128; ++i) self.noteon[i] = false;
        pthread_mutex_init (&self.in_lock, NULL);
        pthread_mutex_init (&self.out_lock, NULL);
        pthread_mutex_init (&self.seq_lock, NULL);
        self.in = NULL;
        self.out = NULL;
        self.current = -1;
        self.in_devicename[0] = self.out_devicename[0] = 0;
        self.receive_thread = thread_create (midi_receive_thread, NULL);
        self.send_thread = thread_create (midi_send_thread, NULL);
        initialized = true;
    }
}

/** Set, or change, the PortMidi device to use for input */
void midi_set_input_device (int devid) {
    pthread_mutex_lock (&self.in_lock);
    if (self.in) {
        Pm_Close (self.in);
        self.in = NULL;
    }
    
    Pm_OpenInput (&self.in, devid, NULL, 128, NULL, NULL);
    Pm_SetFilter(self.in, PM_FILT_ACTIVE | PM_FILT_SYSEX | PM_FILT_CLOCK);
    pthread_mutex_unlock (&self.in_lock);
}

/** Set, or change, the PortMidi device to use for output */
void midi_set_output_device (int devid) {
    pthread_mutex_lock (&self.out_lock);
    if (self.out) {
        Pm_Close (self.out);
        self.out = NULL;
    }
    
    Pm_OpenOutput (&self.out, devid, NULL, 128, NULL, NULL, 0);
    pthread_mutex_unlock (&self.out_lock);
}

/** Check configuration for preferred MIDI ports. Hook up the first
  * physical In and Out ports if nothing seems configued.
  */
void midi_check_ports (void) {
    int devcount = Pm_CountDevices();
    if (! self.in_devicename[0]) {
        if (! CTX.portname_midi_in[0]) {
            /* Nothing configured, take the first in and out */
            for (int i=2; i<devcount; ++i) {
                const PmDeviceInfo *d = Pm_GetDeviceInfo (i);
                if (d->input) {
                    midi_set_input_device (i);
                }
                else if (d->output) {
                    midi_set_output_device (i);
                }
                if (self.in_devicename[0] && self.out_devicename[0]) break;
            }
        }
    }
    if (CTX.portname_midi_in[0]) {
        for (int i=2; i<devcount; ++i) {
            const PmDeviceInfo *d = Pm_GetDeviceInfo (i);
            if (strcmp (d->name, CTX.portname_midi_in) == 0) {
                midi_set_input_device (i);
            }
            else if (strcmp (d->name, CTX.portname_midi_out) == 0) {
                midi_set_output_device (i);
            }
        }
    }
}
