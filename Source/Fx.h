#pragma once
#include "Common.h"
#include "Params.h"

namespace atm
{
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

class DelayLine
{
public:
    void prepare (int maxSamples) { size = maxSamples + 4; buf.assign ((size_t) size, 0.0f); wp = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); wp = 0; }
    int getSize() const noexcept { return size; }
    void push (float x) noexcept { buf[(size_t) wp] = x; if (++wp >= size) wp = 0; }

    // d = 0 returns the most recently pushed sample
    float read (float d) const noexcept
    {
        float rp = (float) wp - 1.0f - d;
        while (rp < 0.0f) rp += (float) size;
        int i0 = (int) rp;
        const float fr = rp - (float) i0;
        if (i0 >= size) i0 -= size;
        int i1 = i0 + 1;
        if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }

private:
    std::vector<float> buf;
    int size = 4, wp = 0;
};

inline float onePoleCoef (float hz, double sr) { return 1.0f - std::exp (-kTwoPi * hz / (float) sr); }

//==============================================================================
// Distortion / wavefolder / bit crusher / sample-rate reducer for the master bus.
class GritFx
{
public:
    void reset() { cnt[0] = cnt[1] = 0.0f; held[0] = held[1] = 0.0f; }

    void process (float* l, float* r, int n, float drive, float fold, float bits, float rate, float mix)
    {
        if (drive < 0.002f && fold < 0.002f && bits < 0.005f && rate < 0.005f)
            return;

        const float g = 1.0f + drive * drive * 30.0f;
        const float comp = 1.0f / (1.0f + drive * 0.6f);
        const float foldGain = 1.0f + fold * 8.0f;
        const float foldBlend = juce::jmin (1.0f, fold * 3.0f);
        const float levels = std::pow (2.0f, 15.0f - 13.0f * bits);
        const float hold = 1.0f + 31.0f * rate * rate;
        float* ch[2] = { l, r };

        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float dry = ch[c][i];
                float y = dry;
                if (drive > 0.002f) y = std::tanh (y * g) * comp;
                if (fold > 0.002f)  y += foldBlend * (std::sin (y * foldGain) - y);
                if (bits > 0.005f)  y = std::round (y * levels) / levels;
                if (rate > 0.005f)
                {
                    cnt[c] += 1.0f;
                    if (cnt[c] >= hold) { cnt[c] -= hold; held[c] = y; }
                    y = held[c];
                }
                ch[c][i] = dry + (y - dry) * mix;
            }
    }

private:
    float cnt[2] { 0, 0 }, held[2] { 0, 0 };
};

//==============================================================================
class ChorusFx
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        lines[0].prepare ((int) (0.06 * sr)); lines[1].prepare ((int) (0.06 * sr));
        reset();
    }
    void reset() { lines[0].reset(); lines[1].reset(); phase = 0.0f; }

    void process (float* l, float* r, int n, float rateHz, float depth01, float mix)
    {
        if (mix < 0.001f) { phase += rateHz * (float) n / sampleRate; phase -= std::floor (phase); return; }
        const float inc = rateHz / sampleRate;
        const float depth = depth01 * 4.0f * 0.001f * sampleRate;
        const float base = 0.012f * sampleRate;
        float* ch[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                lines[c].push (ch[c][i]);
                const float ph = phase + (c == 1 ? 0.35f : 0.0f);
                const float d1 = juce::jmax (0.0f, base + depth * std::sin (kTwoPi * ph));
                const float d2 = juce::jmax (0.0f, base * 1.2f + depth * std::sin (kTwoPi * (ph + 0.37f)));
                const float wet = 0.5f * (lines[c].read (d1) + lines[c].read (d2));
                ch[c][i] += (wet - ch[c][i]) * mix;
            }
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    float sampleRate = 44100.0f, phase = 0.0f;
    DelayLine lines[2];
};

