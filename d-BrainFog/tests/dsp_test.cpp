// Pruebas del motor DSP de d-BrainFog (sin JUCE). Compilar: ver tests/run_tests.sh
#include "../Source/DSP/BrainFogEngine.h"
#include "../Source/DSP/Presets.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <string>

// ---- contador de asignaciones de memoria (para verificar que process() no asigna)
static std::atomic<bool>   gCount { false };
static std::atomic<size_t> gAllocs { 0 };
void* operator new(size_t n) { if (gCount.load()) gAllocs++; void* p = std::malloc(n ? n : 1); if (!p) throw std::bad_alloc(); return p; }
void* operator new[](size_t n) { if (gCount.load()) gAllocs++; void* p = std::malloc(n ? n : 1); if (!p) throw std::bad_alloc(); return p; }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }

using namespace bf;

static int gFail = 0, gPass = 0;
#define CHECK(cond, ...) do { if (cond) { ++gPass; } else { ++gFail; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Ev { int t; int type; int a; float b; };   // 0 on, 1 off, 2 bend, 3 sustain, 4 allOff

struct Out { std::vector<float> l, r; };

static Out render(const float* params, const std::vector<Ev>& evs, int total, int block, double sr, double bpm = 120.0)
{
    Engine e; e.prepare(sr);
    Out o; o.l.assign((size_t) total, 0.f); o.r.assign((size_t) total, 0.f);
    size_t ei = 0; int pos = 0;
    while (pos < total) {
        const int n = std::min(block, total - pos);
        e.setParams(params); e.setTempo(bpm);
        int cur = 0;
        while (ei < evs.size() && evs[ei].t < pos + n) {
            const int t = std::max(evs[ei].t - pos, 0);
            if (t > cur) { e.process(o.l.data() + pos + cur, o.r.data() + pos + cur, t - cur); cur = t; }
            const Ev& ev = evs[ei++];
            switch (ev.type) { case 0: e.noteOn(ev.a, ev.b); break; case 1: e.noteOff(ev.a); break; case 2: e.pitchBend(ev.b); break; case 3: e.sustain(ev.a != 0); break; default: e.allNotesOff(); }
        }
        if (cur < n) e.process(o.l.data() + pos + cur, o.r.data() + pos + cur, n - cur);
        pos += n;
    }
    return o;
}

static bool allFinite(const Out& o) { for (size_t i = 0; i < o.l.size(); ++i) if (!std::isfinite(o.l[i]) || !std::isfinite(o.r[i])) return false; return true; }
static float peak(const Out& o) { float m = 0; for (size_t i = 0; i < o.l.size(); ++i) m = std::max(m, std::max(std::fabs(o.l[i]), std::fabs(o.r[i]))); return m; }
static float rms(const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i]; return (float) std::sqrt(s / (double) std::max<size_t>(1, b - a)); }
static float maxDelta(const std::vector<float>& x, size_t a, size_t b) { float m = 0; for (size_t i = a + 1; i < b; ++i) m = std::max(m, std::fabs(x[i] - x[i - 1])); return m; }
static float brightness(const std::vector<float>& x, size_t a, size_t b) { double d = 0, s = 0; for (size_t i = a + 1; i < b; ++i) { d += std::fabs(x[i] - x[i - 1]); s += std::fabs(x[i]); } return (float) (d / std::max(s, 1e-9)); }

static void writeWav(const char* path, const Out& o, int sr)
{
    FILE* f = std::fopen(path, "wb"); if (!f) return;
    const uint32_t n = (uint32_t) o.l.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, rate = (uint32_t) sr, byteRate = rate * 4;
    const uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f); std::fwrite(&fmtLen, 4, 1, f);
    std::fwrite(&pcm, 2, 1, f); std::fwrite(&ch, 2, 1, f); std::fwrite(&rate, 4, 1, f); std::fwrite(&byteRate, 4, 1, f);
    std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f); std::fwrite("data", 1, 4, f); std::fwrite(&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) {
        const int16_t a = (int16_t) std::lround(std::max(-1.f, std::min(1.f, o.l[i])) * 32767.f);
        const int16_t b = (int16_t) std::lround(std::max(-1.f, std::min(1.f, o.r[i])) * 32767.f);
        std::fwrite(&a, 2, 1, f); std::fwrite(&b, 2, 1, f);
    }
    std::fclose(f);
}

