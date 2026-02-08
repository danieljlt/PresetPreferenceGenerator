#include "GAControlsPanel.h"
#include "PPGLookAndFeel.h"

GAControlsPanel::GAControlsPanel(JX11AudioProcessor& processor)
    : audioProcessor(processor)
{
    // Start/Stop button
    startStopButton.setButtonText("Start GA");
    startStopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kAccent));
    startStopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    startStopButton.onClick = [this]()
    {
        if (audioProcessor.isGARunning())
            audioProcessor.stopGA();
        else
            audioProcessor.startGA();
        updateButtonState();
    };
    addAndMakeVisible(startStopButton);

    // Pause/Resume button
    pauseResumeButton.setButtonText("Pause");
    pauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kNeutral));
    pauseResumeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pauseResumeButton.onClick = [this]()
    {
        if (audioProcessor.isGAPaused())
            audioProcessor.resumeGA();
        else
            audioProcessor.pauseGA();
        updateButtonState();
    };
    addAndMakeVisible(pauseResumeButton);

    // Like button
    likeButton.setButtonText("Like");
    likeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kPositive));
    likeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    likeButton.onClick = [this]()
    {
        audioProcessor.logFeedback({ 1.0f, audioProcessor.getPlayTimeSeconds() });
        audioProcessor.fetchNextPreset();
    };
    addAndMakeVisible(likeButton);

    // Dislike button
    dislikeButton.setButtonText("Dislike");
    dislikeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kNegative));
    dislikeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    dislikeButton.onClick = [this]()
    {
        audioProcessor.logFeedback({ 0.0f, audioProcessor.getPlayTimeSeconds() });
        audioProcessor.fetchNextPreset();
    };
    addAndMakeVisible(dislikeButton);

    // Skip button
    skipButton.setButtonText("Skip");
    skipButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kNeutral));
    skipButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    skipButton.onClick = [this]()
    {
        audioProcessor.fetchNextPreset();
    };
    addAndMakeVisible(skipButton);

    // Experiment mode
    experimentLabel.setText("Experiment", juce::dontSendNotification);
    experimentLabel.setJustificationType(juce::Justification::centredRight);
    experimentLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    addAndMakeVisible(experimentLabel);

    experimentModeBox.addItem("Baseline", 1);
    experimentModeBox.addItem("Adaptive Only", 2);
    experimentModeBox.addItem("Novelty Only", 3);
    experimentModeBox.addItem("All Enhancements", 4);
    experimentModeBox.setSelectedId(static_cast<int>(audioProcessor.getExperimentMode()) + 1,
                                    juce::dontSendNotification);
    experimentModeBox.onChange = [this]()
    {
        int id = experimentModeBox.getSelectedId();
        audioProcessor.setExperimentMode(static_cast<ExperimentMode>(id - 1));
    };
    addAndMakeVisible(experimentModeBox);

    // Input mode
    inputModeLabel.setText("Input Mode", juce::dontSendNotification);
    inputModeLabel.setJustificationType(juce::Justification::centredRight);
    inputModeLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    addAndMakeVisible(inputModeLabel);

    inputModeBox.addItem("Genome", 1);
    inputModeBox.addItem("Audio", 2);
    inputModeBox.setSelectedId(static_cast<int>(audioProcessor.getInputMode()) + 1,
                               juce::dontSendNotification);
    inputModeBox.onChange = [this]()
    {
        int id = inputModeBox.getSelectedId();
        audioProcessor.setInputMode(static_cast<GAConfig::MLPInputMode>(id - 1));
    };
    addAndMakeVisible(inputModeBox);

    updateButtonState();
    startTimer(33); // 30fps
}

GAControlsPanel::~GAControlsPanel()
{
    stopTimer();
}

//==============================================================================
void GAControlsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(PPGLookAndFeel::kBackground));

    paintCard(g, controlsCardBounds, "CONTROLS");
    paintCard(g, feedbackCardBounds, "FEEDBACK");
    paintCard(g, configCardBounds, "CONFIGURATION");
}

