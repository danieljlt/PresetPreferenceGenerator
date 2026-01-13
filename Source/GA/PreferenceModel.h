/*
  ==============================================================================
    PreferenceModel.h
    Created: 12 Jan 2026
    Author:  Daniel Lister

    Implementation of IFitnessModel that stores user feedback in a CSV file
    for future MLP training.
  ==============================================================================
*/

#pragma once

#include "IFitnessModel.h"
#include <juce_core/juce_core.h>

class PreferenceModel : public IFitnessModel
{
public:
    PreferenceModel(const std::vector<juce::String>& parameterNames);
    ~PreferenceModel() override;

    /**
     * Evaluates a genome.
     * Currently returns a random score to facilitate exploration
     * until the MLP is trained/integrated.
     */
    float evaluate(const std::vector<float>& genome) override;

    /**
     * Appends the genome and rating to the dataset CSV.
     */
    void sendFeedback(const std::vector<float>& genome, const Feedback& feedback) override;

private:
    juce::File datasetFile;
    juce::Random rng;
    
    // Schema tracking
    const std::vector<juce::String> parameterNames;
    
    // Ensure thread safety for file writing
    juce::CriticalSection fileLock;
    
    // Ensure thread safety for RNG (called from multiple threads)
    juce::CriticalSection rngLock;
    
    // Check if existing file schema matches current code, rotate if not
    void validateSchema();
    
    // Generate the header string (used as schema identifier)
    juce::String getHeaderString() const;
};
