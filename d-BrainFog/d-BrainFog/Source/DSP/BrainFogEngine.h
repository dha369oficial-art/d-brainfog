// d-BrainFog DSP engine - C++17 puro (sin JUCE) para poder probarlo fuera del plugin.
// Toda la memoria se reserva en prepare(); process()/noteOn()/etc. no asignan memoria.
#pragma once
#include "ParamDefs.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace bf {

constexpr int   kMaxVoices = 8;
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float flushDenorm(float x) noexcept { return std::fabs(x) < 1.0e-20f ? 0.0f : x; }

inline float softClip(float x) noexcept
{
    const float t = 0.85f;
    const float a = std::fabs(x);
    if (a <= t) return x;
    const float s = t + (1.0f - t) * std::tanh((a - t) / (1.0f - t));
    return x < 0.0f ? -s : s;
}

inline float mtof(int n) noexcept { return 440.0f * std::exp2((float) (n - 69) / 12.0f); }

struct Rng {
    uint32_t s = 0x9E3779B9u;
    void seed(uint32_t v) noexcept { s = v ? v : 1u; }
    float next() noexcept   // -1 .. 1
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s & 0xFFFFFFu) / 8388608.0f - 1.0f;
    }
};

struct Smooth {
    float v = 0, t = 0, c = 0.01f;
    void setTime(float sec, float sr) noexcept { c = 1.0f - std::exp(-1.0f / (std::max(sec, 1.0e-4f) * sr)); }
    void snap(float x) noexcept { v = t = x; }
    float tick() noexcept { v += (t - v) * c; return v; }
};

// ---------------------------------------------------------------- osciladores
inline float polyBlep(float t, float dt) noexcept
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float oscSample(int wave, float ph, float dt, float pw) noexcept
{
    switch (wave) {
    case 0:  return 2.0f * ph - 1.0f - polyBlep(ph, dt);                       // saw
    case 1: {                                                                  // pulse
        float v = ph < pw ? 1.0f : -1.0f;
        v += polyBlep(ph, dt);
        float p2 = ph - pw; if (p2 < 0.0f) p2 += 1.0f;
        v -= polyBlep(p2, dt);
        return v; }
    default: return 2.0f * std::fabs(2.0f * ph - 1.0f) - 1.0f;                 // triangulo
    }
}

// ---------------------------------------------------------------- filtro
struct Svf {   // SVF TPT pasa bajos 12 dB/oct
    float ic1 = 0, ic2 = 0;
    void reset() noexcept { ic1 = ic2 = 0; }
    float process(float x, float g, float k) noexcept
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1, a3 = g * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flushDenorm(2.0f * v1 - ic1);
        ic2 = flushDenorm(2.0f * v2 - ic2);
        return v2;
    }
};

// ---------------------------------------------------------------- envolvente
struct EnvP { float aInc = 0.01f, dC = 0.001f, sus = 0.7f, rC = 0.001f; };

struct Adsr {
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float level = 0.0f;
    void gateOn()  noexcept { stage = Attack; }
    void gateOff() noexcept { if (stage != Idle) stage = Release; }
    float tick(const EnvP& e) noexcept
    {
        switch (stage) {
        case Attack:  level += e.aInc; if (level >= 1.0f) { level = 1.0f; stage = Decay; } break;
        case Decay:   level += (e.sus - level) * e.dC; if (std::fabs(level - e.sus) < 1.0e-4f) stage = Sustain; break;
        case Sustain: level += (e.sus - level) * e.dC; break;
        case Release: level -= level * e.rC; if (level < 1.0e-4f) { level = 0.0f; stage = Idle; } break;
        default: break;
        }
        return level;
    }
};

