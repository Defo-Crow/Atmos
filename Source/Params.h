#pragma once
#include "Common.h"
#include <array>
#include <vector>

namespace atm
{
enum PId
{
    O1_SHAPE, O1_OCT, O1_FINE, O1_LEVEL, O1_UNI, O1_DET,
    O2_SHAPE, O2_OCT, O2_FINE, O2_LEVEL, O2_UNI, O2_DET,
    SUB_LEVEL, SUB_OCT, NOISE_LEVEL, NOISE_COLOR,
    RING, FM, DRIFT, GLIDE, SPREAD,
    F_TYPE, F_CUT, F_RES, F_ENV, F_KEY,
    A_A, A_D, A_S, A_R,
    E_A, E_D, E_S, E_R,
    L1_SHAPE, L1_DEST, L1_RATE, L1_AMT,
    L2_SHAPE, L2_DEST, L2_RATE, L2_AMT,
    G_DRIVE, G_FOLD, G_BITS, G_RATE, G_MIX,
    C_RATE, C_DEPTH, C_MIX,
    D_TIME, D_FB, D_TONE, D_MIX,
    R_SIZE, R_DECAY, R_DAMP, R_PRE, R_SHIM, R_MIX,
    MASTER,
    P_COUNT
};

enum Kind { kPct, kPctSigned, kIntSigned, kInt, kCents, kHz, kSec, kMs, kShape, kDetune, kGlide, kRvSize, kRvDecay, kPreMs, kToneHz, kChoice };

struct Spec
{
    const char* id;
    const char* name;
    Kind kind;
    float lo, hi, def, skew;
    const char* choices; // '|' separated, only for kChoice
};

// Value mappings shared by the DSP and the value text
namespace map
{
    inline float detuneCents (float v) { return v * 50.0f; }
    inline float glideSec (float v)    { return v * v * 3.0f; }
    inline float reverbScale (float v) { return 0.6f + 1.6f * v; }
    inline float reverbRt60 (float v)  { return 0.5f * std::pow (80.0f, v); }
    inline float reverbPreMs (float v) { return 200.0f * v; }
    inline float delayToneHz (float v) { return 800.0f * std::pow (15.0f, v); }
}

inline const std::array<Spec, P_COUNT>& specs()
{
    static const std::array<Spec, P_COUNT> s = { {
        { "o1_shape", "Osc 1 Shape",  kShape,   0, 1, 0.667f, 1, nullptr },
        { "o1_oct",   "Osc 1 Octave", kIntSigned, -3, 3, 0, 1, nullptr },
        { "o1_fine",  "Osc 1 Fine",   kCents,   -100, 100, 0, 1, nullptr },
        { "o1_level", "Osc 1 Level",  kPct,     0, 1, 0.7f, 1, nullptr },
        { "o1_uni",   "Osc 1 Voices", kInt,     1, 7, 3, 1, nullptr },
        { "o1_det",   "Osc 1 Detune", kDetune,  0, 1, 0.3f, 1, nullptr },

        { "o2_shape", "Osc 2 Shape",  kShape,   0, 1, 0.333f, 1, nullptr },
        { "o2_oct",   "Osc 2 Octave", kIntSigned, -3, 3, 0, 1, nullptr },
        { "o2_fine",  "Osc 2 Fine",   kCents,   -100, 100, 7, 1, nullptr },
        { "o2_level", "Osc 2 Level",  kPct,     0, 1, 0.5f, 1, nullptr },
        { "o2_uni",   "Osc 2 Voices", kInt,     1, 7, 2, 1, nullptr },
        { "o2_det",   "Osc 2 Detune", kDetune,  0, 1, 0.3f, 1, nullptr },

        { "sub_level",   "Sub Level",   kPct, 0, 1, 0.2f, 1, nullptr },
        { "sub_oct",     "Sub Octave",  kIntSigned, -2, -1, -1, 1, nullptr },
        { "noise_level", "Noise Level", kPct, 0, 1, 0.0f, 1, nullptr },
        { "noise_color", "Noise Color", kPct, 0, 1, 0.5f, 1, nullptr },

        { "ring",   "Ring Mod", kPct,    0, 1, 0.0f, 1, nullptr },
        { "fm",     "FM",       kPct,    0, 1, 0.0f, 1, nullptr },
        { "drift",  "Drift",    kPct,    0, 1, 0.25f, 1, nullptr },
        { "glide",  "Glide",    kGlide,  0, 1, 0.0f, 1, nullptr },
        { "spread", "Stereo Spread", kPct, 0, 1, 0.8f, 1, nullptr },

        { "f_type", "Filter Type",   kChoice, 0, 4, 1, 1, "Low-pass 12|Low-pass 24|High-pass|Band-pass|Notch" },
        { "f_cut",  "Cutoff",        kHz,     20, 20000, 3500, 0.25f, nullptr },
        { "f_res",  "Resonance",     kPct,    0, 1, 0.2f, 1, nullptr },
        { "f_env",  "Filter Env",    kPctSigned, -1, 1, 0.2f, 1, nullptr },
        { "f_key",  "Key Track",     kPct,    0, 1, 0.3f, 1, nullptr },

        { "a_a", "Amp Attack",  kSec, 0.002f, 20, 0.8f, 0.3f, nullptr },
        { "a_d", "Amp Decay",   kSec, 0.005f, 20, 2.0f, 0.3f, nullptr },
        { "a_s", "Amp Sustain", kPct, 0, 1, 0.7f, 1, nullptr },
        { "a_r", "Amp Release", kSec, 0.01f, 30, 3.0f, 0.3f, nullptr },

        { "e_a", "Filter Attack",  kSec, 0.002f, 20, 0.5f, 0.3f, nullptr },
        { "e_d", "Filter Decay",   kSec, 0.005f, 20, 3.0f, 0.3f, nullptr },
        { "e_s", "Filter Sustain", kPct, 0, 1, 0.4f, 1, nullptr },
        { "e_r", "Filter Release", kSec, 0.01f, 30, 3.0f, 0.3f, nullptr },

        { "l1_shape", "LFO 1 Shape", kChoice, 0, 4, 0, 1, "Sine|Triangle|Saw|Square|Random" },
        { "l1_dest",  "LFO 1 Target", kChoice, 0, 5, 2, 1, "Off|Pitch|Cutoff|Volume|Shape|Pan" },
        { "l1_rate",  "LFO 1 Rate",  kHz, 0.01f, 30, 0.15f, 0.3f, nullptr },
        { "l1_amt",   "LFO 1 Amount", kPct, 0, 1, 0.25f, 1, nullptr },

        { "l2_shape", "LFO 2 Shape", kChoice, 0, 4, 1, 1, "Sine|Triangle|Saw|Square|Random" },
        { "l2_dest",  "LFO 2 Target", kChoice, 0, 5, 4, 1, "Off|Pitch|Cutoff|Volume|Shape|Pan" },
        { "l2_rate",  "LFO 2 Rate",  kHz, 0.01f, 30, 0.07f, 0.3f, nullptr },
        { "l2_amt",   "LFO 2 Amount", kPct, 0, 1, 0.3f, 1, nullptr },

        { "g_drive", "Grit Drive", kPct, 0, 1, 0, 1, nullptr },
        { "g_fold",  "Grit Fold",  kPct, 0, 1, 0, 1, nullptr },
        { "g_bits",  "Grit Crush", kPct, 0, 1, 0, 1, nullptr },
        { "g_rate",  "Grit Rate",  kPct, 0, 1, 0, 1, nullptr },
        { "g_mix",   "Grit Mix",   kPct, 0, 1, 1, 1, nullptr },

        { "c_rate",  "Chorus Rate",  kHz,  0.05f, 6, 0.4f, 0.4f, nullptr },
        { "c_depth", "Chorus Depth", kPct, 0, 1, 0.4f, 1, nullptr },
        { "c_mix",   "Chorus Mix",   kPct, 0, 1, 0.25f, 1, nullptr },

        { "d_time", "Delay Time",     kMs,     20, 1000, 450, 0.5f, nullptr },
        { "d_fb",   "Delay Feedback", kPct,    0, 0.95f, 0.45f, 1, nullptr },
        { "d_tone", "Delay Tone",     kToneHz, 0, 1, 0.5f, 1, nullptr },
        { "d_mix",  "Delay Mix",      kPct,    0, 1, 0.2f, 1, nullptr },

        { "r_size",  "Reverb Size",    kRvSize,  0, 1, 0.6f, 1, nullptr },
        { "r_decay", "Reverb Decay",   kRvDecay, 0, 1, 0.6f, 1, nullptr },
        { "r_damp",  "Reverb Damping", kPct,     0, 1, 0.5f, 1, nullptr },
        { "r_pre",   "Reverb Pre-delay", kPreMs, 0, 1, 0.1f, 1, nullptr },
        { "r_shim",  "Reverb Shimmer", kPct,     0, 1, 0.0f, 1, nullptr },
        { "r_mix",   "Reverb Mix",     kPct,     0, 1, 0.35f, 1, nullptr },

        { "master", "Master", kPct, 0, 1, 0.7f, 1, nullptr },
    } };
    return s;
}

inline juce::String valueText (Kind k, float v)
{
    using juce::String;
    switch (k)
    {
        case kPct:       return String (juce::roundToInt (v * 100.0f)) + " %";
        case kPctSigned: return String (v > 0.004f ? "+" : "") + String (juce::roundToInt (v * 100.0f)) + " %";
        case kIntSigned: return String (v > 0.5f ? "+" : "") + String (juce::roundToInt (v));
        case kInt:       return String (juce::roundToInt (v));
        case kCents:     return String (v > 0.5f ? "+" : "") + String (juce::roundToInt (v)) + " ct";
        case kHz:        return v < 10.0f ? String (v, 2) + " Hz" : v < 1000.0f ? String (juce::roundToInt (v)) + " Hz" : String (v / 1000.0f, 2) + " kHz";
        case kSec:       return v < 0.1f ? String (juce::roundToInt (v * 1000.0f)) + " ms" : v < 10.0f ? String (v, 2) + " s" : String (v, 1) + " s";
        case kMs:        return String (juce::roundToInt (v)) + " ms";
        case kShape:
        {
            static const char* n[] = { "Sine", "Tri", "Saw", "Square" };
            const float s = juce::jlimit (0.0f, 3.0f, v * 3.0f);
            const int i = juce::jmin (2, (int) s);
            const float f = s - (float) i;
            if (f < 0.12f) return n[i];
            if (f > 0.88f) return n[i + 1];
            return String (n[i]) + "/" + n[i + 1];
        }
        case kDetune:    return "+/-" + String (juce::roundToInt (map::detuneCents (v))) + " ct";
        case kGlide:     { const float s = map::glideSec (v); return s < 0.01f ? String ("Off") : String (s, 2) + " s"; }
        case kRvSize:    return "x" + String (map::reverbScale (v), 2);
        case kRvDecay:   return String (map::reverbRt60 (v), 1) + " s";
        case kPreMs:     return String (juce::roundToInt (map::reverbPreMs (v))) + " ms";
        case kToneHz:    { const float f = map::delayToneHz (v); return f < 1000.0f ? String (juce::roundToInt (f)) + " Hz" : String (f / 1000.0f, 1) + " kHz"; }
        default:         return String (v, 2);
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& s : specs())
    {
        const ParameterID pid { s.id, 1 };
        if (s.kind == kChoice)
        {
            StringArray items; items.addTokens (s.choices, "|", "");
            layout.add (std::make_unique<AudioParameterChoice> (pid, s.name, items, (int) s.def));
        }
        else if (s.kind == kInt || s.kind == kIntSigned)
        {
            const Kind k = s.kind;
            layout.add (std::make_unique<AudioParameterInt> (pid, s.name, (int) s.lo, (int) s.hi, (int) s.def,
                AudioParameterIntAttributes().withStringFromValueFunction ([k] (int v, int) { return valueText (k, (float) v); })));
        }
        else
        {
            const Kind k = s.kind;
            layout.add (std::make_unique<AudioParameterFloat> (pid, s.name,
                NormalisableRange<float> (s.lo, s.hi, 0.0f, s.skew), s.def,
                AudioParameterFloatAttributes().withStringFromValueFunction ([k] (float v, int) { return valueText (k, v); })));
        }
    }
    return layout;
}

struct PresetEntry { PId id; float value; };
struct Preset { const char* name; std::vector<PresetEntry> values; };

// Factory starting points. Parameters not listed keep their defaults.
inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", {} },
        { "Slow Strings", {
            { O1_SHAPE, 0.667f }, { O1_UNI, 5 }, { O1_DET, 0.35f }, { O1_LEVEL, 0.6f },
            { O2_SHAPE, 0.667f }, { O2_UNI, 4 }, { O2_FINE, 7 }, { O2_LEVEL, 0.5f },
            { SUB_LEVEL, 0.2f }, { F_TYPE, 1 }, { F_CUT, 1800 }, { F_RES, 0.15f }, { F_ENV, 0.3f }, { F_KEY, 0.5f },
            { A_A, 2.5f }, { A_D, 2 }, { A_S, 0.8f }, { A_R, 4 }, { E_A, 2 }, { E_D, 3 }, { E_S, 0.4f },
            { L1_DEST, 2 }, { L1_RATE, 0.12f }, { L1_AMT, 0.2f },
            { C_MIX, 0.35f }, { D_MIX, 0.15f }, { R_SIZE, 0.75f }, { R_DECAY, 0.7f }, { R_MIX, 0.4f } } },
        { "Deep Drone", {
            { O1_SHAPE, 0.667f }, { O1_OCT, -1 }, { O1_UNI, 5 }, { O1_DET, 0.25f },
            { O2_SHAPE, 1.0f }, { O2_OCT, -2 }, { O2_UNI, 1 }, { O2_LEVEL, 0.4f }, { O2_FINE, 3 },
            { SUB_LEVEL, 0.5f }, { SUB_OCT, -2 }, { NOISE_LEVEL, 0.08f }, { NOISE_COLOR, 0.9f },
            { DRIFT, 0.6f }, { F_TYPE, 1 }, { F_CUT, 500 }, { F_RES, 0.25f }, { F_ENV, 0.15f }, { F_KEY, 0.2f },
            { A_A, 6 }, { A_D, 3 }, { A_S, 1 }, { A_R, 8 }, { E_A, 6 },
            { L1_DEST, 2 }, { L1_RATE, 0.05f }, { L1_AMT, 0.4f }, { L2_DEST, 1 }, { L2_RATE, 0.08f }, { L2_AMT, 0.08f },
            { R_SIZE, 0.9f }, { R_DECAY, 0.85f }, { R_MIX, 0.5f }, { D_MIX, 0.1f } } },
        { "Glass Shimmer", {
            { O1_SHAPE, 0.15f }, { O1_OCT, 1 }, { O1_UNI, 3 }, { O1_DET, 0.15f }, { O1_LEVEL, 0.6f },
            { O2_SHAPE, 0.333f }, { O2_OCT, 1 }, { O2_FINE, 3 }, { O2_UNI, 1 }, { O2_LEVEL, 0.3f },
            { FM, 0.35f }, { SUB_LEVEL, 0.0f }, { F_TYPE, 0 }, { F_CUT, 9000 }, { F_RES, 0.1f }, { F_ENV, 0.0f },
            { A_A, 0.05f }, { A_D, 2.5f }, { A_S, 0.3f }, { A_R, 5 }, { L1_DEST, 0 }, { L2_AMT, 0.15f },
            { C_MIX, 0.3f }, { D_TIME, 620 }, { D_FB, 0.5f }, { D_MIX, 0.3f },
            { R_SIZE, 0.8f }, { R_DECAY, 0.8f }, { R_SHIM, 0.55f }, { R_MIX, 0.55f } } },
        { "Rust and Concrete", {
            { O1_SHAPE, 1.0f }, { O1_OCT, -1 }, { O1_UNI, 2 }, { O1_DET, 0.1f },
            { O2_SHAPE, 0.667f }, { O2_FINE, 13 }, { O2_UNI, 1 }, { RING, 0.5f }, { FM, 0.2f },
            { NOISE_LEVEL, 0.2f }, { NOISE_COLOR, 0.3f }, { F_TYPE, 3 }, { F_CUT, 900 }, { F_RES, 0.5f }, { F_ENV, 0.4f },
            { A_A, 0.01f }, { A_D, 1.2f }, { A_S, 0.6f }, { A_R, 0.6f }, { E_A, 0.01f }, { E_D, 1.0f },
            { G_DRIVE, 0.5f }, { G_FOLD, 0.25f }, { G_BITS, 0.3f }, { G_RATE, 0.2f }, { G_MIX, 0.8f },
            { L1_SHAPE, 4 }, { L1_DEST, 2 }, { L1_RATE, 6 }, { L1_AMT, 0.3f },
            { R_SIZE, 0.5f }, { R_DECAY, 0.4f }, { R_MIX, 0.25f }, { D_MIX, 0.2f } } },
        { "Machine Hum", {
            { O1_SHAPE, 0.667f }, { O1_OCT, -2 }, { O1_UNI, 1 }, { O2_SHAPE, 1.0f }, { O2_OCT, -1 }, { O2_FINE, 5 }, { O2_UNI, 1 },
            { RING, 0.3f }, { SUB_LEVEL, 0.6f }, { NOISE_LEVEL, 0.05f }, { F_TYPE, 1 }, { F_CUT, 350 }, { F_RES, 0.45f },
            { A_A, 0.5f }, { A_D, 1 }, { A_S, 1 }, { A_R, 1.5f }, { G_DRIVE, 0.6f }, { G_FOLD, 0.1f }, { G_MIX, 0.7f },
            { L1_SHAPE, 3 }, { L1_DEST, 3 }, { L1_RATE, 7 }, { L1_AMT, 0.25f }, { R_MIX, 0.15f } } },
        { "Dust Bed", {
            { O1_LEVEL, 0.15f }, { O2_LEVEL, 0.1f }, { SUB_LEVEL, 0.0f }, { NOISE_LEVEL, 1.0f }, { NOISE_COLOR, 0.6f },
            { F_TYPE, 3 }, { F_CUT, 1500 }, { F_RES, 0.2f }, { F_ENV, 0.0f }, { A_A, 3 }, { A_S, 1 }, { A_R, 5 }, { MASTER, 0.9f },
            { L1_SHAPE, 4 }, { L1_DEST, 2 }, { L1_RATE, 0.3f }, { L1_AMT, 0.5f },
            { C_MIX, 0.4f }, { R_SIZE, 0.85f }, { R_DECAY, 0.75f }, { R_MIX, 0.5f } } },
    };
    return p;
}
} // namespace atm
