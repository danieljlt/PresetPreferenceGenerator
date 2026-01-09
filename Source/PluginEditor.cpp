/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JX11AudioProcessorEditor::JX11AudioProcessorEditor (JX11AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      genericEditor(audioProcessor)
{
    // Set up the tabbed component
    addAndMakeVisible(tabbedComponent);
    
    // Set up the GA tab with a simple label and button
    gaLabel.setText("Genetic Algorithm", juce::dontSendNotification);
    gaLabel.setJustificationType(juce::Justification::centred);
    gaTab.addAndMakeVisible(gaLabel);
    
    gaStartStopButton.setButtonText("Start GA");
    gaStartStopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::lightblue);
    gaStartStopButton.onClick = [this]() 
    {
        if (audioProcessor.isGARunning()) 
        {
            audioProcessor.stopGA();
        } else 
        {
            audioProcessor.startGA();
        }
        updateGAButtonState();
    };
    gaTab.addAndMakeVisible(gaStartStopButton);
    
    gaPauseResumeButton.setButtonText("Pause");
    gaPauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    gaPauseResumeButton.setVisible(true);
    gaPauseResumeButton.onClick = [this]() 
    {
        if (audioProcessor.isGAPaused()) 
        {
            audioProcessor.resumeGA();
        } else 
        {
            audioProcessor.pauseGA();
        }
        updateGAButtonState();
    };
    gaTab.addAndMakeVisible(gaPauseResumeButton);
    
    // Fitness display
    fitnessLabel.setText("Fitness: --", juce::dontSendNotification);
    fitnessLabel.setJustificationType(juce::Justification::centred);
    fitnessLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    fitnessLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    fitnessLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    gaTab.addAndMakeVisible(fitnessLabel);
    
    // Candidates "Next Preset" button
    nextPresetButton.setButtonText("Next Preset");
    nextPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen);
    nextPresetButton.onClick = [this]()
    {
        if (audioProcessor.fetchNextPreset())
        {
           // Success. Also dump the queue for debugging.
           audioProcessor.debugLogQueue();
        }
        else
        {
            // Optional: Indicate no preset available (could flash red)
        }
    };
    gaTab.addAndMakeVisible(nextPresetButton);
    
    // Candidates count label
    candidatesLabel.setText("Candidates: 0", juce::dontSendNotification);
    candidatesLabel.setJustificationType(juce::Justification::centred);
    gaTab.addAndMakeVisible(candidatesLabel);
    
    // Debug parameters label
    debugParamsLabel.setText("Params: --", juce::dontSendNotification);
    debugParamsLabel.setJustificationType(juce::Justification::centredTop);
    // Debug dump button removed (integrated into Next Preset)
    
    // Add tabs
    tabbedComponent.addTab("Synth Controls", juce::Colours::lightgrey, &genericEditor, false);
    tabbedComponent.addTab("Genetic Algorithm", juce::Colours::lightgrey, &gaTab, false);
    
    // Sync button state with GA state
    updateGAButtonState();
    
    // Start timer to update button states periodically (30 FPS)
    startTimer(33);
    
    // Set initial size
    setSize (800, 600);
}

JX11AudioProcessorEditor::~JX11AudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void JX11AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void JX11AudioProcessorEditor::resized()
{
    tabbedComponent.setBounds(getLocalBounds());
    
    // Layout the GA tab content
    auto gaBounds = gaTab.getLocalBounds().reduced(20, 10);
    
    gaLabel.setBounds(gaBounds.removeFromTop(40));
    gaBounds.removeFromTop(10); // Spacing
    
    // Layout GA control buttons in a row
    auto buttonArea = gaBounds.removeFromTop(40);
    auto buttonWidth = buttonArea.getWidth() / 2;
    
    auto startStopBounds = buttonArea.removeFromLeft(buttonWidth).reduced(5, 0);
    auto pauseResumeBounds = buttonArea.removeFromLeft(buttonWidth).reduced(5, 0);
    
    gaStartStopButton.setBounds(startStopBounds);
    gaPauseResumeButton.setBounds(pauseResumeBounds);
    
    // Fitness label
    gaBounds.removeFromTop(20); // Spacing
    fitnessLabel.setBounds(gaBounds.removeFromTop(30));
    
    // Candidates area
    gaBounds.removeFromTop(20); // Spacing
    candidatesLabel.setBounds(gaBounds.removeFromTop(25));
    nextPresetButton.setBounds(gaBounds.removeFromTop(40).reduced(20, 0));
    
    // Debug params area
    gaBounds.removeFromTop(10);
    debugParamsLabel.setBounds(gaBounds.removeFromTop(100));
}

void JX11AudioProcessorEditor::timerCallback()
{
    updateGAButtonState();

    // Update fitness display
    float fitness = audioProcessor.getLastGAFitness();
    fitnessLabel.setText("Fitness: " + juce::String(fitness, 4), juce::dontSendNotification);
    
    // Update candidates count
    int candidates = audioProcessor.getNumCandidatesAvailable();
    candidatesLabel.setText("Candidates: " + juce::String(candidates), juce::dontSendNotification);
    
    // Enable "Next" button only if candidates are available
    nextPresetButton.setEnabled(candidates > 0);
    
    // Update debug parameters display
    const auto& params = audioProcessor.getCurrentGAParameters();
    if (!params.empty())
    {
        juce::String s;
        for (size_t i = 0; i < params.size(); ++i)
        {
            s << juce::String(params[i], 2) << " ";
            if ((i + 1) % 6 == 0) s << "\n"; // Newline every 6 params
        }
        debugParamsLabel.setText(s, juce::dontSendNotification);
    }
}

void JX11AudioProcessorEditor::logMessage(const juce::String&)
{
    // Empty for now
}

void JX11AudioProcessorEditor::updateGAButtonState()
{
    bool isRunning = audioProcessor.isGARunning();
    bool isPaused = audioProcessor.isGAPaused();
    // Note: Start/Stop button logic simplified (always enabled if idle)
    if (isRunning) 
    {
        gaStartStopButton.setButtonText("Stop GA");
        gaStartStopButton.setEnabled(true);
    } 
    else 
    {
        gaStartStopButton.setButtonText("Start GA");
        gaStartStopButton.setEnabled(true);
    }
    
    // Pause/Resume toggle button - only enabled when running
    gaPauseResumeButton.setEnabled(isRunning);
    
    // Update button text and color based on current state
    if (isPaused) 
    {
        gaPauseResumeButton.setButtonText("Resume");
        gaPauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    } else 
    {
        gaPauseResumeButton.setButtonText("Pause");
        gaPauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    }
}
