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
    
    // Fitness display (hidden to avoid biasing user ratings)
    fitnessLabel.setText("Fitness: --", juce::dontSendNotification);
    fitnessLabel.setJustificationType(juce::Justification::centred);
    fitnessLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    fitnessLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    fitnessLabel.setVisible(false);
    gaTab.addAndMakeVisible(fitnessLabel);
    
    // Like Button
    likeButton.setButtonText("Like");
    likeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen);
    likeButton.onClick = [this]()
    {
        audioProcessor.logFeedback({1.0f, audioProcessor.getPlayTimeSeconds()});
        audioProcessor.fetchNextPreset();
    };
    gaTab.addAndMakeVisible(likeButton);

    // Dislike Button
    dislikeButton.setButtonText("Dislike");
    dislikeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    dislikeButton.onClick = [this]()
    {
        audioProcessor.logFeedback({0.0f, audioProcessor.getPlayTimeSeconds()});
        audioProcessor.fetchNextPreset();
    };
    gaTab.addAndMakeVisible(dislikeButton);
    
    // Skip Button
    skipButton.setButtonText("Skip");
    skipButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    skipButton.onClick = [this]()
    {
        audioProcessor.fetchNextPreset();
    };
    gaTab.addAndMakeVisible(skipButton);
    
    // Experiment mode selector
    experimentLabel.setText("Experiment Mode:", juce::dontSendNotification);
    gaTab.addAndMakeVisible(experimentLabel);
    
    experimentModeBox.addItem("Baseline", 1);
    experimentModeBox.addItem("Adaptive Only", 2);
    experimentModeBox.addItem("Novelty Only", 3);
    experimentModeBox.addItem("All Enhancements", 4);
    experimentModeBox.setSelectedId(static_cast<int>(audioProcessor.getExperimentMode()) + 1, juce::dontSendNotification);
    experimentModeBox.onChange = [this]()
    {
        int id = experimentModeBox.getSelectedId();
        audioProcessor.setExperimentMode(static_cast<ExperimentMode>(id - 1));
    };
    gaTab.addAndMakeVisible(experimentModeBox);
    
    // Input mode selector (Genome vs Audio features)
    inputModeLabel.setText("Input Mode:", juce::dontSendNotification);
    gaTab.addAndMakeVisible(inputModeLabel);
    
    inputModeBox.addItem("Genome", 1);
    inputModeBox.addItem("Audio", 2);
    inputModeBox.setSelectedId(static_cast<int>(audioProcessor.getInputMode()) + 1, juce::dontSendNotification);
    inputModeBox.onChange = [this]()
    {
        int id = inputModeBox.getSelectedId();
        audioProcessor.setInputMode(static_cast<GAConfig::MLPInputMode>(id - 1));
    };
    gaTab.addAndMakeVisible(inputModeBox);
    
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
    
    // Fitness label hidden - skip layout
    gaBounds.removeFromTop(20); // Spacing
    
    // Feedback buttons
    gaBounds.removeFromTop(20); // Spacing
    auto feedbackArea = gaBounds.removeFromTop(40).reduced(20, 0);
    int feedbackButtonWidth = feedbackArea.getWidth() / 3;
    dislikeButton.setBounds(feedbackArea.removeFromLeft(feedbackButtonWidth).reduced(5, 0));
    skipButton.setBounds(feedbackArea.removeFromLeft(feedbackButtonWidth).reduced(5, 0));
    likeButton.setBounds(feedbackArea.removeFromLeft(feedbackButtonWidth).reduced(5, 0));
    
    // Experiment mode selector
    gaBounds.removeFromTop(30);
    auto modeArea = gaBounds.removeFromTop(30);
    experimentLabel.setBounds(modeArea.removeFromLeft(150));
    experimentModeBox.setBounds(modeArea.reduced(5, 0));
    
    // Input mode selector
    gaBounds.removeFromTop(10);
    auto inputModeArea = gaBounds.removeFromTop(30);
    inputModeLabel.setBounds(inputModeArea.removeFromLeft(150));
    inputModeBox.setBounds(inputModeArea.reduced(5, 0));
}

void JX11AudioProcessorEditor::timerCallback()
{
    updateGAButtonState();

    // Update fitness display
    float fitness = audioProcessor.getLastGAFitness();
    fitnessLabel.setText("Fitness: " + juce::String(fitness, 4), juce::dontSendNotification);
    
    // Enable Feedback buttons only when GA is running
    bool hasPreset = audioProcessor.getNumCandidatesAvailable() > 0;
    likeButton.setEnabled(hasPreset);
    dislikeButton.setEnabled(hasPreset);
    skipButton.setEnabled(hasPreset);
}

void JX11AudioProcessorEditor::updateGAButtonState()
{
    bool isRunning = audioProcessor.isGARunning();
    bool isPaused = audioProcessor.isGAPaused();
    
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
