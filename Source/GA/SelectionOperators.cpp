/*
  ==============================================================================
    SelectionOperators.cpp
    Created: 9 Oct 2025
    Author:  Daniel Lister

    Implementation of selection operators.
  ==============================================================================
*/

#include "SelectionOperators.h"
#include "Population.h"

int TournamentSelection::operator()(const Population& population, juce::Random& rng) const
{
    const int popSize = population.size();
    
    // Select first random individual as initial best
    int bestIndex = rng.nextInt(popSize);
    float bestFitness = population[bestIndex].getFitness();
    
    // Compare against remaining tournament participants
    for (int i = 1; i < tournamentSize; ++i)
    {
        int candidateIndex = rng.nextInt(popSize);
        float candidateFitness = population[candidateIndex].getFitness();
        
        if (candidateFitness > bestFitness)
        {
            bestIndex = candidateIndex;
            bestFitness = candidateFitness;
        }
    }
    
    return bestIndex;
}

