#pragma once
#include "Common.h"
#include "Params.h"
#include "Synth.h"
#include "Fx.h"

class AtmosProcessor : public juce::AudioProcessor
{
public:
    AtmosProcessor();
    ~AtmosProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Atmos"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override { return (int) atm::presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return atm::presets()[(size_t) juce::jlimit (0, getNumPrograms() - 1, index)].name; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    float v (atm::PId id) const { return ptr[(size_t) id]->load(); }

    std::array<std::atomic<float>*, atm::P_COUNT> ptr {};
    atm::Engine engine;
    atm::GritFx grit;
    atm::ChorusFx chorus;
    atm::DelayFx delay;
    atm::FdnReverb reverb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterSm;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtmosProcessor)
};
