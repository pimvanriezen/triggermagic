#include "midi.h"
#include "presets.h"
#include "thread.h"
#include "btevent.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static bool initialized = false;

typedef struct triggerstate_s {
    bool             gate;
    uint64_t         ts; /**< time of noteon */
    char             seqpos;
    uint64_t         looppos;
    char             velocity;
    char             gateperc;
} triggerstate;

static struct midistate {
    thread          *receive_thread;
    thread          *send_thread;
    bool             open;
    pthread_mutex_t  in_lock;
    pthread_mutex_t  out_lock;
    PortMidiStream  *in;
    PortMidiStream  *out;
    char             in_devicename[256];
    char             out_devicename[256];
    int              current;
    triggerstate     trig[12];
    bool             noteon[128];
} self;

uint64_t getclock (void) {
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC_RAW, &ts);
    return ((ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL));
}

bool midi_available (void) {
    /* Don't use portmidi yet, or it will be bound to the non-working
       situation */
    
    if (system ("/usr/bin/amidi -l | grep -q ^IO")) return false;
    return true;
}

void midi_send_noteon (char note, char velocity) {
    char channel = CTX.send_channel;
    long msg = 0x90 | channel | ((long) note << 8) | (long) velocity << 16;
    Pm_WriteShort (self.out, 0, msg);
    button_manager_flash_midi_out();
}

void midi_send_noteoff (char note) {
    char channel = CTX.send_channel;
    long msg = 0x91 | channel | ((long) note << 8);
    Pm_WriteShort (self.out, 0, msg);
}

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
    
        switch (T->move) {
            case MOVE_SINGLE:
            case MOVE_LOOP_UP:
                self.trig[ti].seqpos++;
                break;
        
            case MOVE_LOOP_DOWN:
                self.trig[ti].seqpos--;
                break;
        
            case MOVE_LOOP_UPDOWN:
                if ((self.trig[ti].looppos/(T->lastnote+1))&1) {
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
        
        if (self.trig[ti].seqpos < 0) self.trig[ti].seqpos = T->lastnote;
        else if (self.trig[ti].seqpos > T->lastnote) self.trig[ti].seqpos = 0;
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
    
    midi_send_noteon (T->notes[i], velocity);
}

void midi_noteoff_response (int trig) {
    triggerpreset *T = &CTX.preset.triggers[trig];
    if (T->send == SEND_NOTES && T->nmode == NMODE_GATE) {
        for (int i=0; i<128; ++i) {
            if (self.noteon[i]) midi_send_noteoff (i);
        }
        self.trig[trig].gate = false;
    }
}

void midi_noteon_response (int trig, char velo) {
    int i;
    for (i=0; i<128; ++i) {
        if (self.noteon[i]) midi_send_noteoff (i);
    }
    
    self.current = trig;
    self.trig[trig].gate = true;
    self.trig[trig].ts = getclock();
    self.trig[trig].velocity = velo;
    self.trig[trig].seqpos = self.trig[trig].looppos = 0;
    
    triggerpreset *T = &CTX.preset.triggers[trig];
    int ntcount = T->lastnote+1;
    if (T->send == SEND_SEQUENCE) {
        midi_send_sequencer_step (trig);
    }
    else {
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
             
            midi_send_noteon (T->notes[i], velocity);
        }
    }
}

void midi_receive_thread (thread *t) {
    char trigmatch[12] = {0x24,0x26,0x2b,0x2f,0x32,0x25,0x27,0x2a,
                          0x2e,0x31,0x33,0x34};
                          
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
                        
                        for (int n=0; n<12; ++n) {
                            if (trigmatch[n] == note) {
                                if (noteon) midi_noteon_response (n, vel);
                                else midi_noteoff_response (n);
                            }
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

void midi_send_thread (thread *t) {
    while (1) {
        uint64_t qnote = (CTX.preset.tempo * 1000ULL) / 60;
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
                uint64_t notelen = qnote;
                uint64_t gatelen;
                char note = T->notes[self.trig[c].seqpos];
                
                switch (T->slen) {
                    case 2: notelen *= 2; break;
                    case 8: notelen /= 2; break;
                    case 16: notelen /=4; break;
                }
                
                gatelen = (notelen * self.trig[c].gateperc) / 100ULL;
                if (self.noteon[note]) {
                    if (dif - (notelen * self.trig[c].looppos) >= gatelen) {
                        midi_send_noteoff (note);
                    }
                }
                
                if (dif - (notelen * self.trig[c].looppos) >= notelen) {
                    midi_send_sequencer_step (c);
                }
            }
        }
        musleep (500);
    }
}

void midi_init (void) {
    if (! initialized) {
        for (int i=0; i<128; ++i) self.noteon[i] = false;
        pthread_mutex_init (&self.in_lock, NULL);
        pthread_mutex_init (&self.out_lock, NULL);
        self.in = NULL;
        self.out = NULL;
        self.current = -1;
        self.in_devicename[0] = self.out_devicename[0] = 0;
        self.receive_thread = thread_create (midi_receive_thread, NULL);
        self.send_thread = thread_create (midi_send_thread, NULL);
        initialized = true;
    }
}

void midi_set_input_device (int devid) {
    pthread_mutex_lock (&self.in_lock);
    if (self.in) {
        Pm_Close (self.in);
        self.in = NULL;
    }
    
    Pm_OpenInput (&self.in, devid, NULL, 16, NULL, NULL);
    Pm_SetFilter(self.in, PM_FILT_ACTIVE | PM_FILT_SYSEX | PM_FILT_CLOCK);
    pthread_mutex_unlock (&self.in_lock);
}

void midi_set_output_device (int devid) {
    pthread_mutex_lock (&self.out_lock);
    if (self.out) {
        Pm_Close (self.out);
        self.out = NULL;
    }
    
    Pm_OpenOutput (&self.out, devid, NULL, 16, NULL, NULL, 0);
    pthread_mutex_unlock (&self.out_lock);
}

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
