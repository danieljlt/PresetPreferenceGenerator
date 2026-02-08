#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/PPGLookAndFeel.h"
#include "UI/SynthControlsPanel.h"
#include "UI/GAControlsPanel.h"

class JX11AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    JX11AudioProcessorEditor(JX11AudioProcessor&);
    ~JX11AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    JX11AudioProcessor& audioProcessor;

    PPGLookAndFeel lookAndFeel;
    juce::TabbedComponent tabbedComponent;
    SynthControlsPanel synthPanel;
    GAControlsPanel gaPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JX11AudioProcessorEditor)
};
