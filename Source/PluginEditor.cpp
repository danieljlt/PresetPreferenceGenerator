#include "PluginProcessor.h"
#include "PluginEditor.h"

JX11AudioProcessorEditor::JX11AudioProcessorEditor(JX11AudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      synthPanel(p.apvts),
      gaPanel(p)
{
    setLookAndFeel(&lookAndFeel);

    tabbedComponent.setTabBarDepth(36);
    tabbedComponent.addTab("Synth", juce::Colour(PPGLookAndFeel::kBackground), &synthPanel, false);
    tabbedComponent.addTab("Evolution", juce::Colour(PPGLookAndFeel::kBackground), &gaPanel, false);

    addAndMakeVisible(tabbedComponent);
    setSize(800, 600);
}

JX11AudioProcessorEditor::~JX11AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void JX11AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(PPGLookAndFeel::kBackground));
}

void JX11AudioProcessorEditor::resized()
{
    tabbedComponent.setBounds(getLocalBounds());
}
