#pragma once
#include "PluginProcessor.h"

class AtmosLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AtmosLookAndFeel();
    juce::Font getLabelFont (juce::Label&) override { return juce::Font (juce::FontOptions (12.0f)); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.5f)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (14.0f)); }
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, atm::PId id, const juce::String& caption, juce::Colour accent);
    void resized() override;

private:
    juce::Label label;
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class Panel : public juce::Component
{
public:
    Panel (juce::String title, juce::Colour accent) : titleText (std::move (title)), colour (accent) {}

    void addKnob (juce::AudioProcessorValueTreeState& s, atm::PId id, const juce::String& caption);
    void addCombo (juce::AudioProcessorValueTreeState& s, atm::PId id, float weight);
    float totalWeight() const;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Item { juce::Component* comp; float weight; bool isKnob; };
    juce::String titleText;
    juce::Colour colour;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<Item> items;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
};

class AtmosEditor : public juce::AudioProcessorEditor
{
public:
    explicit AtmosEditor (AtmosProcessor&);
    ~AtmosEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void layoutRow (juce::Rectangle<int> row, std::initializer_list<Panel*> panels);

    AtmosProcessor& processor;
    AtmosLookAndFeel laf;
    juce::ComboBox presetBox;
    Panel osc1, osc2, sub, mod, filter, amp, fenv, lfo1, lfo2, grit, out, chorus, delay, reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtmosEditor)
};
