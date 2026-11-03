#include "PluginProcessor.h"
#include "PluginEditor.h"

AtmosProcessor::AtmosProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", atm::createLayout())
{
    for (int i = 0; i < atm::P_COUNT; ++i)
        ptr[(size_t) i] = apvts.getRawParameterValue (atm::specs()[(size_t) i].id);
}

bool AtmosProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AtmosProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    grit.reset();
    chorus.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);
    masterSm.reset (sampleRate, 0.02);
    masterSm.setCurrentAndTargetValue (v (atm::MASTER));
}

void AtmosProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    using namespace atm;
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2 || n == 0) return;

    VoiceParams vp;
    for (int o = 0; o < 2; ++o)
    {
        const int b = o == 0 ? O1_SHAPE : O2_SHAPE;
        vp.o[o].shape = v ((PId) (b + 0));
        vp.o[o].semis = v ((PId) (b + 1)) * 12.0f + v ((PId) (b + 2)) / 100.0f;
        vp.o[o].level = v ((PId) (b + 3));
        vp.o[o].uni   = (int) v ((PId) (b + 4));
        vp.o[o].det   = v ((PId) (b + 5));
    }
    vp.subLevel = v (SUB_LEVEL);  vp.subOct = (int) v (SUB_OCT);
    vp.noiseLevel = v (NOISE_LEVEL);  vp.noiseColor = v (NOISE_COLOR);
    vp.ring = v (RING);  vp.fm = v (FM);  vp.drift = v (DRIFT);
    vp.glideSec = map::glideSec (v (GLIDE));  vp.spread = v (SPREAD);
    vp.fType = (int) v (F_TYPE);  vp.cutoff = v (F_CUT);  vp.res = v (F_RES);
    vp.fEnvAmt = v (F_ENV);  vp.keyTrack = v (F_KEY);
    vp.amp  = { v (A_A), v (A_D), v (A_S), v (A_R) };
    vp.fenv = { v (E_A), v (E_D), v (E_S), v (E_R) };

    LfoParams lp;
    lp.shape[0] = (int) v (L1_SHAPE); lp.dest[0] = (int) v (L1_DEST); lp.rate[0] = v (L1_RATE); lp.amt[0] = v (L1_AMT);
    lp.shape[1] = (int) v (L2_SHAPE); lp.dest[1] = (int) v (L2_DEST); lp.rate[1] = v (L2_RATE); lp.amt[1] = v (L2_AMT);

    engine.process (buffer, midi, vp, lp);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    grit.process (l, r, n, v (G_DRIVE), v (G_FOLD), v (G_BITS), v (G_RATE), v (G_MIX));
    chorus.process (l, r, n, v (C_RATE), v (C_DEPTH), v (C_MIX));
    delay.process (l, r, n, v (D_TIME), v (D_FB), v (D_TONE), v (D_MIX));
    reverb.process (l, r, n, v (R_SIZE), v (R_DECAY), v (R_DAMP), v (R_PRE), v (R_SHIM), v (R_MIX));

    masterSm.setTargetValue (v (MASTER));
    auto softClip = [] (float x)
    {
        const float a = std::abs (x);
        if (a <= 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f);
        return x < 0.0f ? -y : y;
    };
    for (int i = 0; i < n; ++i)
    {
        const float g = masterSm.getNextValue();
        l[i] = softClip (l[i] * g);
        r[i] = softClip (r[i] * g);
    }
}

void AtmosProcessor::setCurrentProgram (int index)
{
    using namespace atm;
    index = juce::jlimit (0, (int) atm::presets().size() - 1, index);
    currentProgram = index;

    // start from defaults, then apply the preset values
    for (int i = 0; i < P_COUNT; ++i)
        if (auto* p = apvts.getParameter (specs()[(size_t) i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (specs()[(size_t) i].def));

    for (const auto& e : presets()[(size_t) index].values)
        if (auto* p = apvts.getParameter (specs()[(size_t) e.id].id))
            p->setValueNotifyingHost (p->convertTo0to1 (e.value));
}

juce::AudioProcessorEditor* AtmosProcessor::createEditor()
{
    return new AtmosEditor (*this);
}

void AtmosProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void AtmosProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, (int) atm::presets().size() - 1, (int) state.getProperty ("program", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AtmosProcessor();
}
