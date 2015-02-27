#include "midi.h"

bool midi_available (void) {
    int devcount = Pm_CountDevices();
    
    /* Wimp out if only the ALSA ports are available */
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
    return true;
}