//==============================================================================
class DelayFx
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        lineL.prepare ((int) (sr * 1.1)); lineR.prepare ((int) (sr * 1.1));
        timeSm.reset (sr, 0.1);
        reset();
    }
    void reset() { lineL.reset(); lineR.reset(); fbL = fbR = 0.0f; first = true; }

    void process (float* l, float* r, int n, float timeMs, float fb, float tone01, float mix)
    {
        if (first) { timeSm.setCurrentAndTargetValue (timeMs); first = false; }
        timeSm.setTargetValue (timeMs);
        const float a = std::exp (-kTwoPi * map::delayToneHz (tone01) / sampleRate);
        const float maxT = (float) lineL.getSize() - 6.0f;
        const float pp = 0.6f;

        for (int i = 0; i < n; ++i)
        {
            const float t = juce::jlimit (2.0f, maxT, timeSm.getNextValue() * 0.001f * sampleRate);
            const float dl = lineL.read (t - 1.0f), dr = lineR.read (t - 1.0f);
            fbL = (1.0f - a) * dl + a * fbL;
            fbR = (1.0f - a) * dr + a * fbR;
            const float inL = l[i], inR = r[i], m = 0.5f * (inL + inR);
            lineL.push (inL * (1.0f - pp) + m * pp + fb * (fbL * (1.0f - pp) + fbR * pp));
            lineR.push (inR * (1.0f - pp) + fb * (fbR * (1.0f - pp) + fbL * pp));
            l[i] = inL + mix * dl;
            r[i] = inR + mix * dr;
        }
    }

private:
    float sampleRate = 44100.0f, fbL = 0, fbR = 0;
    bool first = true;
    DelayLine lineL, lineR;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> timeSm;
};

//==============================================================================
// Delay-line pitch shifter (one octave up), used for the reverb shimmer.
class PitchShifter
{
public:
    void prepare (int maxSamples) { line.prepare (maxSamples); phase = 0.0f; }
    void reset() { line.reset(); phase = 0.0f; }

    float process (float x, float ratio, float W)
    {
        line.push (x);
        float pB = phase + 0.5f;
        if (pB >= 1.0f) pB -= 1.0f;
        const float a = line.read (phase * W), b = line.read (pB * W);
        const float wA = 0.5f - 0.5f * std::cos (kTwoPi * phase);
        phase += (1.0f - ratio) / W;
        if (phase < 0.0f) phase += 1.0f;
        if (phase >= 1.0f) phase -= 1.0f;
        return a * wA + b * (1.0f - wA);
    }

private:
    DelayLine line;
    float phase = 0.0f;
};

