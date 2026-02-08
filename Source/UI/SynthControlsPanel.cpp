#include "SynthControlsPanel.h"
#include "PPGLookAndFeel.h"

SynthControlsPanel::SynthControlsPanel(juce::AudioProcessorValueTreeState& apvts)
{
    // Oscillator
    initKnob(oscMix,  apvts, "oscMix",  "Mix");
    initKnob(oscTune, apvts, "oscTune", "Tune");
    initKnob(oscFine, apvts, "oscFine", "Fine");

    // Filter
    initKnob(filterFreq,     apvts, "filterFreq",     "Freq");
    initKnob(filterReso,     apvts, "filterReso",     "Reso");
    initKnob(filterEnv,      apvts, "filterEnv",      "Env");
    initKnob(filterLFO,      apvts, "filterLFO",      "LFO");
    initKnob(filterVelocity, apvts, "filterVelocity", "Vel");

    // Filter Envelope
    initKnob(filterAttack,  apvts, "filterAttack",  "A");
    initKnob(filterDecay,   apvts, "filterDecay",   "D");
    initKnob(filterSustain, apvts, "filterSustain", "S");
    initKnob(filterRelease, apvts, "filterRelease", "R");

    // Glide
    initCombo(glideMode, apvts, "glideMode", "Mode");
    initKnob(glideRate,  apvts, "glideRate",  "Rate");
    initKnob(glideBend,  apvts, "glideBend",  "Bend");

    // Amp Envelope
    initKnob(envAttack,  apvts, "envAttack",  "A");
    initKnob(envDecay,   apvts, "envDecay",   "D");
    initKnob(envSustain, apvts, "envSustain", "S");
    initKnob(envRelease, apvts, "envRelease", "R");

    // LFO
    initKnob(lfoRate, apvts, "lfoRate", "Rate");
    initKnob(vibrato, apvts, "vibrato", "Vibrato");

    // Output
    initKnob(noise,       apvts, "noise",       "Noise");
    initKnob(octave,      apvts, "octave",      "Octave");
    initKnob(tuning,      apvts, "tuning",      "Tuning");
    initKnob(outputLevel, apvts, "outputLevel", "Level");
    initCombo(polyMode,   apvts, "polyMode",    "Poly");

    // Set up card groups
    oscillatorCard = { "OSCILLATOR", { &oscMix, &oscTune, &oscFine }, {}, {} };
    filterCard     = { "FILTER",     { &filterFreq, &filterReso, &filterEnv, &filterLFO, &filterVelocity }, {}, {} };
    filterEnvCard  = { "FILTER ENVELOPE", { &filterAttack, &filterDecay, &filterSustain, &filterRelease }, {}, {} };
    glideCard      = { "GLIDE",      { &glideRate, &glideBend }, { &glideMode }, {} };
    ampEnvCard     = { "AMP ENVELOPE", { &envAttack, &envDecay, &envSustain, &envRelease }, {}, {} };
    lfoCard        = { "LFO",        { &lfoRate, &vibrato }, {}, {} };
    outputCard     = { "OUTPUT",     { &noise, &octave, &tuning, &outputLevel }, { &polyMode }, {} };
}

void SynthControlsPanel::initKnob(ParamControl& ctrl, juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramId, const juce::String& labelText)
{
    ctrl.slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                  juce::Slider::TextBoxBelow);
    ctrl.slider->setTextBoxIsEditable(true);
    ctrl.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 14);
    addAndMakeVisible(ctrl.slider.get());

    ctrl.label = std::make_unique<juce::Label>(paramId + "_label", labelText);
    ctrl.label->setJustificationType(juce::Justification::centred);
    ctrl.label->setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    ctrl.label->setColour(juce::Label::textColourId, juce::Colour(PPGLookAndFeel::kTextSecondary));
    addAndMakeVisible(ctrl.label.get());

    ctrl.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramId, *ctrl.slider);
}

void SynthControlsPanel::initCombo(ChoiceControl& ctrl, juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& paramId, const juce::String& labelText)
{
    ctrl.comboBox = std::make_unique<juce::ComboBox>(paramId + "_combo");
    ctrl.comboBox->setJustificationType(juce::Justification::centred);

    // Populate from parameter choices
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId)))
    {
        auto choices = param->choices;
        for (int i = 0; i < choices.size(); ++i)
            ctrl.comboBox->addItem(choices[i], i + 1);
    }

    addAndMakeVisible(ctrl.comboBox.get());

    ctrl.label = std::make_unique<juce::Label>(paramId + "_label", labelText);
    ctrl.label->setJustificationType(juce::Justification::centred);
    ctrl.label->setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    ctrl.label->setColour(juce::Label::textColourId, juce::Colour(PPGLookAndFeel::kTextSecondary));
    addAndMakeVisible(ctrl.label.get());

    ctrl.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, paramId, *ctrl.comboBox);
}

