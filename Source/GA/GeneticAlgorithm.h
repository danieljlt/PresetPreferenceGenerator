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
class Individual;
class IFitnessModel;

class GeneticAlgorithm : public juce::Thread
{
public:
    explicit GeneticAlgorithm(IFitnessModel& model);
    ~GeneticAlgorithm() override;
    
    void startGA();
    void stopGA();
    void pauseGA();
    void resumeGA();
    bool isGARunning() const { return isThreadRunning(); }
    bool isGAPaused() const { return paused.load(); }
    
    // Access to parameter bridge for processor to poll updates
    ParameterBridge* getParameterBridge() { return parameterBridge.get(); }

private:
    // GA Configuration Constants
    static constexpr int POPULATION_SIZE = 50;
    static constexpr int OFFSPRING_PER_GENERATION = 10;
    static constexpr int PARAMETER_COUNT = 17;  // Number of GA-controlled synth parameters
    
    std::atomic<bool> paused { false };
    juce::WaitableEvent pauseEvent;
    
    // Population management
    std::unique_ptr<Population> population;
    bool populationInitialized = false;
    
    // Parameter communication bridge to main synth
    std::unique_ptr<ParameterBridge> parameterBridge;
    
    // Track best fitness across generations
    float bestFitnessSoFar = 0.0f;
    
    // Random number generator for GA operations
    juce::Random rng;
    
    // Override from juce::Thread - this is the main thread function
    void run() override;

    // Helper methods for GA operations
    void initializePopulation(bool checkExitSignal = true);
    float evaluateIndividual(const Individual& individual);
    
    // Fitness Model
    IFitnessModel& fitnessModel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneticAlgorithm)
};