//==============================================================================
// 8-line feedback delay network with Hadamard mixing, damping, slow modulation,
// input diffusion, pre-delay and an optional octave-up shimmer in the feedback path.
class FdnReverb
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        const int maxLine = (int) (0.3 * sr);
        for (auto& ln : lines) ln.prepare (maxLine);
        for (int i = 0; i < 3; ++i)
        {
            apL[i].line.prepare ((int) (0.03 * sr)); apR[i].line.prepare ((int) (0.03 * sr));
            apL[i].len = apLenMs[i] * 0.001f * sampleRate;
            apR[i].len = (apLenMs[i] + 0.7f) * 0.001f * sampleRate;
        }
        preL.prepare ((int) (0.25 * sr)); preR.prepare ((int) (0.25 * sr));
        shifter.prepare ((int) (0.2 * sr));
        sizeSm.reset (sr, 1.0);
        preSm.reset (sr, 0.1);
        reset();
    }

    void reset()
    {
        for (auto& ln : lines) ln.reset();
        for (int i = 0; i < 3; ++i) { apL[i].line.reset(); apR[i].line.reset(); }
        preL.reset(); preR.reset(); shifter.reset();
        for (int i = 0; i < 8; ++i) { damp[i] = 0.0f; modPh[i] = (float) i / 8.0f; }
        shimHp = shimLp = 0.0f;
        first = true;
    }

    void process (float* l, float* r, int n, float size01, float decay01, float damp01, float pre01, float shim01, float mix)
    {
        const float scale = map::reverbScale (size01);
        const float rt60 = map::reverbRt60 (decay01);
        const float preSamples = map::reverbPreMs (pre01) * 0.001f * sampleRate;
        if (first) { sizeSm.setCurrentAndTargetValue (scale); preSm.setCurrentAndTargetValue (preSamples); first = false; }
        sizeSm.setTargetValue (scale);
        preSm.setTargetValue (preSamples);

        const float dampCoef = damp01 * 0.85f;
        const float shimAmt = shim01 * 0.4f;
        const float hpA = onePoleCoef (180.0f, sampleRate);
        const float lpA = onePoleCoef (6000.0f, sampleRate);
        const float modDepth = 0.00025f * sampleRate;
        const float modInc = 0.25f / sampleRate;

        for (int i = 0; i < n; ++i)
        {
            const float sc = sizeSm.getNextValue();
            if ((cnt++ & 31) == 0)
            {
                float gSum = 0.0f;
                for (int k = 0; k < 8; ++k)
                {
                    const float lenSec = baseMs[k] * 0.001f * sc;
                    gain[k] = juce::jmin (0.9995f, std::pow (10.0f, -3.0f * lenSec / rt60));
                    gSum += gain[k];
                }
                const float ga = gSum / 8.0f;
                inGain = std::sqrt (juce::jmax (0.0001f, 1.0f - ga * ga)) * 0.9f;
            }

            float d[8];
            for (int k = 0; k < 8; ++k)
            {
                modPh[k] += modInc * (0.8f + 0.07f * (float) k);
                if (modPh[k] >= 1.0f) modPh[k] -= 1.0f;
                const float len = baseMs[k] * 0.001f * sampleRate * sc + modDepth * std::sin (kTwoPi * modPh[k]);
                d[k] = lines[k].read (len - 1.0f);
            }

            const float wetL = 0.3535f * (d[0] - d[1] + d[2] - d[3] + d[4] - d[5] + d[6] - d[7]);
            const float wetR = 0.3535f * (d[0] + d[1] - d[2] - d[3] + d[4] + d[5] - d[6] - d[7]);

            // shimmer: octave-up of the tail fed back into the input
            float shim = 0.0f;
            if (shimAmt > 0.001f)
            {
                float s = shifter.process (0.5f * (wetL + wetR), 2.0f, 0.06f * sampleRate);
                shimHp += hpA * (s - shimHp);
                s -= shimHp;
                shimLp += lpA * (s - shimLp);
                shim = std::tanh (shimLp * shimAmt * 2.0f);
            }

            // input path: pre-delay, diffusion
            const float pd = preSm.getNextValue();
            preL.push (l[i]); preR.push (r[i]);
            float inL = preL.read (pd), inR = preR.read (pd);
            for (int k = 0; k < 3; ++k) { inL = apL[k].process (inL); inR = apR[k].process (inR); }
            inL = inL * inGain + shim * 0.5f;
            inR = inR * inGain + shim * 0.5f;

            // damping + decay
            float y[8];
            for (int k = 0; k < 8; ++k)
            {
                const float x = d[k] * gain[k];
                damp[k] = (1.0f - dampCoef) * x + dampCoef * damp[k];
                y[k] = damp[k];
            }
            hadamard (y);
            for (int k = 0; k < 8; ++k)
                lines[k].push (y[k] * 0.35355339f + ((k & 1) ? inR : inL));

            l[i] += wetL * mix;
            r[i] += wetR * mix;
        }
    }

private:
    struct AllPass
    {
        DelayLine line;
        float len = 100.0f;
        float process (float x)
        {
            const float delayed = line.read (len - 1.0f);
            const float v = x + 0.6f * delayed;
            line.push (v);
            return delayed - 0.6f * v;
        }
    };

    static void hadamard (float* v)
    {
        for (int h = 1; h < 8; h *= 2)
            for (int i = 0; i < 8; i += h * 2)
                for (int j = i; j < i + h; ++j)
                {
                    const float a = v[j], b = v[j + h];
                    v[j] = a + b; v[j + h] = a - b;
                }
    }

    static constexpr float baseMs[8] = { 21.5f, 27.6f, 31.7f, 37.5f, 44.0f, 51.0f, 57.3f, 63.9f };
    static constexpr float apLenMs[3] = { 4.2f, 6.8f, 9.7f };

    float sampleRate = 44100.0f;
    DelayLine lines[8];
    AllPass apL[3], apR[3];
    DelayLine preL, preR;
    PitchShifter shifter;
    float damp[8] {}, modPh[8] {}, gain[8] {}, shimHp = 0, shimLp = 0, inGain = 0.3f;
    unsigned cnt = 0;
    bool first = true;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sizeSm, preSm;
};
} // namespace atm
