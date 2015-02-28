#ifndef _PRESETS_H
#define _PRESETS_H 1

/* =============================== TYPES =============================== */

/** Defines how we handle velocity data */
typedef enum {
    VELO_COPY = 0, /**< Copy from incoming Note On */
    VELO_INDIVIDUAL=1, /**< Set individually per configued note */
    VELO_RND_WIDE=2, /**< Random velocities 1-127 */
    VELO_RND_NARROW=3, /**< Random velocities 70-120 */
    VELO_FIXED_64=4, /**< All fixed velocities of 64 */
    VELO_FIXED_100=5 /**< All fixed velocities of 100 */
} velocityconfig;

/** Defines how to handle multiple configured notes */
typedef enum {
    SEND_NOTES, /**< Send all configured notes as a single chord */
    SEND_SEQUENCE /**< Send all configured notes as a sequence */
} sendconfig;

/** In case we're using SEND_NOTES, this defines how to handle the
    note length */
typedef enum {
    NMODE_GATE, /**< Wait for Note Off from input */
    NMODE_FIXED_2, /**< Fixed 1/2 note length */
    NMODE_FIXED_4, /**< Fixed 1/4 note length */
    NMODE_FIXED_8, /**< Fixed 1/8 note length */
    NMODE_FIXED_16, /**< Fixed 1/16 note length */
    NMODE_LEGATO /**< Send Note Off only when new event arrives */
} notemode;

/** Arpeggiated range of the sequence */
typedef enum {
    RANGE_ORIG = 0, /**< Use original range */
    RANGE_1_OCT, /**< Add a second loop up an octave */
    RANGE_2_OCT /**< Add a third loop up two octaves */
} sequencerange;

/** Defines how the sequence is played */
typedef enum {
    MOVE_SINGLE, /**< Single shot playback */
    MOVE_LOOP_UP, /**< Left-to-right, rinse, repeat */
    MOVE_LOOP_DOWN, /**< Right-to-left, rinse repeat */
    MOVE_LOOP_UPDOWN,   /**< Left-to-right-to-left */
    MOVE_LOOP_STEPUP,   /**< 123234345456567678781812 */
    MOVE_LOOP_STEPDOWN, /**< 876765654543432321218187 */
    MOVE_LOOP_RANDOM    /**< 4444444 (fair dice roll) */
} movetype;

typedef int seqlen;

/** Defines magical values for sequencer gate width, values from
    1 to 100 inclusive represent actual percentages */
typedef enum {
    SGATE_RND_WIDE=101, /**< random between 5 and 95% */
    SGATE_RND_NARROW=0 /**< random between 25 and 75% */
} gateconfig;

/** Storage for the values within a preset belonging to a single
    trigger/trigger */
typedef struct triggerpreset_s {
    char             notes[8]; /**< Note values, 0=end, 1=rst */
    int              lastnote; /**< Position of last active note */
    velocityconfig   vconf; /**< Velocity settings */
    char             velocities[8]; /**< Individual fixed velocities */
    sendconfig       send; /**< Send settings */
    notemode         nmode; /**< Note send length settings */
    seqlen           slen; /**< Sequence note length */
    gateconfig       sgate; /**< Sequencer gate settings */
    sequencerange    range; /**< Sequencer arpeggiator range */
    movetype         move; /**< Sequencer loop settings */
    char             pad[20]; /**< Room for future expansion */
} triggerpreset;

/** Storage for a single, complete, preset */
typedef struct preset_s {
    char             name[16]; /**< Preset name (max 13 chars) */
    triggerpreset    triggers[12]; /**< Per-trigger settings */
    int              tempo; /**< Sequencer tempo */
} preset;

/** Global performance context */
typedef struct context_global_s {
    int              preset_nr; /**< Number of loaded preset (1-99) */
    int              trigger_nr; /**< Edited trigger number (0-11) */
    preset           preset; /**< Working copy of loaded preset */
    int              transpose; /**< Current transpose */
    preset           presets[100]; /**< Stored presets 1-99 */
    char             portname_midi_in[256];
    char             portname_midi_out[256];
    char             send_channel;
} context_global;

/* ============================== GLOBALS ============================== */

extern context_global CTX;

/* ============================= FUNCTIONS ============================= */

void context_init (void);
void context_load_preset (int nr);
void context_store_preset (void);

#endif
