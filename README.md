# triggermagic
Triggermagic is a MIDI application designed to run on a Raspberry Pi 2
board, using the PiFace board as an interface.

The application takes MIDI input from a limited MIDI controller,
like bass pedals, or a laser harp, and turns them into either a musical
scale, a chord, or a sequence.

For scales and chords, the gate time of each trigger can be either
copied from the controller, or set to a specific tempo-defined time,
or turned into a legato mode, where only a new trigger will mute the
selected one.

For all forms of playback, velocities can either be copied over from the
controller input, or assigned a static, or bounded random value.

The sequencer allows various loop modes over up to 8 step positions.
Gate times can be pre-set or controlled by bounded random.

This application uses the libpifacecad library for interacting with
the LCD module, and buttons. It also needs the PortMidi library for
interacting with MIDI interfaces.