void GAControlsPanel::paintCard(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                  const juce::String& title)
{
    if (bounds.isEmpty())
        return;

    auto r = bounds.toFloat();

    g.setColour(juce::Colour(PPGLookAndFeel::kSurface));
    g.fillRoundedRectangle(r, 12.0f);

    g.setColour(juce::Colour(PPGLookAndFeel::kSurfaceBorder));
    g.drawRoundedRectangle(r.reduced(0.5f), 12.0f, 1.0f);

    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(juce::Colour(PPGLookAndFeel::kGroupHeader));
    g.drawText(title, bounds.getX() + 14, bounds.getY() + 10, bounds.getWidth() - 28, 14,
               juce::Justification::centredLeft, false);
}

//==============================================================================
void GAControlsPanel::resized()
{
    auto area = getLocalBounds().reduced(20);

    int cardGap = 12;
    int numGaps = 2;
    int availableHeight = area.getHeight() - numGaps * cardGap;

    // Proportional card heights: Controls ~28%, Feedback ~40%, Config ~32%
    int controlsH = (int)(availableHeight * 0.28f);
    int feedbackH = (int)(availableHeight * 0.40f);
    auto content = area;

    controlsCardBounds = content.removeFromTop(controlsH);
    content.removeFromTop(cardGap);
    feedbackCardBounds = content.removeFromTop(feedbackH);
    content.removeFromTop(cardGap);
    configCardBounds = content;

    // Layout inside Controls card
    {
        auto inner = controlsCardBounds.reduced(20, 0);
        inner.removeFromTop(30);
        auto row = inner.reduced(0, 6);
        int half = row.getWidth() / 2;
        startStopButton.setBounds(row.removeFromLeft(half).reduced(6, 0));
        pauseResumeButton.setBounds(row.reduced(6, 0));
    }

    // Layout inside Feedback card
    {
        auto inner = feedbackCardBounds.reduced(20, 0);
        inner.removeFromTop(30);
        auto row = inner.reduced(0, 8);
        int third = row.getWidth() / 3;
        dislikeButton.setBounds(row.removeFromLeft(third).reduced(6, 0));
        skipButton.setBounds(row.removeFromLeft(third).reduced(6, 0));
        likeButton.setBounds(row.reduced(6, 0));
    }

    // Layout inside Configuration card
    {
        auto inner = configCardBounds.reduced(20, 0);
        inner.removeFromTop(30);
        int rowH = juce::jmin(36, (inner.getHeight() - 10) / 2);

        auto row1 = inner.removeFromTop(rowH);
        experimentLabel.setBounds(row1.removeFromLeft(100));
        experimentModeBox.setBounds(row1.reduced(6, 4));

        inner.removeFromTop(10);

        auto row2 = inner.removeFromTop(rowH);
        inputModeLabel.setBounds(row2.removeFromLeft(100));
        inputModeBox.setBounds(row2.reduced(6, 4));
    }
}

//==============================================================================
void GAControlsPanel::timerCallback()
{
    updateButtonState();

    bool hasPreset = audioProcessor.getNumCandidatesAvailable() > 0;
    likeButton.setEnabled(hasPreset);
    dislikeButton.setEnabled(hasPreset);
    skipButton.setEnabled(hasPreset);
}

void GAControlsPanel::updateButtonState()
{
    bool isRunning = audioProcessor.isGARunning();
    bool isPaused = audioProcessor.isGAPaused();

    if (isRunning)
    {
        startStopButton.setButtonText("Stop GA");
        startStopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kNegative));
    }
    else
    {
        startStopButton.setButtonText("Start GA");
        startStopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kAccent));
    }

    pauseResumeButton.setEnabled(isRunning);

    if (isPaused)
    {
        pauseResumeButton.setButtonText("Resume");
        pauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kPositive));
    }
    else
    {
        pauseResumeButton.setButtonText("Pause");
        pauseResumeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(PPGLookAndFeel::kNeutral));
    }
}
