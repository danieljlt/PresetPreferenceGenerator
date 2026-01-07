/*
  ==============================================================================
    CrossoverOperators.h
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Crossover operator functors for genetic algorithm.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "Individual.h"

//==============================================================================
/**
    Uniform crossover operator.
    Each parameter is randomly inherited from either parent with equal probability.
*/
struct UniformCrossover
{
    Individual operator()(const Individual& parent1, 
                         const Individual& parent2, 
                         juce::Random& rng) const;
};

