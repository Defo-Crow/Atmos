#pragma once
#include "Common.h"
#include "Params.h"

namespace atm
{
constexpr float kTwoPiS = juce::MathConstants<float>::twoPi;
constexpr float kHalfPi = juce::MathConstants<float>::halfPi;
constexpr int kMaxUnison = 7;
constexpr int kControlBlock = 16;

struct Rng
{
    uint32_t s = 22222u;
    float noise() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.0f / 2147483648.0f); }
    float uni() noexcept { return noise() * 0.5f + 0.5f; }
};

inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)         { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// 0 sine, 1 triangle, 2 saw, 3 square. All start in phase with the sine, so they can be morphed.
inline float waveAt (int w, float p, float dt) noexcept
{
    switch (w)
    {
        case 0:  return std::sin (kTwoPiS * p);
        case 1:  { float q = p + 0.25f; q -= std::floor (q); return 1.0f - 4.0f * std::abs (q - 0.5f); }
        case 2:  { float q = p + 0.5f; if (q >= 1.0f) q -= 1.0f; return 2.0f * q - 1.0f - polyBlep (q, dt); }
        default: { float q = p + 0.5f; if (q >= 1.0f) q -= 1.0f;
                   return (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, dt) - polyBlep (q, dt); }
    }
}

inline float shapeAt (float shape01, float p, float dt) noexcept
{
    const float s = juce::jlimit (0.0f, 3.0f, shape01 * 3.0f);
    int i = (int) s;
    float f = s - (float) i;
    if (i >= 3) { i = 2; f = 1.0f; }
    const float a = waveAt (i, p, dt);
    if (f < 0.001f) return a;
    return a + (waveAt (i + 1, p, dt) - a) * f;
}

struct SvfCoef { float a1 = 0, a2 = 0, a3 = 0, k = 1; };

inline SvfCoef makeSvf (float fc, float q, float sr) noexcept
{
    const float g = std::tan (juce::MathConstants<float>::pi * fc / sr);
    SvfCoef c; c.k = 1.0f / q;
    c.a1 = 1.0f / (1.0f + g * (g + c.k));
    c.a2 = g * c.a1;
    c.a3 = g * c.a2;
    return c;
}

struct Svf
{
    float ic1 = 0, ic2 = 0;
    void reset() noexcept { ic1 = ic2 = 0; }
    // mode: 0 low, 1 band (unity peak), 2 high, 3 notch
    float process (float x, const SvfCoef& c, int mode) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        switch (mode)
        {
            case 0:  return v2;
            case 1:  return c.k * v1;
            case 2:  return x - c.k * v1 - v2;
            default: return x - c.k * v1;
        }
    }
};

struct OscSet { float shape = 0.5f, semis = 0.0f, det = 0.0f, level = 0.0f; int uni = 1; };

struct VoiceParams
{
    OscSet o[2];
    float subLevel = 0, noiseLevel = 0, noiseColor = 0.5f;
    int subOct = -1;
    float ring = 0, fm = 0, drift = 0, glideSec = 0, spread = 0.8f;
    int fType = 1;
    float cutoff = 3500, res = 0.2f, fEnvAmt = 0, keyTrack = 0;
    juce::ADSR::Parameters amp, fenv;
};

struct Mod { float pitch = 0, cutOct = 0, gain = 1, shape = 0, pan = 0; };

