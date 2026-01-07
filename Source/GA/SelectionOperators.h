/*
  ==============================================================================
    SelectionOperators.h
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Selection operator functors for genetic algorithm.
    Used for both parent and survivor selection.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

// Forward declarations
class Population;

//==============================================================================
/**
    Tournament selection operator.
    Randomly selects K individuals and returns the index of the best.
*/
struct TournamentSelection
{
    int tournamentSize = 3;
    
    int operator()(const Population& population, juce::Random& rng) const;
};