//==============================================================================
void SynthControlsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(PPGLookAndFeel::kBackground));

    // Paint each card
    paintCard(g, oscillatorCard.bounds, oscillatorCard.title);
    paintCard(g, filterCard.bounds, filterCard.title);
    paintCard(g, filterEnvCard.bounds, filterEnvCard.title);
    paintCard(g, glideCard.bounds, glideCard.title);
    paintCard(g, ampEnvCard.bounds, ampEnvCard.title);
    paintCard(g, lfoCard.bounds, lfoCard.title);
    paintCard(g, outputCard.bounds, outputCard.title);
}

void SynthControlsPanel::paintCard(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                     const juce::String& title)
{
    if (bounds.isEmpty())
        return;

    auto r = bounds.toFloat();

    // Card background
    g.setColour(juce::Colour(PPGLookAndFeel::kSurface));
    g.fillRoundedRectangle(r, 12.0f);

    // Card border
    g.setColour(juce::Colour(PPGLookAndFeel::kSurfaceBorder));
    g.drawRoundedRectangle(r.reduced(0.5f), 12.0f, 1.0f);

    // Title
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(juce::Colour(PPGLookAndFeel::kGroupHeader));
    g.drawText(title, bounds.getX() + 14, bounds.getY() + 8, bounds.getWidth() - 28, 14,
               juce::Justification::centredLeft, false);
}

//==============================================================================
void SynthControlsPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    auto cardGap = 10;
    auto topRowHeight = (area.getHeight() - cardGap * 2 - 10) / 3;  // 3 rows + bottom row
    auto colWidth = (area.getWidth() - cardGap * 2) / 3;

    // Row 1
    auto row1 = area.removeFromTop(topRowHeight);
    oscillatorCard.bounds = row1.removeFromLeft(colWidth).reduced(cardGap / 2);
    filterCard.bounds = row1.removeFromLeft(colWidth).reduced(cardGap / 2);
    filterEnvCard.bounds = row1.reduced(cardGap / 2);

    area.removeFromTop(cardGap);

    // Row 2
    auto row2 = area.removeFromTop(topRowHeight);
    glideCard.bounds = row2.removeFromLeft(colWidth).reduced(cardGap / 2);
    ampEnvCard.bounds = row2.removeFromLeft(colWidth).reduced(cardGap / 2);
    lfoCard.bounds = row2.reduced(cardGap / 2);

    area.removeFromTop(cardGap);

    // Row 3 (full width)
    outputCard.bounds = area.reduced(cardGap / 2);

    // Layout controls inside each card
    layoutCard(oscillatorCard.bounds, oscillatorCard.knobs, oscillatorCard.combos);
    layoutCard(filterCard.bounds, filterCard.knobs, filterCard.combos);
    layoutCard(filterEnvCard.bounds, filterEnvCard.knobs, filterEnvCard.combos);
    layoutCard(glideCard.bounds, glideCard.knobs, glideCard.combos);
    layoutCard(ampEnvCard.bounds, ampEnvCard.knobs, ampEnvCard.combos);
    layoutCard(lfoCard.bounds, lfoCard.knobs, lfoCard.combos);
    layoutCard(outputCard.bounds, outputCard.knobs, outputCard.combos);
}

void SynthControlsPanel::layoutCard(const juce::Rectangle<int>& bounds,
                                      const std::vector<ParamControl*>& knobs,
                                      const std::vector<ChoiceControl*>& combos)
{
    if (bounds.isEmpty())
        return;

    auto contentArea = bounds.reduced(10, 0);
    contentArea.removeFromTop(26); // header space

    int totalControls = (int)knobs.size() + (int)combos.size();
    if (totalControls == 0)
        return;

    int controlWidth = contentArea.getWidth() / totalControls;
    int knobSize = juce::jmin(60, controlWidth - 4);
    int labelHeight = 14;
    int comboHeight = 24;

    auto controlsRow = contentArea;

    // Layout combos first (left side)
    for (auto* combo : combos)
    {
        auto slot = controlsRow.removeFromLeft(controlWidth);
        auto labelArea = slot.removeFromTop(labelHeight);
        combo->label->setBounds(labelArea);

        auto comboArea = slot.withSizeKeepingCentre(juce::jmin(controlWidth - 8, 90), comboHeight);
        comboArea.setY(labelArea.getBottom() + 8);
        combo->comboBox->setBounds(comboArea);
    }

    // Layout knobs
    for (auto* knob : knobs)
    {
        auto slot = controlsRow.removeFromLeft(controlWidth);
        auto labelArea = slot.removeFromTop(labelHeight);
        knob->label->setBounds(labelArea);

        auto knobArea = slot.withSizeKeepingCentre(knobSize, knobSize + 16); // +16 for text box
        knobArea.setY(labelArea.getBottom() + 2);
        knob->slider->setBounds(knobArea);
    }
}
