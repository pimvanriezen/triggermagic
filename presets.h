#ifndef _PRESETS_H
#define _PRESETS_H 1

/** Defines how we handle velocity data */
typedef enum {
    VELO_COPY = 0, /* Copy from incoming Note On */
    VELO_INDIVIDUAL=1, /* Set individually per configued note */
    VELO_RND_WIDE=2, /* Random velocities 1-127 */
    VELO_RND_NARROW=3 /* Random velocities 70-120 */
} velocityconfig;

/** Defines how to handle multiple configured notes */
typedef enum {
    SEND_NOTES, /* Send all configured notes as a single chord */
    SEND_SEQUENCE /* Send all configured notes as a sequence */
} sendconfig;

/** In case we're using SEND_NOTES, this defines how to handle the
    note length */
typedef enum {
    NMODE_GATE, /* Wait for Note Off from input */
    NMODE_FIXED_2, /* Fixed 1/2 note length */
    NMODE_FIXED_4, /* Fixed 1/4 note length */
    NMODE_FIXED_8, /* ... */
    NMODE_FIXED_16,
    NMODE_LEGATO /* Send Note Off only when new event arrives */
} notemode;

/** Arpeggiated range of the sequence */
typedef enum {
    RANGE_ORIG, /* Use original range */
    RANGE_1_OCT, /* Add a second loop up an octave */
    RANGE_2_OCT /* Add a third loop up two octaves */
} sequencerange;

/** Defines how the sequence is played */
typedef enum {
    SEQ_SINGLE, /* Single shot playback */
    SEQ_LOOP_UP, /* Left-to-right, rinse, repeat */
    SEQ_LOOP_DOWN,
    SEQ_LOOP_UPDOWN,   /* Left-to-right-to-left */
    SEQ_LOOP_STEPUP,   /* 123234345456567678781812 */
    SEQ_LOOP_STEPDOWN, /* 876765654543432321218187 */
    SEQ_LOOP_RANDOM    /* 4444444 (fair dice roll) */
} sequencetype;

/** Defines magical values for sequencer gate width, values from
    1 to 100 inclusive represent actual percentages */
typedef enum {
    SGATE_RND_WIDE=101, /* random between 5 and 95% */
    SGATE_RND_NARROW=0 /* random between 25 and 75% */
} gateconfig;

/** Storage for the values within a preset belonging to a single
    beam/trigger */
typedef struct beampreset_s {
    char notes[8];
    velocityconfig vconf;
    char velocities[8];
    sendconfig send;
    notemode nmode;
    gateconfig sgate;
    sequencerange range;
    sequencetype seq;
    char pad[24];
} beampreset;

/** Storage for a single, complete, preset */
typedef struct preset_s {
    char name[16];
    beampreset beams[12];
    int tempo;
} preset;

/** Global performance context */
typedef struct context_global_s {
    int cur_preset_nr;
    preset cur_preset;
    int cur_transpose;
    preset presets[100];
} context_global;

extern preset PRESETS[100];

#endif
