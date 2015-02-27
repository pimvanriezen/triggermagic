#include "midi.h"

bool midi_available (void) {
    /* Don't use portmidi yet, or it will be bound to the non-working
       situation */
    
    if (system ("/home/pi/checkmidi.sh")) return false;
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
