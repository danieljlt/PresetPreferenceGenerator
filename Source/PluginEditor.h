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
    JX11AudioProcessor& audioProcessor;
    
    // UI Components
    juce::TabbedComponent tabbedComponent;
    juce::GenericAudioProcessorEditor genericEditor;
    juce::Component gaTab;
    juce::Label gaLabel;
    juce::TextButton gaStartStopButton;
    juce::TextButton gaPauseResumeButton;
    juce::Label fitnessLabel;
 
    juce::TextButton likeButton;
    juce::TextButton dislikeButton;
    juce::TextButton skipButton;
    
    juce::Label experimentLabel;
    juce::ComboBox experimentModeBox;
    
    juce::Label inputModeLabel;
    juce::ComboBox inputModeBox;
    
    // Helper methods
    void updateGAButtonState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JX11AudioProcessorEditor)
};
