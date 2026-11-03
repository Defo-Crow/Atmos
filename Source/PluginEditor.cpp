#include "PluginEditor.h"

namespace
{
const juce::Colour kBg      { 0xff12141a };
const juce::Colour kPanel   { 0xff1c2029 };
const juce::Colour kText    { 0xffd8dee9 };
const juce::Colour kMuted   { 0xff7d879a };
const juce::Colour kOutline { 0xff363d4c };

const juce::Colour cOsc    { 0xff4fd1c5 };
const juce::Colour cSub    { 0xff6fa8f0 };
const juce::Colour cMod    { 0xffb48cf2 };
const juce::Colour cFilter { 0xfff2d45b };
const juce::Colour cEnv    { 0xffe5a35b };
const juce::Colour cLfo    { 0xff8fd35b };
const juce::Colour cGrit   { 0xffee6a5b };
const juce::Colour cFx     { 0xfff27fa5 };
const juce::Colour cOut    { 0xffd8dee9 };
}

AtmosLookAndFeel::AtmosLookAndFeel()
{
    setColourScheme (getDarkColourScheme());
    setColour (juce::Slider::rotarySliderFillColourId, kMuted);
    setColour (juce::Slider::rotarySliderOutlineColourId, kOutline);
    setColour (juce::Slider::thumbColourId, kText);
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::ComboBox::backgroundColourId, kBg);
    setColour (juce::ComboBox::outlineColourId, kOutline);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kText);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kOutline);
}

Knob::Knob (juce::AudioProcessorValueTreeState& state, atm::PId id, const juce::String& caption, juce::Colour accent)
{
    label.setText (caption, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (label);

    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
    slider.setTextBoxIsEditable (false);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, atm::specs()[(size_t) id].id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (15));
    const int w = juce::jmin (r.getWidth(), 90);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::jmin (w, 74), 16);
    slider.setBounds (r.withSizeKeepingCentre (w, r.getHeight()));
}

void Panel::addKnob (juce::AudioProcessorValueTreeState& s, atm::PId id, const juce::String& caption)
{
    auto k = std::make_unique<Knob> (s, id, caption, colour);
    addAndMakeVisible (*k);
    items.push_back ({ k.get(), 1.0f, true });
    owned.push_back (std::move (k));
}

void Panel::addCombo (juce::AudioProcessorValueTreeState& s, atm::PId id, float weight)
{
    auto c = std::make_unique<juce::ComboBox>();
    const char* pid = atm::specs()[(size_t) id].id;
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (pid)))
        c->addItemList (p->choices, 1);
    addAndMakeVisible (*c);
    comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, pid, *c));
    items.push_back ({ c.get(), weight, false });
    owned.push_back (std::move (c));
}

float Panel::totalWeight() const
{
    float t = 0.0f;
    for (auto& i : items) t += i.weight;
    return t;
}

void Panel::paint (juce::Graphics& g)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (kOutline);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (colour);
    g.fillRect (juce::Rectangle<float> (7.0f, 1.0f, (float) getWidth() - 14.0f, 2.0f));
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText (titleText, 12, 6, getWidth() - 24, 16, juce::Justification::left);
}

void Panel::resized()
{
    auto area = getLocalBounds().reduced (6, 4);
    area.removeFromTop (22);
    const float total = totalWeight();
    if (total <= 0.0f) return;

    float x = (float) area.getX();
    for (auto& it : items)
    {
        const float w = (float) area.getWidth() * it.weight / total;
        auto cell = juce::Rectangle<float> (x, (float) area.getY(), w, (float) area.getHeight()).toNearestInt();
        if (it.isKnob)
            it.comp->setBounds (cell);
        else
            it.comp->setBounds (cell.reduced (3, 0).withSizeKeepingCentre (cell.getWidth() - 6, 26)
                                    .withY (area.getY() + (int) ((float) area.getHeight() * 0.36f)));
        x += w;
    }
}

