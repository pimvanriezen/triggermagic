#include "midi.h"
#include "presets.h"
#include "thread.h"
#include "btevent.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool initialized = false;

static struct midistate {
    thread          *read_thread;
    bool             open;
    pthread_mutex_t  in_lock;
    pthread_mutex_t  out_lock;
    PortMidiStream  *in;
    PortMidiStream  *out;
    char             in_devicename[256];
    char             out_devicename[256];
} self;

bool midi_available (void) {
    /* Don't use portmidi yet, or it will be bound to the non-working
       situation */
    
    if (system ("/usr/bin/amidi -l | grep -q ^IO")) return false;
    return true;
    /*
    int devcount = Pm_CountDevices();
    
    if (devcount <= 2) return false;
    
    int numinputs = 0;
    int numoutputs = 0;
    
    for (int i=2; i<devcount; ++i) {
        const PmDeviceInfo *d = Pm_GetDeviceInfo (i);
        numinputs += d->input;
        numoutputs += d->output;
    }
    
    if (! numinputs) return false;
    if (! numoutputs) return false;
    return true; */
}

void midi_read_thread (thread *t) {
    PmEvent buffer[128];
    int count;
    while (1) {
        pthread_mutex_lock (&self.in_lock);
        if (self.in) {
            if (Pm_Poll (self.in) == TRUE) {
                count = Pm_Read (self.in, buffer, 128);
                if (count) {
                    printf ("%i ", count);
                    for (int i=0; i<count; ++i) {
                        printf ("%08x ", buffer[i].message);
                    }
                    printf ("\n");
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

void midi_init (void) {
    if (! initialized) {
        pthread_mutex_init (&self.in_lock, NULL);
        pthread_mutex_init (&self.out_lock, NULL);
        self.in = NULL;
        self.out = NULL;
        self.in_devicename[0] = self.out_devicename[0] = 0;
        self.read_thread = thread_create (midi_read_thread, NULL);
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
    Pm_SetFilter(&self.in, PM_FILT_ACTIVE | PM_FILT_SYSEX);
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