class Voice
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        amp.setSampleRate (sampleRate);
        fenv.setSampleRate (sampleRate);
        reset();
    }

    void reset()
    {
        active = false; released = true; keyDown = false;
        amp.reset(); fenv.reset();
        for (auto& st : filt) for (auto& f : st) f.reset();
        nLp[0] = nLp[1] = 0.0f;
    }

    bool isActive() const noexcept { return active; }
    bool isKeyDown() const noexcept { return keyDown; }
    int getNote() const noexcept { return midiNote; }
    uint64_t getOrder() const noexcept { return order; }
    void setKeyDown (bool d) noexcept { keyDown = d; }

    void setEnvelopes (const VoiceParams& p) { amp.setParameters (p.amp); fenv.setParameters (p.fenv); }

    void start (int note, float vel, float fromNote, const VoiceParams& vp, uint64_t startOrder, uint32_t seed)
    {
        midiNote = note; velocity = vel; order = startOrder;
        keyDown = true; released = false; active = true;
        rng.s = seed | 1u;
        for (int o = 0; o < 2; ++o)
            for (int u = 0; u < kMaxUnison; ++u) phase[o][u] = rng.uni();
        subPhase = 0.0f;
        curNote = (vp.glideSec > 0.0005f && fromNote >= 0.0f) ? fromNote : (float) note;
        for (auto& st : filt) for (auto& f : st) f.reset();
        nLp[0] = nLp[1] = 0.0f;
        for (int k = 0; k < 2; ++k) { dA[k] = rng.noise(); dB[k] = rng.noise(); }
        dPh = rng.uni();
        fenvLast = 0.0f; gainPrev = -1.0f;
        amp.setParameters (vp.amp); fenv.setParameters (vp.fenv);
        amp.noteOn(); fenv.noteOn();
    }

    void release()
    {
        if (! released) { released = true; amp.noteOff(); fenv.noteOff(); }
    }

    void render (float* L, float* R, int n, const VoiceParams& vp, const Mod& mod, float bend, float mwOct)
    {
        if (! active) return;

        // glide
        const float target = (float) midiNote;
        if (vp.glideSec > 0.0005f) curNote += (target - curNote) * (1.0f - std::exp (-(float) n / (vp.glideSec * sr)));
        else curNote = target;

        // slow random drift (two independent values: pitch and cutoff)
        dPh += (float) n * 0.15f / sr;
        if (dPh >= 1.0f) { dPh -= 1.0f; for (int k = 0; k < 2; ++k) { dA[k] = dB[k]; dB[k] = rng.noise(); } }
        const float tt = dPh * dPh * (3.0f - 2.0f * dPh);
        const float driftPitch = (dA[0] + (dB[0] - dA[0]) * tt) * vp.drift * 0.25f;
        const float driftCut   = (dA[1] + (dB[1] - dA[1]) * tt) * vp.drift * 0.5f;

        const float baseNote = curNote + mod.pitch + bend + driftPitch;

        // oscillator setup
        float inc[2][kMaxUnison], gl[2][kMaxUnison], gr[2][kMaxUnison], sh[2];
        for (int o = 0; o < 2; ++o)
        {
            const auto& os = vp.o[o];
            const int nU = juce::jlimit (1, kMaxUnison, os.uni);
            sh[o] = juce::jlimit (0.0f, 1.0f, os.shape + mod.shape);
            const float norm = os.level * 1.4142f / std::sqrt ((float) nU);
            for (int u = 0; u < nU; ++u)
            {
                const float pos = nU > 1 ? 2.0f * (float) u / (float) (nU - 1) - 1.0f : 0.0f;
                const float semis = baseNote + os.semis + pos * map::detuneCents (os.det) * 0.01f;
                inc[o][u] = juce::jmin (0.45f, 440.0f * std::pow (2.0f, (semis - 69.0f) / 12.0f) / sr);
                const float th = (pos * vp.spread * 0.5f + 0.5f) * kHalfPi;
                gl[o][u] = std::cos (th) * norm;
                gr[o][u] = std::sin (th) * norm;
            }
        }
        const int n1 = juce::jlimit (1, kMaxUnison, vp.o[0].uni), n2 = juce::jlimit (1, kMaxUnison, vp.o[1].uni);
        const bool needO2 = vp.o[1].level > 0.0f || vp.fm > 0.001f || vp.ring > 0.001f;
        const float fmIdx = vp.fm * vp.fm * 4.0f;
        const float subInc = 440.0f * std::pow (2.0f, (baseNote + (float) vp.subOct * 12.0f - 69.0f) / 12.0f) / sr;
        const float nA = onePole (20000.0f * std::pow (0.01f, vp.noiseColor));
        // low-passed noise loses level; compensate so the Noise knob means roughly the same loudness at any colour
        const float noiseGain = vp.noiseLevel * juce::jmin (6.0f, std::sqrt ((2.0f - nA) / nA)) * 0.5f;

        // filter
        const float oct = vp.keyTrack * (curNote - 60.0f) / 12.0f + vp.fEnvAmt * 6.0f * fenvLast + mod.cutOct + mwOct + driftCut;
        const float fc = juce::jlimit (20.0f, 0.45f * sr, vp.cutoff * std::exp2 (oct));
        const float q1 = vp.fType == 1 ? 0.6f + vp.res * vp.res * 8.0f : 0.6f + vp.res * vp.res * 14.0f;
        const SvfCoef c1 = makeSvf (fc, q1, sr);
        const SvfCoef c2 = makeSvf (fc, 0.707f, sr);
        const int mode = vp.fType == 0 || vp.fType == 1 ? 0 : vp.fType == 2 ? 2 : vp.fType == 3 ? 1 : 3;
        const bool twoPole = vp.fType == 1;

        const float gl0 = 1.0f - juce::jmax (0.0f, mod.pan), gr0 = 1.0f + juce::jmin (0.0f, mod.pan);
        const float gStart = gainPrev < 0.0f ? mod.gain : gainPrev;
        gainPrev = mod.gain;

        for (int k = 0; k < n; ++k)
        {
            // oscillator 2 first: its first unison voice is the FM source
            float o2L = 0, o2R = 0, o2c = 0;
            if (needO2)
                for (int u = 0; u < n2; ++u)
                {
                    const float s = shapeAt (sh[1], phase[1][u], inc[1][u]);
                    o2L += s * gl[1][u]; o2R += s * gr[1][u];
                    if (u == 0) o2c = s;
                    phase[1][u] += inc[1][u]; if (phase[1][u] >= 1.0f) phase[1][u] -= 1.0f;
                }

            float o1L = 0, o1R = 0;
            const float fmOff = fmIdx * o2c;
            for (int u = 0; u < n1; ++u)
            {
                float p = phase[0][u] + fmOff;
                p -= std::floor (p);
                const float s = shapeAt (sh[0], p, inc[0][u]);
                o1L += s * gl[0][u]; o1R += s * gr[0][u];
                phase[0][u] += inc[0][u]; if (phase[0][u] >= 1.0f) phase[0][u] -= 1.0f;
            }

            const float sub = std::sin (kTwoPiS * subPhase) * vp.subLevel;
            subPhase += subInc; if (subPhase >= 1.0f) subPhase -= 1.0f;

            nLp[0] += nA * (rng.noise() - nLp[0]);
            nLp[1] += nA * (rng.noise() - nLp[1]);

            const float mixL = o1L + o2L + sub + nLp[0] * noiseGain;
            const float mixR = o1R + o2R + sub + nLp[1] * noiseGain;
            const float inL = mixL + vp.ring * (o1L * o2L * 2.0f - mixL);
            const float inR = mixR + vp.ring * (o1R * o2R * 2.0f - mixR);

            float yl, yr;
            if (twoPole)
            {
                yl = filt[1][0].process (filt[0][0].process (inL, c1, 0), c2, 0);
                yr = filt[1][1].process (filt[0][1].process (inR, c1, 0), c2, 0);
            }
            else
            {
                yl = filt[0][0].process (inL, c1, mode);
                yr = filt[0][1].process (inR, c1, mode);
            }

            fenvLast = fenv.getNextSample();
            const float a = amp.getNextSample();
            if (! amp.isActive()) { active = false; return; }

            const float g = (gStart + (mod.gain - gStart) * (float) (k + 1) / (float) n) * a * velocity * 0.3f;
            L[k] += yl * g * gl0;
            R[k] += yr * g * gr0;
        }
    }