static std::vector<Ev> melody(int sr)
{   // frase original corta, en La menor, con notas sostenidas y un acorde
    std::vector<Ev> e; auto S = [&](double s) { return (int) (s * sr); };
    const int notes[] = { 57, 60, 64, 62, 60, 59, 57 };
    double t = 0.1;
    for (int n : notes) { e.push_back({ S(t), 0, n, 0.8f }); e.push_back({ S(t + 0.42), 1, n, 0 }); t += 0.5; }
    e.push_back({ S(t), 0, 45, 0.9f }); e.push_back({ S(t), 0, 57, 0.8f }); e.push_back({ S(t), 0, 60, 0.8f }); e.push_back({ S(t), 0, 64, 0.8f });
    e.push_back({ S(t + 2.0), 1, 45, 0 }); e.push_back({ S(t + 2.0), 1, 57, 0 }); e.push_back({ S(t + 2.0), 1, 60, 0 }); e.push_back({ S(t + 2.0), 1, 64, 0 });
    return e;
}

int main(int argc, char** argv)
{
    const bool writeDemos = argc > 1 && std::string(argv[1]) == "--demos";
    const char* demoDir = argc > 2 ? argv[2] : ".";

    for (int i = 0; i < P_COUNT; ++i) if (!kParams[i].id || !kParams[i].name) { std::printf("tabla de parametros incompleta en %d\n", i); return 2; }
    {   // ids unicos
        bool ok = true;
        for (int i = 0; i < P_COUNT; ++i) for (int j = i + 1; j < P_COUNT; ++j) if (!std::strcmp(kParams[i].id, kParams[j].id)) ok = false;
        CHECK(ok, "ids de parametros duplicados");
        bool rng = true; for (int i = 0; i < P_COUNT; ++i) if (!(kParams[i].mn < kParams[i].mx && kParams[i].def >= kParams[i].mn && kParams[i].def <= kParams[i].mx)) rng = false;
        CHECK(rng, "rangos/valores por defecto invalidos");
        std::printf("[1] tabla de %d parametros: ids unicos, rangos validos\n", (int) P_COUNT);
    }

    // ---- 2. presets: estabilidad en varias frecuencias de muestreo y bloques
    std::printf("[2] presets x sample rate x tamano de bloque (finitos, sin clipping, sin clic en inicio)\n");
    const int rates[] = { 44100, 48000, 96000 };
    const int blocks[] = { 1, 64, 480, 512, 1024, 4096 };
    for (int pi = 0; pi < kNumFactoryPresets; ++pi) {
        float v[P_COUNT]; factoryPresetValues(pi, v);
        for (int sr : rates) for (int bs : blocks) {
            if (bs == 1 && sr != 44100) continue;
            const int total = (int) (sr * (bs == 1 ? 1.5 : 6.5));
            auto o = render(v, melody(sr), total, bs, sr);
            CHECK(allFinite(o), "%s sr=%d bs=%d: valores no finitos", kFactoryPresetNames[pi], sr, bs);
            CHECK(peak(o) <= 1.0f, "%s sr=%d bs=%d: pico %.3f > 1", kFactoryPresetNames[pi], sr, bs, peak(o));
            CHECK(peak(o) > 0.01f, "%s sr=%d bs=%d: silencio inesperado (pico %.5f)", kFactoryPresetNames[pi], sr, bs, peak(o));
        }
    }

    // ---- 3. clic en nota on/off
    std::printf("[3] ausencia de clics en inicio/fin de nota\n");
    for (int pi = 0; pi < kNumFactoryPresets; ++pi) {
        const int sr = 48000; float v[P_COUNT]; factoryPresetValues(pi, v);
        std::vector<Ev> e = { { sr / 2, 0, 60, 0.8f }, { sr * 3 / 2, 1, 60, 0 } };
        auto o = render(v, e, sr * 6, 256, sr);
        const size_t on = (size_t) sr / 2, off = (size_t) sr * 3 / 2;
        // primer sample de la nota: debe empezar en ~0 (la subida posterior es la rampa del ataque, no un salto)
        const float firstMs = [&] { float m = 0; for (size_t i = on; i < on + 2; ++i) m = std::max(m, std::fabs(o.l[i])); return m; }();
        const float sus = maxDelta(o.l, off - (size_t) sr / 2, off);
        const float rel = maxDelta(o.l, off, off + (size_t) sr / 20);
        const float tail = rms(o.l, o.l.size() - 4800, o.l.size());
        std::printf("    %-12s 1er sample=%.4f  maxDelta sost=%.4f rel=%.4f  cola final rms=%.6f\n", kFactoryPresetNames[pi], firstMs, sus, rel, tail);
        CHECK(firstMs < 0.01f, "%s: salto al iniciar nota %.4f", kFactoryPresetNames[pi], firstMs);
        CHECK(rel <= sus * 1.5f + 0.01f, "%s: posible clic en release (%.4f vs %.4f)", kFactoryPresetNames[pi], rel, sus);
    }

    // ---- 4. los presets suenan realmente distintos
    std::printf("[4] presets distintos (RMS / brillo)\n");
    float rmsV[4], brV[4];
    for (int pi = 0; pi < 4; ++pi) {
        const int sr = 48000; float v[P_COUNT]; factoryPresetValues(pi, v);
        std::vector<Ev> e = { { 1000, 0, 57, 0.8f }, { 1000, 0, 64, 0.8f } };
        auto o = render(v, e, sr * 3, 512, sr);
        rmsV[pi] = rms(o.l, (size_t) sr, (size_t) sr * 3); brV[pi] = brightness(o.l, (size_t) sr, (size_t) sr * 3);
        std::printf("    %-12s rms=%.4f brillo=%.4f\n", kFactoryPresetNames[pi], rmsV[pi], brV[pi]);
    }
    for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b) {
        const float dr = std::fabs(rmsV[a] - rmsV[b]) / std::max(rmsV[a], rmsV[b]);
        const float db = std::fabs(brV[a] - brV[b]) / std::max(brV[a], brV[b]);
        CHECK(dr > 0.05f || db > 0.10f, "presets %s y %s demasiado parecidos (dRMS %.2f dBrillo %.2f)", kFactoryPresetNames[a], kFactoryPresetNames[b], dr, db);
    }

    // ---- 5. independencia del tamano de bloque
    std::printf("[5] resultado identico con bloques 64 vs 1024\n");
    for (int pi = 0; pi < 4; ++pi) {
        const int sr = 48000; float v[P_COUNT]; factoryPresetValues(pi, v);
        auto a = render(v, melody(sr), sr * 5, 64, sr), b = render(v, melody(sr), sr * 5, 1024, sr);
        float md = 0; for (size_t i = 0; i < a.l.size(); ++i) md = std::max(md, std::fabs(a.l[i] - b.l[i]));
        std::printf("    %-12s max|diff|=%.2e\n", kFactoryPresetNames[pi], md);
        CHECK(md < 1e-4f, "%s: salida depende del tamano de bloque (%.2e)", kFactoryPresetNames[pi], md);
    }

    // ---- 6. precision de afinacion y pitch bend
    std::printf("[6] afinacion (La4=440 Hz) y pitch bend\n");
    {
        float v[P_COUNT]; for (int i = 0; i < P_COUNT; ++i) v[i] = kParams[i].def;
        v[P_O1WAVE] = 2; v[P_O1LEVEL] = 1; v[P_O2LEVEL] = 0; v[P_SUB] = 0; v[P_NOISE] = 0; v[P_DRIFT] = 0; v[P_LFOPITCH] = 0; v[P_LFOCUT] = 0;
        v[P_DETUNE] = 0; v[P_CUTOFF] = 20000; v[P_RES] = 0; v[P_FENV] = 0; v[P_KEYTRACK] = 0; v[P_DISTON] = 0; v[P_CHON] = 0; v[P_DLON] = 0; v[P_RVON] = 0;
        v[P_TONE] = 0.5f; v[P_VOLUME] = 0; v[P_AS] = 1; v[P_GLIDE] = 0;
        const int sr = 48000;
        auto freq = [&](const Out& o, size_t a, size_t b) { int zc = 0; for (size_t i = a + 1; i < b; ++i) if (o.l[i - 1] < 0 && o.l[i] >= 0) ++zc; return zc * (double) sr / (double) (b - a); };
        auto o = render(v, { { 1000, 0, 69, 0.9f } }, sr * 2, 512, sr);
        const double f0 = freq(o, (size_t) sr / 2, (size_t) sr * 3 / 2);
        std::printf("    La4: %.2f Hz\n", f0);
        CHECK(std::fabs(f0 - 440.0) < 1.5, "afinacion %.2f Hz", f0);
        auto ob = render(v, { { 1000, 0, 69, 0.9f }, { sr / 4, 2, 0, 1.0f } }, sr * 2, 512, sr);
        const double fb = freq(ob, (size_t) sr / 2, (size_t) sr * 3 / 2);
        const double expect = 440.0 * std::pow(2.0, 2.0 / 12.0);
        std::printf("    Pitch bend +2 st: %.2f Hz (esperado %.2f)\n", fb, expect);
        CHECK(std::fabs(fb - expect) < 2.0, "pitch bend %.2f Hz", fb);
        // velocity
        auto lo = render(v, { { 1000, 0, 69, 0.2f } }, sr, 512, sr), hi = render(v, { { 1000, 0, 69, 1.0f } }, sr, 512, sr);
        std::printf("    velocity: rms(0.2)=%.4f rms(1.0)=%.4f\n", rms(lo.l, sr / 2, sr), rms(hi.l, sr / 2, sr));
        CHECK(rms(hi.l, sr / 2, sr) > rms(lo.l, sr / 2, sr) * 1.3f, "velocity no afecta al volumen");
    }

    // ---- 7. polifonia, robo de voces, legato, sustain
    std::printf("[7] polifonia, voice stealing, legato, pedal de sustain\n");
    {
        const int sr = 48000; float v[P_COUNT]; factoryPresetValues(0, v);
        std::vector<Ev> e; for (int i = 0; i < 20; ++i) e.push_back({ 2000 + i * 900, 0, 40 + i * 2, 0.9f });
        v[P_POLY] = 4;
        auto o = render(v, e, sr * 3, 128, sr);
        CHECK(allFinite(o) && peak(o) <= 1.0f, "robo de voces inestable");
        std::printf("    20 notas con 4 voces: pico=%.3f maxDelta=%.3f\n", peak(o), maxDelta(o.l, 0, o.l.size()));
        v[P_POLY] = 8;
        std::vector<Ev> ch; for (int i = 0; i < 8; ++i) ch.push_back({ 1000, 0, 48 + i * 3, 1.0f });
        auto o8 = render(v, ch, sr * 2, 512, sr);
        CHECK(allFinite(o8) && peak(o8) <= 1.0f, "8 voces simultaneas");
        std::printf("    8 voces simultaneas: pico=%.3f\n", peak(o8));
        // notas sostenidas largas
        auto os = render(v, { { 100, 0, 60, 0.8f } }, sr * 30, 512, sr);
        CHECK(allFinite(os) && rms(os.l, sr * 29, sr * 30) > 0.001f, "nota sostenida 30 s se apaga o falla");
        // legato
        float vl[P_COUNT]; factoryPresetValues(0, vl); vl[P_LEGATO] = 1; vl[P_GLIDE] = 150;
        auto ol = render(vl, { { 1000, 0, 60, 0.8f }, { sr / 2, 0, 67, 0.8f }, { sr, 1, 67, 0 }, { sr * 3 / 2, 1, 60, 0 } }, sr * 4, 256, sr);
        CHECK(allFinite(ol) && peak(ol) <= 1.0f, "legato");
        std::printf("    legato+glide: pico=%.3f\n", peak(ol));
        // sustain: tras note-off la nota sigue sonando mientras el pedal esta pulsado
        float vs[P_COUNT]; factoryPresetValues(0, vs); vs[P_AS] = 0.9f; vs[P_AR] = 0.05f;
        auto op = render(vs, { { 1000, 3, 1, 0 }, { 1000, 0, 60, 0.8f }, { sr / 2, 1, 60, 0 }, { sr * 2, 3, 0, 0 } }, sr * 4, 256, sr);
        const float held = rms(op.l, sr, sr * 3 / 2), after = rms(op.l, sr * 3, sr * 4);
        std::printf("    sustain: rms con pedal=%.4f  tras soltar=%.5f\n", held, after);
        CHECK(held > 0.01f && after < held * 0.2f, "pedal de sustain");
    }

    // ---- 8. barrido de parametros extremos / aleatorios
    std::printf("[8] parametros extremos y aleatorios (incluye NaN/inf/fuera de rango)\n");
    {
        const int sr = 44100; Rng rg; rg.seed(2024);
        int bad = 0;
        for (int trial = 0; trial < 120; ++trial) {
            float v[P_COUNT];
            for (int i = 0; i < P_COUNT; ++i) {
                const ParamDef& d = kParams[i];
                const int mode = trial % 4;
                if (mode == 0) v[i] = d.mn; else if (mode == 1) v[i] = d.mx;
                else if (mode == 2) v[i] = d.mn + (0.5f * rg.next() + 0.5f) * (d.mx - d.mn);
                else { const float r = rg.next(); v[i] = r > 0.8f ? NAN : r > 0.6f ? INFINITY : r > 0.4f ? d.mx * 100.f : d.mn + (0.5f * rg.next() + 0.5f) * (d.mx - d.mn); }
            }
            auto o = render(v, melody(sr), (int) (sr * 3.5), 333, sr);
            if (!allFinite(o) || peak(o) > 1.0f) ++bad;
        }
        std::printf("    120 configuraciones: %d con problemas\n", bad);
        CHECK(bad == 0, "%d configuraciones produjeron valores invalidos o clipping", bad);
        // cada parametro por separado: min y max siguen estables y (salvo excepciones) cambian el sonido
        float base[P_COUNT]; factoryPresetValues(0, base);
        int silent = 0;
        for (int i = 0; i < P_COUNT; ++i) {
            if (i == P_LFODIV || i == P_LFOSYNC) continue;   // dependen de otros ajustes
            float a[P_COUNT], b[P_COUNT]; std::memcpy(a, base, sizeof a); std::memcpy(b, base, sizeof b);
            a[i] = kParams[i].mn; b[i] = kParams[i].mx;
            if (i != P_NOISE) a[P_NOISE] = b[P_NOISE] = 0.1f;
            const int sr2 = 48000;
            auto mel = melody(sr2); mel.insert(mel.begin() + 1, { sr2 / 3, 2, 0, 1.0f });   // incluye pitch bend para probar bendRange
            auto oa = render(a, mel, sr2 * 3, 512, sr2), ob = render(b, mel, sr2 * 3, 512, sr2);
            double d = 0; for (size_t k = 0; k < oa.l.size(); ++k) d += std::fabs(oa.l[k] - ob.l[k]);
            if (d < 1e-3) { ++silent; std::printf("    parametro sin efecto audible: %s\n", kParams[i].id); }
        }
        CHECK(silent == 0, "%d parametros sin efecto", silent);
        std::printf("    parametros sin efecto audible: %d\n", silent);
        // LFO sincronizado
        float vs[P_COUNT]; std::memcpy(vs, base, sizeof vs); vs[P_LFOSYNC] = 1; vs[P_LFOPITCH] = 1;
        auto s1 = render(vs, melody(48000), 48000 * 3, 512, 48000, 60.0), s2 = render(vs, melody(48000), 48000 * 3, 512, 48000, 140.0);
        double d = 0; for (size_t k = 0; k < s1.l.size(); ++k) d += std::fabs(s1.l[k] - s2.l[k]);
        CHECK(d > 1.0, "LFO sync no responde al tempo");
    }

    // ---- 9. sin asignaciones de memoria en el camino de audio
    std::printf("[9] asignaciones de memoria durante process()/MIDI\n");
    {
        const int sr = 48000; Engine e; e.prepare(sr);
        float v[P_COUNT]; factoryPresetValues(3, v);
        std::vector<float> l(512), r(512);
        gAllocs = 0; gCount = true;
        for (int blk = 0; blk < 400; ++blk) {
            v[P_CUTOFF] = 200.f + (float) blk * 40.f; e.setParams(v);
            if (blk % 7 == 0) e.noteOn(40 + (blk % 30), 0.8f);
            if (blk % 11 == 0) e.noteOff(40 + ((blk - 11) % 30));
            e.pitchBend(std::sin((float) blk) * 0.5f);
            e.process(l.data(), r.data(), 512);
        }
        gCount = false;
        std::printf("    asignaciones: %zu\n", (size_t) gAllocs.load());
        CHECK(gAllocs.load() == 0, "%zu asignaciones en el camino de audio", (size_t) gAllocs.load());
    }

    // ---- 10. benchmark de CPU
    {
        const int sr = 48000; Engine e; e.prepare(sr);
        float v[P_COUNT]; factoryPresetValues(2, v); v[P_POLY] = 8; e.setParams(v);
        for (int i = 0; i < 8; ++i) e.noteOn(40 + i * 4, 0.8f);
        std::vector<float> l(512), r(512);
        const clock_t t0 = clock(); const int secs = 10;
        for (int b = 0; b < sr * secs / 512; ++b) e.process(l.data(), r.data(), 512);
        const double cpu = (double) (clock() - t0) / CLOCKS_PER_SEC / secs;
        std::printf("[10] CPU con 8 voces + 4 efectos: %.1f %% de un nucleo (esta maquina Linux, -O2)\n", cpu * 100.0);
    }

    if (writeDemos) {
        const int sr = 48000;
        for (int pi = 0; pi < 4; ++pi) {
            float v[P_COUNT]; factoryPresetValues(pi, v);
            auto o = render(v, melody(sr), sr * 8, 512, sr);
            std::string path = std::string(demoDir) + "/" + kFactoryPresetNames[pi] + ".wav";
            for (auto& c : path) if (c == ' ') c = '_';
            writeWav(path.c_str(), o, sr);
            std::printf("demo escrito: %s\n", path.c_str());
        }
    }

    std::printf("\nRESULTADO: %d comprobaciones OK, %d fallos\n", gPass, gFail);
    return gFail ? 1 : 0;
}