AtmosEditor::AtmosEditor (AtmosProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      osc1 ("OSC 1", cOsc), osc2 ("OSC 2", cOsc), sub ("SUB / NOISE", cSub), mod ("MODULATION", cMod),
      filter ("FILTER", cFilter), amp ("AMP ENVELOPE", cEnv), fenv ("FILTER ENVELOPE", cEnv),
      lfo1 ("LFO 1", cLfo), lfo2 ("LFO 2", cLfo), grit ("GRIT", cGrit), out ("OUT", cOut),
      chorus ("CHORUS", cFx), delay ("DELAY", cFx), reverb ("REVERB", cFx)
{
    using namespace atm;
    setLookAndFeel (&laf);
    auto& s = p.apvts;

    osc1.addKnob (s, O1_SHAPE, "Shape"); osc1.addKnob (s, O1_OCT, "Octave"); osc1.addKnob (s, O1_FINE, "Fine");
    osc1.addKnob (s, O1_LEVEL, "Level"); osc1.addKnob (s, O1_UNI, "Voices"); osc1.addKnob (s, O1_DET, "Detune");
    osc2.addKnob (s, O2_SHAPE, "Shape"); osc2.addKnob (s, O2_OCT, "Octave"); osc2.addKnob (s, O2_FINE, "Fine");
    osc2.addKnob (s, O2_LEVEL, "Level"); osc2.addKnob (s, O2_UNI, "Voices"); osc2.addKnob (s, O2_DET, "Detune");
    sub.addKnob (s, SUB_LEVEL, "Sub"); sub.addKnob (s, SUB_OCT, "Sub oct");
    sub.addKnob (s, NOISE_LEVEL, "Noise"); sub.addKnob (s, NOISE_COLOR, "Color");

    mod.addKnob (s, RING, "Ring mod"); mod.addKnob (s, FM, "FM"); mod.addKnob (s, DRIFT, "Drift");
    mod.addKnob (s, GLIDE, "Glide"); mod.addKnob (s, SPREAD, "Spread");
    filter.addCombo (s, F_TYPE, 1.5f); filter.addKnob (s, F_CUT, "Cutoff"); filter.addKnob (s, F_RES, "Reso");
    filter.addKnob (s, F_ENV, "Env"); filter.addKnob (s, F_KEY, "Key");
    amp.addKnob (s, A_A, "Attack"); amp.addKnob (s, A_D, "Decay"); amp.addKnob (s, A_S, "Sustain"); amp.addKnob (s, A_R, "Release");
    fenv.addKnob (s, E_A, "Attack"); fenv.addKnob (s, E_D, "Decay"); fenv.addKnob (s, E_S, "Sustain"); fenv.addKnob (s, E_R, "Release");

    lfo1.addCombo (s, L1_SHAPE, 1.2f); lfo1.addCombo (s, L1_DEST, 1.2f); lfo1.addKnob (s, L1_RATE, "Rate"); lfo1.addKnob (s, L1_AMT, "Amount");
    lfo2.addCombo (s, L2_SHAPE, 1.2f); lfo2.addCombo (s, L2_DEST, 1.2f); lfo2.addKnob (s, L2_RATE, "Rate"); lfo2.addKnob (s, L2_AMT, "Amount");
    grit.addKnob (s, G_DRIVE, "Drive"); grit.addKnob (s, G_FOLD, "Fold"); grit.addKnob (s, G_BITS, "Crush");
    grit.addKnob (s, G_RATE, "Rate"); grit.addKnob (s, G_MIX, "Mix");
    out.addKnob (s, MASTER, "Volume");

    chorus.addKnob (s, C_RATE, "Rate"); chorus.addKnob (s, C_DEPTH, "Depth"); chorus.addKnob (s, C_MIX, "Mix");
    delay.addKnob (s, D_TIME, "Time"); delay.addKnob (s, D_FB, "Feedback"); delay.addKnob (s, D_TONE, "Tone"); delay.addKnob (s, D_MIX, "Mix");
    reverb.addKnob (s, R_SIZE, "Size"); reverb.addKnob (s, R_DECAY, "Decay"); reverb.addKnob (s, R_DAMP, "Damping");
    reverb.addKnob (s, R_PRE, "Pre-delay"); reverb.addKnob (s, R_SHIM, "Shimmer"); reverb.addKnob (s, R_MIX, "Mix");

    for (auto* c : { &osc1, &osc2, &sub, &mod, &filter, &amp, &fenv, &lfo1, &lfo2, &grit, &out, &chorus, &delay, &reverb })
        addAndMakeVisible (*c);

    for (int i = 0; i < (int) presets().size(); ++i)
        presetBox.addItem (presets()[(size_t) i].name, i + 1);
    presetBox.setSelectedItemIndex (p.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { processor.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    setSize (1280, 700);
}

AtmosEditor::~AtmosEditor()
{
    setLookAndFeel (nullptr);
}

void AtmosEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kText);
    g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    g.drawText ("ATMOS", 16, 10, 200, 34, juce::Justification::left);
    g.setColour (kMuted);
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawText ("Preset", getWidth() - 292, 10, 60, 34, juce::Justification::centredRight);
}

void AtmosEditor::layoutRow (juce::Rectangle<int> row, std::initializer_list<Panel*> panels)
{
    const int gap = 8;
    float sum = 0.0f;
    for (auto* p : panels) sum += p->totalWeight();
    const float avail = (float) (row.getWidth() - gap * ((int) panels.size() - 1));
    float x = (float) row.getX();
    for (auto* p : panels)
    {
        const float w = avail * p->totalWeight() / sum;
        p->setBounds (juce::Rectangle<float> (x, (float) row.getY(), w, (float) row.getHeight()).toNearestInt());
        x += w + (float) gap;
    }
}

void AtmosEditor::resized()
{
    presetBox.setBounds (getWidth() - 224, 14, 208, 28);

    auto r = getLocalBounds().reduced (16, 0);
    r.removeFromTop (56);
    r.removeFromBottom (14);
    const int gap = 8;
    const int rowH = (r.getHeight() - 3 * gap) / 4;

    layoutRow (r.removeFromTop (rowH), { &osc1, &osc2, &sub });
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), { &mod, &filter, &amp, &fenv });
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), { &lfo1, &lfo2, &grit, &out });
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), { &chorus, &delay, &reverb });
}