// ---------------------------------------------------------------- efectos
struct DelayLine {
    std::vector<float> buf;
    int w = 0;
    void init(int size) { buf.assign((size_t) size + 4, 0.0f); w = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    void write(float x) noexcept { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    float read(float d) const noexcept
    {
        const int n = (int) buf.size();
        float pos = (float) w - d;
        while (pos < 0.0f) pos += (float) n;
        int i0 = (int) pos; const float fr = pos - (float) i0;
        if (i0 >= n) i0 -= n;
        int i1 = i0 + 1; if (i1 >= n) i1 = 0;
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }
};

struct Chorus {
    DelayLine dl[2];
    float ph = 0.0f, sr = 44100.0f;
    void prepare(float s) { sr = s; for (auto& d : dl) d.init((int) (0.06f * sr) + 8); ph = 0.0f; }
    void clear() { for (auto& d : dl) d.clear(); ph = 0.0f; }
    void process(float& l, float& r, float rate, float depth, float mix) noexcept
    {
        ph += rate / sr; if (ph >= 1.0f) ph -= 1.0f;
        dl[0].write(l); dl[1].write(r);
        const float base = 0.013f * sr, dev = 0.007f * sr * depth;
        const float wl = dl[0].read(base + dev * std::sin(kTwoPi * ph));
        const float wr = dl[1].read(base + dev * std::sin(kTwoPi * (ph + 0.25f)));
        l = l * (1.0f - 0.5f * mix) + wl * 0.7f * mix;
        r = r * (1.0f - 0.5f * mix) + wr * 0.7f * mix;
    }
};

struct Delay {
    DelayLine dl[2];
    float lpL = 0, lpR = 0, a = 0.3f, maxD = 1000.0f;
    void prepare(float sr) { dl[0].init((int) (1.6f * sr)); dl[1].init((int) (1.6f * sr)); a = 1.0f - std::exp(-kTwoPi * 3500.0f / sr); maxD = 1.55f * sr; lpL = lpR = 0; }
    void clear() { dl[0].clear(); dl[1].clear(); lpL = lpR = 0; }
    void process(float& l, float& r, float timeSamp, float fb, float mix) noexcept
    {
        const float tl = std::max(1.0f, std::min(timeSamp, maxD));
        const float tr = std::max(1.0f, std::min(timeSamp * 0.75f, maxD));
        const float dL = dl[0].read(tl), dR = dl[1].read(tr);
        lpL = flushDenorm(lpL + (dL - lpL) * a);
        lpR = flushDenorm(lpR + (dR - lpR) * a);
        const float fL = std::tanh(lpL * fb), fR = std::tanh(lpR * fb);
        dl[0].write(flushDenorm(l + fR));
        dl[1].write(flushDenorm(r + fL));
        l += dL * mix; r += dR * mix;
    }
};

struct Comb {
    std::vector<float> buf; int i = 0; float store = 0.0f;
    float process(float in, float fb, float damp) noexcept
    {
        const float o = buf[(size_t) i];
        store = flushDenorm(o * (1.0f - damp) + store * damp);
        buf[(size_t) i] = in + store * fb;
        if (++i >= (int) buf.size()) i = 0;
        return o;
    }
};
struct Allpass {
    std::vector<float> buf; int i = 0;
    float process(float in) noexcept
    {
        const float b = buf[(size_t) i];
        const float o = -in + b;
        buf[(size_t) i] = flushDenorm(in + b * 0.5f);
        if (++i >= (int) buf.size()) i = 0;
        return o;
    }
};

struct Reverb {   // estilo Freeverb, oscuro (damping alto)
    Comb c[2][8];
    Allpass ap[2][4];
    void prepare(float sr)
    {
        static const int ct[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int at[4] = { 556, 441, 341, 225 };
        const float sc = sr / 44100.0f;
        for (int ch = 0; ch < 2; ++ch) {
            for (int k = 0; k < 8; ++k) { c[ch][k].buf.assign((size_t) ((ct[k] + ch * 23) * sc) + 2, 0.0f); c[ch][k].i = 0; c[ch][k].store = 0; }
            for (int k = 0; k < 4; ++k) { ap[ch][k].buf.assign((size_t) ((at[k] + ch * 23) * sc) + 2, 0.0f); ap[ch][k].i = 0; }
        }
    }
    void clear()
    {
        for (int ch = 0; ch < 2; ++ch) {
            for (auto& x : c[ch])  { std::fill(x.buf.begin(), x.buf.end(), 0.0f); x.i = 0; x.store = 0; }
            for (auto& x : ap[ch]) { std::fill(x.buf.begin(), x.buf.end(), 0.0f); x.i = 0; }
        }
    }
    void process(float inL, float inR, float size, float dark, float& wL, float& wR) noexcept
    {
        const float in = (inL + inR) * 0.015f;
        const float fb = 0.70f + 0.28f * size;
        const float damp = 0.10f + 0.85f * dark;
        float sl = 0, sr_ = 0;
        for (int k = 0; k < 8; ++k) { sl += c[0][k].process(in, fb, damp); sr_ += c[1][k].process(in, fb, damp); }
        for (int k = 0; k < 4; ++k) { sl = ap[0][k].process(sl); sr_ = ap[1][k].process(sr_); }
        wL = sl * 3.0f; wR = sr_ * 3.0f;
    }
};

// ---------------------------------------------------------------- motor
class Engine {
public:
    void prepare(double sampleRate)
    {
        sr = (float) std::max(8000.0, sampleRate);
        chorus.prepare(sr); delay.prepare(sr); reverb.prepare(sr);
        for (auto& s : sm) s.setTime(0.01f, sr);
        sm[S_CUT].setTime(0.02f, sr);
        sm[S_DLTIME].setTime(0.08f, sr);
        bendC   = 1.0f - std::exp(-1.0f / (0.005f * sr));
        driftC  = 1.0f - std::exp(-1.0f / (0.4f * sr));
        lfoC    = 1.0f - std::exp(-1.0f / (0.002f * sr));
        toneA   = 1.0f - std::exp(-kTwoPi * 800.0f / sr);
        distA   = 1.0f - std::exp(-kTwoPi * 6500.0f / sr);
        stealStep = 1.0f / (0.003f * sr);
        initialised = false;
        reset();
    }

    void reset()
    {
        for (int i = 0; i < kMaxVoices; ++i) {
            voices[i] = Voice();
            voices[i].rng.seed(0x1234567u + 7919u * (uint32_t) i);
            const float pan = ((float) i / (float) (kMaxVoices - 1) - 0.5f) * 0.8f;     // -0.4 .. 0.4
            voices[i].gL = std::cos((pan + 1.0f) * kPi * 0.25f);
            voices[i].gR = std::sin((pan + 1.0f) * kPi * 0.25f);
        }
        heldCount = 0; orderCounter = 0; bend = bendS = 0.0f; sustainPedal = false; haveLast = false;
        lfoPh = 0; lfoOut = 0; shValue = 0; lfoRng.seed(99u);
        chorus.clear(); delay.clear(); reverb.clear();
        toneLpL = toneLpR = distLpL = distLpR = 0.0f;
    }

    void setTempo(double bpm) noexcept { tempo = (float) std::min(400.0, std::max(20.0, bpm)); }

    void setParams(const float* in) noexcept
    {
        for (int i = 0; i < P_COUNT; ++i) {
            float v = in[i];
            const ParamDef& d = kParams[i];
            if (!std::isfinite(v)) v = d.def;
            p[i] = std::min(std::max(v, d.mn), d.mx);
        }
        numVoices = std::min(kMaxVoices, std::max(1, (int) std::lround(p[P_POLY])));
        const bool newLegato = p[P_LEGATO] > 0.5f;
        if (newLegato != legato) heldCount = 0;
        legato = newLegato;

        envA = makeEnv(p[P_AA], p[P_AD], p[P_AS], p[P_AR]);
        envF = makeEnv(p[P_FA], p[P_FD], p[P_FS], p[P_FR]);
        glideC = p[P_GLIDE] < 0.5f ? 1.0f : 1.0f - std::exp(-4.6f / (p[P_GLIDE] * 0.001f * sr));
        r1 = std::exp2(-p[P_DETUNE] / 2400.0f);
        r2 = std::exp2((float) std::lround(p[P_O2SEMI]) / 12.0f + p[P_DETUNE] / 2400.0f);

        sm[S_CUT].t    = std::log2(p[P_CUTOFF]);
        sm[S_RES].t    = p[P_RES];
        sm[S_PW].t     = p[P_PW];
        sm[S_L1].t     = p[P_O1LEVEL];
        sm[S_L2].t     = p[P_O2LEVEL];
        sm[S_LS].t     = p[P_SUB];
        sm[S_LN].t     = p[P_NOISE];
        sm[S_GRIT].t   = p[P_GRIT];
        sm[S_DMIX].t   = p[P_DISTON] > 0.5f ? p[P_DISTMIX] : 0.0f;
        sm[S_CMIX].t   = p[P_CHON]   > 0.5f ? p[P_CHMIX]   : 0.0f;
        sm[S_DLMIX].t  = p[P_DLON]   > 0.5f ? p[P_DLMIX]   : 0.0f;
        sm[S_RVMIX].t  = p[P_RVON]   > 0.5f ? p[P_SPACEMIX] : 0.0f;
        sm[S_TONE].t   = p[P_TONE];
        sm[S_VOL].t    = p[P_VOLUME] <= -47.9f ? 0.0f : std::pow(10.0f, p[P_VOLUME] / 20.0f);
        sm[S_DLTIME].t = p[P_DLTIME] * 0.001f * sr;
        if (!initialised) { for (auto& s : sm) s.snap(s.t); initialised = true; }
    }

    // ---- MIDI
    void noteOn(int note, float vel) noexcept
    {
        note = std::min(127, std::max(0, note));
        if (vel <= 0.0f) { noteOff(note); return; }
        vel = std::min(1.0f, vel);
        if (legato) {
            pushHeld(note);
            Voice& v = voices[0];
            if (v.active && !v.released && !v.stealing) {
                v.note = note; v.vel = vel; v.lt = std::log2(mtof(note)); lastLf = v.lt; haveLast = true;
                return;
            }
            allocate(note, vel, 1);
            return;
        }
        allocate(note, vel, numVoices);
    }

    void noteOff(int note) noexcept
    {
        note = std::min(127, std::max(0, note));
        if (legato) {
            removeHeld(note);
            Voice& v = voices[0];
            if (v.active && !v.stealing && v.note == note && !v.released) {
                if (heldCount > 0) { v.note = held[heldCount - 1]; v.lt = std::log2(mtof(v.note)); lastLf = v.lt; }
                else releaseVoice(v);
            } else if (v.stealing && v.pendNote == note && heldCount == 0) v.pendRelease = true;
            return;
        }
        for (int i = 0; i < kMaxVoices; ++i) {
            Voice& v = voices[i];
            if (v.stealing) { if (v.pendNote == note) v.pendRelease = true; }
            else if (v.active && v.note == note && !v.released && !v.sustained) releaseVoice(v);
        }
    }

    void allNotesOff() noexcept
    {
        heldCount = 0; sustainPedal = false;
        for (auto& v : voices) {
            if (v.stealing) { v.pendNote = -1; continue; }
            if (v.active) { v.sustained = false; v.released = true; v.amp.gateOff(); v.flt.gateOff(); }
        }
    }

    void pitchBend(float v) noexcept { bend = std::min(1.0f, std::max(-1.0f, v)); }

    void sustain(bool down) noexcept
    {
        sustainPedal = down;
        if (!down) for (auto& v : voices) if (v.active && v.sustained) { v.sustained = false; v.released = true; v.amp.gateOff(); v.flt.gateOff(); }
    }

    int activeVoices() const noexcept { int n = 0; for (auto& v : voices) if (v.active) ++n; return n; }

    // ---- audio
    void process(float* L, float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i) {
            Frame f;
            f.cutLog = sm[S_CUT].tick();
            const float res = std::min(1.0f, std::max(0.0f, sm[S_RES].tick()));
            f.k  = 1.9f - 1.78f * res;
            f.pw = std::min(0.95f, std::max(0.05f, sm[S_PW].tick()));
            f.l1 = sm[S_L1].tick(); f.l2 = sm[S_L2].tick(); f.ls = sm[S_LS].tick(); f.ln = sm[S_LN].tick();
            f.lfo = tickLfo();
            bendS += (bend - bendS) * bendC;
            f.bendSemis = bendS * p[P_PBRANGE];

            float l = 0.0f, r = 0.0f;
            for (auto& v : voices) {
                if (!v.active) continue;
                const float s = renderVoice(v, f);
                l += s * v.gL; r += s * v.gR;
            }
            l *= 0.3f; r *= 0.3f;

            // distorsion / saturacion
            const float dm = sm[S_DMIX].tick(), gr = sm[S_GRIT].tick();
            if (dm > 1.0e-4f) {
                const float g = 1.0f + gr * 30.0f;
                const float sc = 0.3f / std::tanh(g * 0.3f);
                const float b = 0.2f, tb = std::tanh(b);
                const float wl = (std::tanh(g * l + b) - tb) * sc;
                const float wr = (std::tanh(g * r + b) - tb) * sc;
                distLpL += (wl - distLpL) * distA; distLpR += (wr - distLpR) * distA;
                l = l * (1.0f - dm) + distLpL * dm;
                r = r * (1.0f - dm) + distLpR * dm;
            }
            // chorus
            chorus.process(l, r, p[P_CHRATE], p[P_CHDEPTH], sm[S_CMIX].tick());
            // delay
            delay.process(l, r, sm[S_DLTIME].tick(), p[P_DLFB], sm[S_DLMIX].tick());
            // reverb oscura
            const float rm = sm[S_RVMIX].tick();
            float wl, wr;
            reverb.process(l, r, p[P_RVSIZE], p[P_DARKSPACE], wl, wr);
            l = l * (1.0f - 0.4f * rm) + wl * rm;
            r = r * (1.0f - 0.4f * rm) + wr * rm;
            // tono (tilt oscuro <-> brillante)
            const float tone = sm[S_TONE].tick();
            toneLpL += (l - toneLpL) * toneA; toneLpR += (r - toneLpR) * toneA;
            toneLpL = flushDenorm(toneLpL); toneLpR = flushDenorm(toneLpR);
            const float lowG = std::exp2((0.5f - tone) * 1.5f), highG = std::exp2((tone - 0.5f) * 1.5f);
            l = toneLpL * lowG + (l - toneLpL) * highG;
            r = toneLpR * lowG + (r - toneLpR) * highG;
            // volumen + proteccion anti-clipping
            const float vol = sm[S_VOL].tick();
            l = softClip(l * vol); r = softClip(r * vol);
            if (!std::isfinite(l)) l = 0.0f;
            if (!std::isfinite(r)) r = 0.0f;
            L[i] = l; R[i] = r;
        }
    }

private:
    enum { S_CUT, S_RES, S_PW, S_L1, S_L2, S_LS, S_LN, S_GRIT, S_DMIX, S_CMIX, S_DLMIX, S_RVMIX, S_TONE, S_VOL, S_DLTIME, S_COUNT };

    struct Frame { float cutLog = 0, k = 1, pw = 0.5f, l1 = 0, l2 = 0, ls = 0, ln = 0, lfo = 0, bendSemis = 0; };

    struct Voice {
        bool active = false, released = false, sustained = false, stealing = false, pendRelease = false;
        int note = -1, pendNote = -1;
        float vel = 0, pendVel = 0, stealGain = 1.0f;
        float ph1 = 0, ph2 = 0, phs = 0;
        float lf = 8.0f, lt = 8.0f;           // log2(freq) actual y objetivo (glide)
        float drift = 0, driftTarget = 0; int driftCount = 0;
        float gL = 0.7f, gR = 0.7f;
        uint32_t order = 0;
        Adsr amp, flt; Svf filt; Rng rng;
    };

    float sr = 44100.0f, tempo = 120.0f;
    float p[P_COUNT] = {};
    std::array<Smooth, S_COUNT> sm;
    bool initialised = false, legato = false, sustainPedal = false, haveLast = false;
    int numVoices = 6;
    EnvP envA, envF;
    float glideC = 1, r1 = 1, r2 = 1, bendC = 0.01f, driftC = 0.001f, lfoC = 0.1f, toneA = 0.1f, distA = 0.5f, stealStep = 0.01f;
    float bend = 0, bendS = 0, lastLf = 8.0f;
    float lfoPh = 0, lfoOut = 0, shValue = 0; Rng lfoRng;
    float toneLpL = 0, toneLpR = 0, distLpL = 0, distLpR = 0;
    uint32_t orderCounter = 0;
    int held[32] = {}; int heldCount = 0;
    Voice voices[kMaxVoices];
    Chorus chorus; Delay delay; Reverb reverb;

    EnvP makeEnv(float a, float d, float s, float r) const noexcept
    {
        EnvP e;
        e.aInc = 1.0f / (std::max(a, 0.001f) * sr);
        e.dC = 1.0f - std::exp(-4.6f / (std::max(d, 0.002f) * sr));
        e.sus = s;
        e.rC = 1.0f - std::exp(-4.6f / (std::max(r, 0.002f) * sr));
        return e;
    }

    void pushHeld(int n) noexcept
    {
        removeHeld(n);
        if (heldCount >= 32) { for (int i = 1; i < 32; ++i) held[i - 1] = held[i]; heldCount = 31; }
        held[heldCount++] = n;
    }
    void removeHeld(int n) noexcept
    {
        int j = 0;
        for (int i = 0; i < heldCount; ++i) if (held[i] != n) held[j++] = held[i];
        heldCount = j;
    }

    void releaseVoice(Voice& v) noexcept
    {
        if (sustainPedal) { v.sustained = true; return; }
        v.released = true; v.amp.gateOff(); v.flt.gateOff();
    }

    void startVoice(Voice& v, int note, float vel) noexcept
    {
        v.note = note; v.vel = vel; v.released = false; v.sustained = false; v.order = ++orderCounter;
        v.lt = std::log2(mtof(note));
        v.lf = (p[P_GLIDE] >= 0.5f && haveLast) ? lastLf : v.lt;
        lastLf = v.lt; haveLast = true;
        v.amp.level = 0.0f; v.flt.level = 0.0f; v.filt.reset();
        v.ph1 = 0.5f * (v.rng.next() + 1.0f); v.ph2 = 0.5f * (v.rng.next() + 1.0f); v.phs = 0.5f * (v.rng.next() + 1.0f);
        v.driftCount = 0;
        v.amp.gateOn(); v.flt.gateOn();
        v.active = true; v.stealing = false; v.stealGain = 1.0f; v.pendNote = -1; v.pendRelease = false;
    }

    int pickVoice(int limit, int note) const noexcept
    {
        for (int i = 0; i < limit; ++i) if (voices[i].active && !voices[i].stealing && voices[i].note == note) return i;
        for (int i = 0; i < limit; ++i) if (!voices[i].active) return i;
        int best = -1; float bestLvl = 2.0f;
        for (int i = 0; i < limit; ++i)
            if (voices[i].released && !voices[i].stealing && voices[i].amp.level < bestLvl) { bestLvl = voices[i].amp.level; best = i; }
        if (best >= 0) return best;
        uint32_t bo = 0xFFFFFFFFu;
        for (int i = 0; i < limit; ++i) if (!voices[i].stealing && voices[i].order < bo) { bo = voices[i].order; best = i; }
        if (best >= 0) return best;
        bo = 0xFFFFFFFFu;
        for (int i = 0; i < limit; ++i) if (voices[i].order <= bo) { bo = voices[i].order; best = i; }
        return best < 0 ? 0 : best;
    }

    void allocate(int note, float vel, int limit) noexcept
    {
        Voice& v = voices[pickVoice(limit, note)];
        if (!v.stealing && (!v.active || v.amp.level < 0.002f)) { startVoice(v, note, vel); return; }
        v.stealing = true; v.pendNote = note; v.pendVel = vel; v.pendRelease = false;   // fade corto antes de reutilizar
    }

    float tickLfo() noexcept
    {
        const float div[6] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 3.0f };   // ciclos por negra
        float rate = p[P_LFORATE];
        if (p[P_LFOSYNC] > 0.5f) rate = tempo / 60.0f * div[std::min(5, std::max(0, (int) std::lround(p[P_LFODIV])))];
        lfoPh += rate / sr;
        if (lfoPh >= 1.0f) { lfoPh -= 1.0f; shValue = lfoRng.next(); }
        float raw;
        switch ((int) std::lround(p[P_LFOSHAPE])) {
        case 0:  raw = std::sin(kTwoPi * lfoPh); break;
        case 1:  raw = 1.0f - 4.0f * std::fabs(lfoPh - 0.5f); break;
        case 2:  raw = lfoPh < 0.5f ? 1.0f : -1.0f; break;
        default: raw = shValue; break;
        }
        lfoOut += (raw - lfoOut) * lfoC;
        return lfoOut;
    }

    float renderVoice(Voice& v, const Frame& f) noexcept
    {
        v.lf += (v.lt - v.lf) * glideC;
        if (--v.driftCount <= 0) {
            v.driftTarget = v.rng.next();
            v.driftCount = (int) (sr * (0.25f + 0.35f * (0.5f * v.rng.next() + 0.5f)));
        }
        v.drift += (v.driftTarget - v.drift) * driftC;
        const float dr = p[P_DRIFT];
        const float semis = f.bendSemis + f.lfo * p[P_LFOPITCH] + v.drift * dr * 0.35f;
        const float base = std::exp2(v.lf + semis / 12.0f);
        const float dt1 = std::min(0.45f, base * r1 / sr), dt2 = std::min(0.45f, base * r2 / sr), dts = std::min(0.45f, base * 0.5f / sr);

        v.ph1 += dt1; if (v.ph1 >= 1.0f) v.ph1 -= 1.0f;
        v.ph2 += dt2; if (v.ph2 >= 1.0f) v.ph2 -= 1.0f;
        v.phs += dts; if (v.phs >= 1.0f) v.phs -= 1.0f;

        const float pw = std::min(0.95f, std::max(0.05f, f.pw + f.lfo * p[P_LFOPW] * 0.4f));
        float s = oscSample((int) std::lround(p[P_O1WAVE]), v.ph1, dt1, pw) * f.l1
                + oscSample((int) std::lround(p[P_O2WAVE]), v.ph2, dt2, pw) * f.l2
                + std::tanh(2.0f * std::sin(kTwoPi * v.phs)) * 0.7f * f.ls
                + v.rng.next() * f.ln;

        const float ae = v.amp.tick(envA), fe = v.flt.tick(envF);
        const float oct = p[P_FENV] * fe * 5.0f + f.lfo * p[P_LFOCUT] * 3.0f
                        + p[P_KEYTRACK] * (v.lf - 8.0305f) + v.drift * dr * 0.15f + 0.8f * (v.vel - 0.5f);
        float fc = std::exp2(f.cutLog + oct);
        fc = std::min(fc, std::min(0.45f * sr, 20000.0f)); fc = std::max(fc, 15.0f);
        const float g = std::tan(kPi * fc / sr);
        float y = v.filt.process(s, g, f.k);
        y *= ae * (0.35f + 0.65f * v.vel) * v.stealGain;

        if (v.stealing) {
            v.stealGain -= stealStep;
            if (v.stealGain <= 0.0f) {
                if (v.pendNote >= 0) {
                    const bool rel = v.pendRelease;
                    startVoice(v, v.pendNote, v.pendVel);
                    if (rel) releaseVoice(v);
                } else { v.active = false; v.stealing = false; v.stealGain = 1.0f; }
            }
        } else if (v.amp.stage == Adsr::Idle) v.active = false;
        return y;
    }
};

} // namespace bf
