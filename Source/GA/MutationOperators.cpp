/*
  ==============================================================================
    MutationOperators.cpp
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Implementation of mutation operators.
  ==============================================================================
*/

#include "MutationOperators.h"
#include "Individual.h"

void UniformMutation::operator()(Individual& individual, juce::Random& rng) const
{
    auto& parameters = individual.getParameters();
    const int paramCount = individual.getParameterCount();
    
    bool mutated = false;
    
    for (int i = 0; i < paramCount; ++i)
    {
        if (rng.nextFloat() < mutationRate)
        {
            // Apply random perturbation in range [-strength, +strength]
            float perturbation = (rng.nextFloat() * 2.0f - 1.0f) * mutationStrength;
            float newValue = parameters[i] + perturbation;
            
            // Clamp to [0, 1] range
            parameters[i] = juce::jlimit(0.0f, 1.0f, newValue);
            
            mutated = true;
        }
    }
    
    // Invalidate fitness if any parameter changed
    if (mutated)
    {
        individual.invalidateFitness();
    }
}

