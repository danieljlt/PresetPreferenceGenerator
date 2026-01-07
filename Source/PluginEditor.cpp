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
    
    // Target file label
    targetFileLabel.setText("No target loaded", juce::dontSendNotification);
    targetFileLabel.setJustificationType(juce::Justification::centred);
    targetFileLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    targetFileLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    gaTab.addAndMakeVisible(targetFileLabel);
    
    // Load target button
    loadTargetButton.setButtonText("Load Target Audio");
    loadTargetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    loadTargetButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select target audio file",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg;*.m4a");
        
        auto flags = juce::FileBrowserComponent::openMode 
                   | juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            
            if (file == juce::File{})
                return;
            
            if (!file.existsAsFile())
            {
                targetFileLabel.setText("Error: File not found", juce::dontSendNotification);
                return;
            }
            
            bool success = audioProcessor.loadTargetAudio(file);
            
            if (success)
            {
                updateTargetLabel();
                updateGAButtonState();
            }
            else
            {
                targetFileLabel.setText("Error: Could not load file", juce::dontSendNotification);
            }
        });
    };
    gaTab.addAndMakeVisible(loadTargetButton);
    
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
    gaTab.addAndMakeVisible(fitnessLabel);
    
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
    
    // Target file label
    targetFileLabel.setBounds(gaBounds.removeFromTop(30));
    gaBounds.removeFromTop(10); // Spacing
    
    // Load target button
    loadTargetButton.setBounds(gaBounds.removeFromTop(40).reduced(100, 0));
    gaBounds.removeFromTop(20); // Spacing
    
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
}

void JX11AudioProcessorEditor::timerCallback()
{
    updateGAButtonState();
    
    // Update fitness display
    if (audioProcessor.isGARunning())
    {
        float fitness = audioProcessor.getLastGAFitness();
        fitnessLabel.setText("Fitness: " + juce::String(fitness, 4), juce::dontSendNotification);
    }
    else
    {
        fitnessLabel.setText("Fitness: --", juce::dontSendNotification);
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
    bool hasTarget = audioProcessor.hasTargetAudio();
    
    // Start/Stop button - only enabled if target is loaded
    if (isRunning) 
    {
        gaStartStopButton.setButtonText("Stop GA");
        gaStartStopButton.setEnabled(true);
    } 
    else 
    {
        gaStartStopButton.setButtonText(hasTarget ? "Start GA" : "Start GA (Load Target First)");
        gaStartStopButton.setEnabled(hasTarget);
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

void JX11AudioProcessorEditor::updateTargetLabel()
{
    if (audioProcessor.hasTargetAudio())
    {
        juce::String filename = audioProcessor.getTargetFileName();
        targetFileLabel.setText("Target: " + filename, juce::dontSendNotification);
        targetFileLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgreen);
    }
    else
    {
        targetFileLabel.setText("No target loaded", juce::dontSendNotification);
        targetFileLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    }
}
