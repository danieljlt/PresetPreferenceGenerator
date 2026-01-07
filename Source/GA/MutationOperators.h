/*
  ==============================================================================
    MutationOperators.h
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Mutation operator functors for genetic algorithm.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

// Forward declarations
class Individual;

//==============================================================================
/**
    Uniform mutation operator.
    Each parameter has a probability of being perturbed by a random amount.
*/
struct UniformMutation
{
    float mutationRate = 0.1f;      // Probability per parameter [0,1]
    float mutationStrength = 0.2f;  // Maximum perturbation amount [0,1]
    
    void operator()(Individual& individual, juce::Random& rng) const;
};

