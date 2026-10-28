// d-BrainFog - tabla de parametros (sin dependencias de JUCE, compartida por motor, plugin y tests)
#pragma once

namespace bf {

enum ParamId {
    P_VOLUME, P_TONE, P_CUTOFF, P_GRIT, P_DRIFT, P_DARKSPACE, P_SPACEMIX,
    P_O1WAVE, P_O1LEVEL, P_PW, P_O2WAVE, P_O2LEVEL, P_O2SEMI, P_DETUNE, P_SUB, P_NOISE,
    P_RES, P_FENV, P_KEYTRACK,
    P_AA, P_AD, P_AS, P_AR, P_FA, P_FD, P_FS, P_FR,
    P_LFOSHAPE, P_LFOSYNC, P_LFODIV, P_LFORATE, P_LFOPITCH, P_LFOCUT, P_LFOPW,
    P_POLY, P_LEGATO, P_GLIDE, P_PBRANGE,
    P_DISTON, P_DISTMIX,
    P_CHON, P_CHRATE, P_CHDEPTH, P_CHMIX,
    P_DLON, P_DLTIME, P_DLFB, P_DLMIX,
    P_RVON, P_RVSIZE,
    P_COUNT
};

enum Kind { K_FLOAT, K_INT, K_BOOL, K_CHOICE };

struct ParamDef {
    const char* id;       // identificador estable (automatizacion / estado)
    const char* name;     // nombre visible
    Kind kind;
    float mn, mx, def;
    float mid;            // centro de curvatura (0 = lineal)
    const char* unit;
    const char* choices;  // opciones separadas por '|'
};

inline constexpr ParamDef kParams[P_COUNT] = {
 /* VOLUME   */ {"volume",        "Volume",       K_FLOAT, -48.f, 6.f,   -8.f,  0.f,   "dB", ""},
 /* TONE     */ {"tone",          "Tone",         K_FLOAT, 0.f,   1.f,   0.45f, 0.f,   "",   ""},
 /* CUTOFF   */ {"cutoff",        "Cutoff",       K_FLOAT, 20.f,  20000.f, 2100.f, 1000.f, "Hz", ""},
 /* GRIT     */ {"grit",          "Grit",         K_FLOAT, 0.f,   1.f,   0.12f, 0.f,   "",   ""},
 /* DRIFT    */ {"drift",         "Drift",        K_FLOAT, 0.f,   1.f,   0.5f,  0.f,   "",   ""},
 /* DARKSPACE*/ {"darkSpace",     "Dark Space",   K_FLOAT, 0.f,   1.f,   0.65f, 0.f,   "",   ""},
 /* SPACEMIX */ {"spaceMix",      "Space Mix",    K_FLOAT, 0.f,   1.f,   0.3f,  0.f,   "",   ""},
 /* O1WAVE   */ {"osc1Wave",      "Osc1 Wave",    K_CHOICE, 0.f,  2.f,   0.f,   0.f,   "",   "Saw|Pulse|Triangle"},
 /* O1LEVEL  */ {"osc1Level",     "Osc1 Lvl",     K_FLOAT, 0.f,   1.f,   0.75f, 0.f,   "",   ""},
 /* PW       */ {"pulseWidth",    "PW",           K_FLOAT, 0.05f, 0.95f, 0.42f, 0.f,   "",   ""},
 /* O2WAVE   */ {"osc2Wave",      "Osc2 Wave",    K_CHOICE, 0.f,  2.f,   1.f,   0.f,   "",   "Saw|Pulse|Triangle"},
 /* O2LEVEL  */ {"osc2Level",     "Osc2 Lvl",     K_FLOAT, 0.f,   1.f,   0.45f, 0.f,   "",   ""},
 /* O2SEMI   */ {"osc2Semi",      "Osc2 Semi",    K_INT,   -24.f, 24.f,  0.f,   0.f,   "st", ""},
 /* DETUNE   */ {"detune",        "Detune",       K_FLOAT, 0.f,   50.f,  22.f,  0.f,   "ct", ""},
 /* SUB      */ {"subLevel",      "Sub",          K_FLOAT, 0.f,   1.f,   0.15f, 0.f,   "",   ""},
 /* NOISE    */ {"noiseLevel",    "Noise",        K_FLOAT, 0.f,   1.f,   0.02f, 0.f,   "",   ""},
 /* RES      */ {"resonance",     "Reso",         K_FLOAT, 0.f,   1.f,   0.28f, 0.f,   "",   ""},
 /* FENV     */ {"filterEnvAmt",  "Env Amt",      K_FLOAT, -1.f,  1.f,   0.35f, 0.f,   "",   ""},
 /* KEYTRACK */ {"keyTrack",      "Keytrack",     K_FLOAT, 0.f,   1.f,   0.5f,  0.f,   "",   ""},
 /* AA       */ {"ampAttack",     "Amp Attack",   K_FLOAT, 0.002f, 5.f,  0.015f, 0.3f, "s",  ""},
 /* AD       */ {"ampDecay",      "Amp Decay",    K_FLOAT, 0.005f, 5.f,  0.6f,  0.5f,  "s",  ""},
 /* AS       */ {"ampSustain",    "Amp Sustain",  K_FLOAT, 0.f,   1.f,   0.75f, 0.f,   "",   ""},
 /* AR       */ {"ampRelease",    "Amp Release",  K_FLOAT, 0.005f, 8.f,  0.55f, 0.6f,  "s",  ""},
 /* FA       */ {"fltAttack",     "Flt Attack",   K_FLOAT, 0.002f, 5.f,  0.02f, 0.3f,  "s",  ""},
 /* FD       */ {"fltDecay",      "Flt Decay",    K_FLOAT, 0.005f, 5.f,  0.7f,  0.5f,  "s",  ""},
 /* FS       */ {"fltSustain",    "Flt Sustain",  K_FLOAT, 0.f,   1.f,   0.35f, 0.f,   "",   ""},
 /* FR       */ {"fltRelease",    "Flt Release",  K_FLOAT, 0.005f, 8.f,  0.5f,  0.6f,  "s",  ""},
 /* LFOSHAPE */ {"lfoShape",      "LFO Shape",    K_CHOICE, 0.f,  3.f,   0.f,   0.f,   "",   "Sine|Triangle|Square|Sample&Hold"},
 /* LFOSYNC  */ {"lfoSync",       "LFO Sync",     K_BOOL,  0.f,   1.f,   0.f,   0.f,   "",   ""},
 /* LFODIV   */ {"lfoDiv",        "LFO Div",      K_CHOICE, 0.f,  5.f,   2.f,   0.f,   "",   "1 bar|1/2|1/4|1/8|1/16|1/8T"},
 /* LFORATE  */ {"lfoRate",       "LFO Rate",     K_FLOAT, 0.05f, 20.f,  5.4f,  2.f,   "Hz", ""},
 /* LFOPITCH */ {"lfoPitch",      "LFO Pitch",    K_FLOAT, 0.f,   1.f,   0.09f, 0.f,   "",   ""},
 /* LFOCUT   */ {"lfoCutoff",     "LFO Cutoff",   K_FLOAT, 0.f,   1.f,   0.06f, 0.f,   "",   ""},
 /* LFOPW    */ {"lfoPulseWidth", "LFO PW",       K_FLOAT, 0.f,   1.f,   0.f,   0.f,   "",   ""},
 /* POLY     */ {"voices",        "Voices",       K_INT,   1.f,   8.f,   6.f,   0.f,   "",   ""},
 /* LEGATO   */ {"legato",        "Legato",       K_BOOL,  0.f,   1.f,   0.f,   0.f,   "",   ""},
 /* GLIDE    */ {"glide",         "Glide",        K_FLOAT, 0.f,   2000.f, 40.f, 200.f, "ms", ""},
 /* PBRANGE  */ {"bendRange",     "Bend Range",   K_INT,   0.f,   12.f,  2.f,   0.f,   "st", ""},
 /* DISTON   */ {"distOn",        "Dist On",      K_BOOL,  0.f,   1.f,   1.f,   0.f,   "",   ""},
 /* DISTMIX  */ {"distMix",       "Dist Mix",     K_FLOAT, 0.f,   1.f,   0.35f, 0.f,   "",   ""},
 /* CHON     */ {"chorusOn",      "Chorus On",    K_BOOL,  0.f,   1.f,   1.f,   0.f,   "",   ""},
 /* CHRATE   */ {"chorusRate",    "Chorus Rate",  K_FLOAT, 0.1f,  5.f,   0.55f, 0.8f,  "Hz", ""},
 /* CHDEPTH  */ {"chorusDepth",   "Chorus Depth", K_FLOAT, 0.f,   1.f,   0.45f, 0.f,   "",   ""},
 /* CHMIX    */ {"chorusMix",     "Chorus Mix",   K_FLOAT, 0.f,   1.f,   0.4f,  0.f,   "",   ""},
 /* DLON     */ {"delayOn",       "Delay On",     K_BOOL,  0.f,   1.f,   1.f,   0.f,   "",   ""},
 /* DLTIME   */ {"delayTime",     "Delay Time",   K_FLOAT, 20.f,  1500.f, 440.f, 300.f, "ms", ""},
 /* DLFB     */ {"delayFeedback", "Delay FB",     K_FLOAT, 0.f,   0.95f, 0.42f, 0.f,   "",   ""},
 /* DLMIX    */ {"delayMix",      "Delay Mix",    K_FLOAT, 0.f,   1.f,   0.22f, 0.f,   "",   ""},
 /* RVON     */ {"reverbOn",      "Reverb On",    K_BOOL,  0.f,   1.f,   1.f,   0.f,   "",   ""},
 /* RVSIZE   */ {"reverbSize",    "Reverb Size",  K_FLOAT, 0.f,   1.f,   0.62f, 0.f,   "",   ""},
};

} // namespace bf
