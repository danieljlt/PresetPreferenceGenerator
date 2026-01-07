/*
  ==============================================================================
    CrossoverOperators.cpp
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Implementation of crossover operators.
  ==============================================================================
*/

#include "CrossoverOperators.h"

Individual UniformCrossover::operator()(const Individual& parent1, 
                                       const Individual& parent2, 
                                       juce::Random& rng) const
{
    const int paramCount = parent1.getParameterCount();
    
    // Create offspring with same parameter count
    Individual offspring(paramCount);
    
    // Uniform crossover: randomly pick each parameter from either parent
    for (int i = 0; i < paramCount; ++i)
    {
        float value = (rng.nextBool()) ? parent1.getParameter(i) 
                                       : parent2.getParameter(i);
        offspring.setParameter(i, value);
    }
    
    // Offspring starts with invalidated fitness (needs evaluation)
    return offspring;
}

