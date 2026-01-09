/*
  ==============================================================================
    DummyFitnessModel.h
    Created: 8 Jan 2026
    Author:  Daniel Lister

    Dummy implementation of IFitnessModel that returns random deterministic
    fitness values.
  ==============================================================================
*/

#pragma once

#include "IFitnessModel.h"
#include <juce_core/juce_core.h>

class DummyFitnessModel : public IFitnessModel
{
public:
    /**
     * Constructor.
     * @param seed Optional seed for deterministic random number generation.
     *             If not provided, a random seed based on time is used (if implemented that way, 
     *             but JUCE Random default constructor is reasonably random, here we want optional control).
     */
    explicit DummyFitnessModel(int seed = 0);

    float evaluate(const std::vector<float>& genome) override;
    void sendFeedback(const std::vector<float>& genome, const Feedback& feedback) override;
    
    struct TrainingData {
        std::vector<float> genome;
        Feedback feedback;
    };
    
    const std::vector<TrainingData>& getStoredFeedback() const { return storedFeedback; }

private:
    std::vector<TrainingData> storedFeedback;
    juce::Random rng;
};
