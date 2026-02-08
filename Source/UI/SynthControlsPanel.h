#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class SynthControlsPanel : public juce::Component
{
public:
    explicit SynthControlsPanel(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct ParamControl
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct ChoiceControl
    {
        std::unique_ptr<juce::ComboBox> comboBox;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct CardGroup
    {
        juce::String title;
        std::vector<ParamControl*> knobs;
        std::vector<ChoiceControl*> combos;
        juce::Rectangle<int> bounds;
    };

    // Parameter controls
    ParamControl oscMix, oscTune, oscFine;
    ParamControl filterFreq, filterReso, filterEnv, filterLFO, filterVelocity;
    ParamControl filterAttack, filterDecay, filterSustain, filterRelease;
    ParamControl glideRate, glideBend;
    ParamControl envAttack, envDecay, envSustain, envRelease;
    ParamControl lfoRate, vibrato;
    ParamControl noise, octave, tuning, outputLevel;

    // Choice controls
    ChoiceControl glideMode, polyMode;

    // Card groups
    CardGroup oscillatorCard, filterCard, filterEnvCard;
    CardGroup glideCard, ampEnvCard, lfoCard;
    CardGroup outputCard;

    void initKnob(ParamControl& ctrl, juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId, const juce::String& labelText);
    void initCombo(ChoiceControl& ctrl, juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId, const juce::String& labelText);

    void paintCard(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& title);
    void layoutCard(const juce::Rectangle<int>& bounds, const std::vector<ParamControl*>& knobs,
                    const std::vector<ChoiceControl*>& combos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthControlsPanel)
};
