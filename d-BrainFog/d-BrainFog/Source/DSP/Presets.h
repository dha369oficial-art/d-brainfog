// d-BrainFog - presets de fabrica (originales). Valores en unidades reales de cada parametro.
#pragma once
#include "ParamDefs.h"

namespace bf {

constexpr int kNumFactoryPresets = 4;
inline constexpr const char* kFactoryPresetNames[kNumFactoryPresets] = {
    "After Dark", "Cold Hall", "Broken Tape", "Red Static"
};

inline void factoryPresetValues(int idx, float* v)
{
    for (int i = 0; i < P_COUNT; ++i) v[i] = kParams[i].def;   // base = After Dark
    switch (idx) {
    default:
    case 0: // After Dark: lead frio, melancolico, desafinado
        break;

    case 1: // Cold Hall: pad ambiental profundo y amplio
        v[P_VOLUME]=-9; v[P_TONE]=0.38f; v[P_CUTOFF]=900; v[P_GRIT]=0.02f; v[P_DRIFT]=0.55f;
        v[P_DARKSPACE]=0.7f; v[P_SPACEMIX]=0.6f;
        v[P_O1WAVE]=0; v[P_O1LEVEL]=0.6f; v[P_O2WAVE]=0; v[P_O2LEVEL]=0.6f; v[P_O2SEMI]=12; v[P_DETUNE]=14;
        v[P_SUB]=0.35f; v[P_NOISE]=0.01f; v[P_RES]=0.22f; v[P_FENV]=0.25f; v[P_KEYTRACK]=0.4f;
        v[P_AA]=1.4f; v[P_AD]=1.5f; v[P_AS]=0.85f; v[P_AR]=3.2f;
        v[P_FA]=1.8f; v[P_FD]=2.5f; v[P_FS]=0.5f; v[P_FR]=3.0f;
        v[P_LFOSHAPE]=1; v[P_LFORATE]=0.18f; v[P_LFOPITCH]=0.04f; v[P_LFOCUT]=0.35f;
        v[P_POLY]=8; v[P_GLIDE]=0;
        v[P_DISTON]=0; v[P_DISTMIX]=0;
        v[P_CHRATE]=0.3f; v[P_CHDEPTH]=0.75f; v[P_CHMIX]=0.55f;
        v[P_DLTIME]=620; v[P_DLFB]=0.5f; v[P_DLMIX]=0.18f; v[P_RVSIZE]=0.92f;
        break;

    case 2: // Broken Tape: deteriorado, saturado, modulacion irregular
        v[P_VOLUME]=-9; v[P_TONE]=0.3f; v[P_CUTOFF]=1300; v[P_GRIT]=0.65f; v[P_DRIFT]=0.95f;
        v[P_DARKSPACE]=0.85f; v[P_SPACEMIX]=0.22f;
        v[P_O1WAVE]=0; v[P_O1LEVEL]=0.7f; v[P_O2WAVE]=2; v[P_O2LEVEL]=0.5f; v[P_O2SEMI]=-12; v[P_DETUNE]=35;
        v[P_SUB]=0.4f; v[P_NOISE]=0.08f; v[P_RES]=0.3f; v[P_FENV]=0.2f; v[P_KEYTRACK]=0.3f;
        v[P_AA]=0.005f; v[P_AD]=0.8f; v[P_AS]=0.6f; v[P_AR]=0.35f;
        v[P_LFOSHAPE]=1; v[P_LFORATE]=0.55f; v[P_LFOPITCH]=0.6f; v[P_LFOCUT]=0.15f;
        v[P_POLY]=4; v[P_GLIDE]=120;
        v[P_DISTMIX]=0.85f;
        v[P_CHRATE]=1.7f; v[P_CHDEPTH]=0.9f; v[P_CHMIX]=0.5f;
        v[P_DLTIME]=280; v[P_DLFB]=0.6f; v[P_DLMIX]=0.3f; v[P_RVSIZE]=0.4f;
        break;

    case 3: // Red Static: aspero, ruidoso, experimental
        v[P_VOLUME]=-10; v[P_TONE]=0.65f; v[P_CUTOFF]=3200; v[P_GRIT]=0.9f; v[P_DRIFT]=0.6f;
        v[P_DARKSPACE]=0.9f; v[P_SPACEMIX]=0.35f;
        v[P_O1WAVE]=1; v[P_O1LEVEL]=0.6f; v[P_PW]=0.25f; v[P_O2WAVE]=0; v[P_O2LEVEL]=0.55f; v[P_O2SEMI]=6; v[P_DETUNE]=50;
        v[P_SUB]=0.25f; v[P_NOISE]=0.55f; v[P_RES]=0.62f; v[P_FENV]=0.6f; v[P_KEYTRACK]=0.2f;
        v[P_AA]=0.002f; v[P_AD]=0.25f; v[P_AS]=0.45f; v[P_AR]=0.2f;
        v[P_FA]=0.002f; v[P_FD]=0.18f; v[P_FS]=0.1f; v[P_FR]=0.2f;
        v[P_LFOSHAPE]=3; v[P_LFORATE]=9; v[P_LFOPITCH]=0.05f; v[P_LFOCUT]=0.55f; v[P_LFOPW]=0.6f;
        v[P_POLY]=5; v[P_GLIDE]=0;
        v[P_DISTMIX]=1.0f;
        v[P_CHON]=0;
        v[P_DLTIME]=160; v[P_DLFB]=0.72f; v[P_DLMIX]=0.28f; v[P_RVSIZE]=0.3f;
        break;
    }
}

} // namespace bf
