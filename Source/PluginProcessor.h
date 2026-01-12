/*
  ==============================================================================

    This file contains the main plugin processor class for the JX11 synthesizer.
    It handles parameter management, audio processing, MIDI input, and state saving.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "JX11/Synth.h"
#include "JX11/Preset.h"
#include "GA/GeneticAlgorithm.h"
#include "GA/IFitnessModel.h"

// Namespace containing string identifiers for all plugin parameters,
// each with a version number (used for state compatibility).
namespace ParameterID
{
#define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

// Oscillator parameters
PARAMETER_ID(oscMix)
PARAMETER_ID(oscTune)
PARAMETER_ID(oscFine)

// Glide/portamento controls
PARAMETER_ID(glideMode)
PARAMETER_ID(glideRate)
PARAMETER_ID(glideBend)

// Filter cutoff, resonance, and modulation
PARAMETER_ID(filterFreq)
PARAMETER_ID(filterReso)
PARAMETER_ID(filterEnv)
PARAMETER_ID(filterLFO)
PARAMETER_ID(filterVelocity)

// Filter envelope (ADSR)
PARAMETER_ID(filterAttack)
PARAMETER_ID(filterDecay)
PARAMETER_ID(filterSustain)
PARAMETER_ID(filterRelease)

// Amplitude envelope (ADSR)
PARAMETER_ID(envAttack)
PARAMETER_ID(envDecay)
PARAMETER_ID(envSustain)
PARAMETER_ID(envRelease)

// Modulation sources
PARAMETER_ID(lfoRate)
PARAMETER_ID(vibrato)

// Misc controls
PARAMETER_ID(noise)
PARAMETER_ID(octave)
PARAMETER_ID(tuning)
PARAMETER_ID(outputLevel)
PARAMETER_ID(polyMode)

#undef PARAMETER_ID
}

//==============================================================================
// Main plugin processor class for JX11
class JX11AudioProcessor  : public juce::AudioProcessor,
                            private juce::ValueTree::Listener,
                            private juce::Timer
{
public:
    JX11AudioProcessor();
    ~JX11AudioProcessor() override;

    // Called before playback starts. Sets up synth voices and sample rate.
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    // Called when playback stops. Frees any temporary resources.
    void releaseResources() override;

    // Called when transport is rewound or reset.
    void reset() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    // Validates supported channel configurations (e.g., mono/stereo)
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    // Main processing function: handles MIDI + audio rendering
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    // UI functions
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    // Plugin metadata
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    // Program/preset management
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    // State persistence (e.g., for saving in DAW sessions)
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Holds and manages all plugin parameters with automation support
    juce::AudioProcessorValueTreeState apvts {
        *this, nullptr, "Parameters", createParameterLayout()
    };
    
    // GA control methods (centralizes GA operations)
    void startGA();
    void stopGA();
    void pauseGA();
    void resumeGA();
    bool isGARunning() const;
    bool isGAPaused() const;
    
    // GA fitness display
    float getLastGAFitness() const { return lastGAFitness; }

    // Manual preset fetching (called from UI)
    bool fetchNextPreset();
    int getNumCandidatesAvailable() const;
    
    // Debug: Get current smoothed parameters
    const std::vector<float>& getCurrentGAParameters() const { return targetParameters; }
    
    // Debug: Print queue to console
    void debugLogQueue();
    
    // Feedback mechanism
    void logFeedback(const IFitnessModel::Feedback& feedback);

private:
    // Timer callback - polls parameter bridge and applies smoothed updates
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JX11AudioProcessor)
    
    // Fitness model (owned by Processor)
    std::unique_ptr<IFitnessModel> fitnessModel;

    // Splits the audio buffer based on incoming MIDI events
    void splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Handles raw MIDI events (used for pitch bend, CC, etc.)
    void handleMIDI(uint8_t data0, uint8_t data1, uint8_t data2);

    // Renders audio for a given buffer range using active voices
    void render(juce::AudioBuffer<float>& buffer, int sampleCount, int bufferOffset);

    // Called when a parameter value changes in the APVTS
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        parametersChanged.store(true); // Flag for deferred DSP update
    }

    std::atomic<bool> parametersChanged { false }; // Used to trigger synth update on param change

    // Recalculates synth internals after parameters have changed
    void update();

    // Initializes the list of factory presets
    void createPrograms();
    std::vector<Preset> presets;    // All built-in presets
    int currentProgram;             // Currently selected preset index

    // Defines & returns the full list of parameter definitions for the plugin
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    Synth synth; // The core synthesizer engine
    std::unique_ptr<GeneticAlgorithm> gaEngine; // The genetic algorithm engine
    
    // Parameter smoothing for GA updates
    std::vector<float> targetParameters;   // Target from GA (18 floats, normalized [0,1])
    std::vector<float> currentParameters;  // Current smoothed values
    bool isInterpolating = false;
    float lastGAFitness = 0.0f;            // Most recent fitness from GA
    static constexpr float parameterSmoothingTime = 0.4f;  // 400ms to reach target
    static constexpr float timerInterval = 0.05f;          // 50ms timer rate

    // Apply interpolated parameters to synth
    void interpolateAndApplyParameters();

    // Pointers to all individual plugin parameters (linked to APVTS)
    // These are read during audio rendering or used in the UI
    juce::AudioParameterFloat* oscMixParam;
    juce::AudioParameterFloat* oscTuneParam;
    juce::AudioParameterFloat* oscFineParam;
    juce::AudioParameterChoice* glideModeParam;
    juce::AudioParameterFloat* glideRateParam;
    juce::AudioParameterFloat* glideBendParam;
    juce::AudioParameterFloat* filterFreqParam;
    juce::AudioParameterFloat* filterResoParam;
    juce::AudioParameterFloat* filterEnvParam;
    juce::AudioParameterFloat* filterLFOParam;
    juce::AudioParameterFloat* filterVelocityParam;
    juce::AudioParameterFloat* filterAttackParam;
    juce::AudioParameterFloat* filterDecayParam;
    juce::AudioParameterFloat* filterSustainParam;
    juce::AudioParameterFloat* filterReleaseParam;
    juce::AudioParameterFloat* envAttackParam;
    juce::AudioParameterFloat* envDecayParam;
    juce::AudioParameterFloat* envSustainParam;
    juce::AudioParameterFloat* envReleaseParam;
    juce::AudioParameterFloat* lfoRateParam;
    juce::AudioParameterFloat* vibratoParam;
    juce::AudioParameterFloat* noiseParam;
    juce::AudioParameterFloat* octaveParam;
    juce::AudioParameterFloat* tuningParam;
    juce::AudioParameterFloat* outputLevelParam;
    juce::AudioParameterChoice* polyModeParam;
};