private:
    float onePole (float hz) const { return 1.0f - std::exp (-kTwoPiS * hz / sr); }

    float sr = 44100.0f;
    bool active = false, released = true, keyDown = false;
    int midiNote = 60;
    float velocity = 1.0f, curNote = 60.0f, subPhase = 0.0f, fenvLast = 0.0f, gainPrev = -1.0f;
    float phase[2][kMaxUnison] {};
    float nLp[2] { 0, 0 }, dA[2] { 0, 0 }, dB[2] { 0, 0 }, dPh = 0.0f;
    uint64_t order = 0;
    Rng rng;
    juce::ADSR amp, fenv;
    Svf filt[2][2];
};

struct LfoParams
{
    int shape[2] { 0, 0 }, dest[2] { 0, 0 };
    float rate[2] { 1, 1 }, amt[2] { 0, 0 };
};

class Engine
{
public:
    static constexpr int kVoices = 8;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& v : voices) v.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v.reset();
        bend = 0; mw = 0; sustain = false; lastNote = -1.0f;
        lfoPh[0] = lfoPh[1] = 0.0f;
        for (int k = 0; k < 2; ++k) { rA[k] = rng.noise(); rB[k] = rng.noise(); }
    }

    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const VoiceParams& vp, const LfoParams& lp)
    {
        for (auto& v : voices) v.setEnvelopes (vp);

        int pos = 0;
        const int total = buffer.getNumSamples();
        for (const auto meta : midi)
        {
            const int evPos = juce::jlimit (0, total, meta.samplePosition);
            if (evPos > pos) { renderSegment (buffer, pos, evPos - pos, vp, lp); pos = evPos; }
            handle (meta.getMessage(), vp);
        }
        if (pos < total) renderSegment (buffer, pos, total - pos, vp, lp);
    }

