#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

class GAControlsPanel : public juce::Component,
                        public juce::Timer
{
public:
    explicit GAControlsPanel(JX11AudioProcessor& processor);
    ~GAControlsPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    JX11AudioProcessor& audioProcessor;

    // Controls card
    juce::TextButton startStopButton;
    juce::TextButton pauseResumeButton;

    // Feedback card
    juce::TextButton likeButton;
    juce::TextButton dislikeButton;
    juce::TextButton skipButton;

    // Configuration card
    juce::Label experimentLabel;
    juce::ComboBox experimentModeBox;
    juce::Label inputModeLabel;
    juce::ComboBox inputModeBox;

    // Card bounds for painting
    juce::Rectangle<int> controlsCardBounds;
    juce::Rectangle<int> feedbackCardBounds;
    juce::Rectangle<int> configCardBounds;

    void updateButtonState();
    void paintCard(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& title);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GAControlsPanel)
};
