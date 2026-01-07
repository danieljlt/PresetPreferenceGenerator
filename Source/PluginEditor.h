/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class JX11AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   public juce::Timer
{
public:
    JX11AudioProcessorEditor (JX11AudioProcessor&);
    ~JX11AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    JX11AudioProcessor& audioProcessor;
    
    // UI Components
    juce::TabbedComponent tabbedComponent;
    juce::GenericAudioProcessorEditor genericEditor;
    juce::Component gaTab;
    juce::Label gaLabel;
    // Target UI removed
    juce::TextButton gaStartStopButton;
    juce::TextButton gaPauseResumeButton;
    juce::Label fitnessLabel;
    
    // File chooser removed
    
    // Helper methods
    void logMessage(const juce::String& message);
    void updateGAButtonState();
    // updateTargetLabel removed

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JX11AudioProcessorEditor)
};