private:
    float lfoValue (int k, int shape)
    {
        const float p = lfoPh[k];
        switch (shape)
        {
            case 0:  return std::sin (kTwoPiS * p);
            case 1:  return 1.0f - 4.0f * std::abs (p - 0.5f) ;
            case 2:  return 2.0f * p - 1.0f;
            case 3:  return p < 0.5f ? 1.0f : -1.0f;
            default: { const float t = p * p * (3.0f - 2.0f * p); return rA[k] + (rB[k] - rA[k]) * t; }
        }
    }

    void renderSegment (juce::AudioBuffer<float>& buffer, int start, int count, const VoiceParams& vp, const LfoParams& lp)
    {
        float* L = buffer.getWritePointer (0) + start;
        float* R = buffer.getWritePointer (1) + start;
        int done = 0;
        while (done < count)
        {
            const int n = juce::jmin (kControlBlock, count - done);

            Mod mod;
            for (int k = 0; k < 2; ++k)
            {
                lfoPh[k] += lp.rate[k] * (float) n / sr;
                if (lfoPh[k] >= 1.0f) { lfoPh[k] -= std::floor (lfoPh[k]); rA[k] = rB[k]; rB[k] = rng.noise(); }
                const float v = lfoValue (k, lp.shape[k]), a = lp.amt[k];
                switch (lp.dest[k])
                {
                    case 1: mod.pitch  += v * a * a * 12.0f; break;
                    case 2: mod.cutOct += v * a * 4.0f; break;
                    case 3: mod.gain   *= 1.0f - a * (0.5f - 0.5f * v); break;
                    case 4: mod.shape  += v * a * 0.4f; break;
                    case 5: mod.pan    += v * a; break;
                    default: break;
                }
            }
            mod.pan = juce::jlimit (-1.0f, 1.0f, mod.pan);

            for (auto& v : voices)
                v.render (L + done, R + done, n, vp, mod, bend, mw * 3.0f);
            done += n;
        }
    }

    void handle (const juce::MidiMessage& m, const VoiceParams& vp)
    {
        if (m.isNoteOn())
        {
            Voice* target = nullptr;
            for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
            if (target == nullptr)
            {
                target = &voices[0];
                for (auto& v : voices) if (v.getOrder() < target->getOrder()) target = &v;
            }
            target->start (m.getNoteNumber(), m.getFloatVelocity(), lastNote, vp, ++counter, rng.s + (uint32_t) counter * 2654435761u);
            lastNote = (float) m.getNoteNumber();
        }
        else if (m.isNoteOff())
        {
            for (auto& v : voices)
                if (v.isActive() && v.isKeyDown() && v.getNote() == m.getNoteNumber())
                {
                    v.setKeyDown (false);
                    if (! sustain) v.release();
                }
        }
        else if (m.isPitchWheel())
            bend = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
        else if (m.isController() && m.getControllerNumber() == 1)
            mw = (float) m.getControllerValue() / 127.0f;
        else if (m.isSustainPedalOn())
            sustain = true;
        else if (m.isSustainPedalOff())
        {
            sustain = false;
            for (auto& v : voices) if (v.isActive() && ! v.isKeyDown()) v.release();
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            for (auto& v : voices) v.reset();
            sustain = false;
        }
    }

    float sr = 44100.0f, bend = 0, mw = 0, lastNote = -1.0f;
    float lfoPh[2] { 0, 0 }, rA[2] { 0, 0 }, rB[2] { 0, 0 };
    bool sustain = false;
    uint64_t counter = 0;
    Rng rng;
    Voice voices[kVoices];
};
} // namespace atm
