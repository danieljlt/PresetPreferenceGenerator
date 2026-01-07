/*
  ==============================================================================
    GeneticAlgorithm.h
    Created: 3 Oct 2025
    Author:  Daniel Lister

    Basic genetic algorithm engine for parameter generation.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

// Forward declarations
class ParameterBridge;
class Population;
class FeatureExtractor;
class FitnessEvaluator;
class HeadlessSynth;
class Individual;

class GeneticAlgorithm : public juce::Thread
{
public:
    GeneticAlgorithm();
    ~GeneticAlgorithm() override;
    
    void startGA();
    void stopGA();
    void pauseGA();
    void resumeGA();
    bool isGARunning() const { return isThreadRunning(); }
    bool isGAPaused() const { return paused.load(); }
    
    // Access to parameter bridge for processor to poll updates
    ParameterBridge* getParameterBridge() { return parameterBridge.get(); }
    
    // Target audio management (thread-safe)
    void setTargetAudio(const juce::AudioBuffer<float>& audioBuffer);
    bool hasTargetLoaded() const { return hasTarget.load(); }

private:
    // GA Configuration Constants
    static constexpr int POPULATION_SIZE = 50;
    static constexpr int OFFSPRING_PER_GENERATION = 10;
    static constexpr int PARAMETER_COUNT = 17;  // Number of GA-controlled synth parameters
    static constexpr int MIDI_NOTE = 60;        // Middle C
    static constexpr int VELOCITY = 100;
    static constexpr float NOTE_ON_DURATION = 0.8f;  // 80% of total duration
    
    std::atomic<bool> paused { false };
    juce::WaitableEvent pauseEvent;
    
    // Population management
    std::unique_ptr<Population> population;
    bool populationInitialized = false;
    
    // Parameter communication bridge to main synth
    std::unique_ptr<ParameterBridge> parameterBridge;
    
    // Track best fitness across generations
    float bestFitnessSoFar = 0.0f;
    
    // Target audio and fitness evaluation (GA thread only)
    std::unique_ptr<FeatureExtractor> featureExtractor;
    std::unique_ptr<FitnessEvaluator> fitnessEvaluator;
    int targetAudioLength = 0;  // In samples, for rendering matched-length audio
    
    // Rendering engine for fitness evaluation
    std::unique_ptr<HeadlessSynth> headlessSynth;
    
    // Random number generator for GA operations
    juce::Random rng;
    
    // Thread-safe target update mechanism
    std::mutex targetUpdateMutex;
    std::optional<juce::AudioBuffer<float>> pendingTargetAudio;
    std::atomic<bool> hasTarget { false };
    
    // Override from juce::Thread - this is the main thread function
    void run() override;
    
    // Check for new target audio and update evaluator
    void checkForTargetUpdate();
    
    // Helper methods for GA operations
    void initializePopulation();
    float evaluateIndividual(const Individual& individual);
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneticAlgorithm)
};
