/*
  ==============================================================================
    Population.h
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Simple population container for genetic algorithm.
    Manages storage, access, and statistics without evolutionary operators.
  ==============================================================================
*/

#pragma once

#include "Individual.h"
#include <vector>

class Population
{
public:
    Population(int size, int parameterCount);
    
    // Initialization
    void initializeRandom();  // Random [0,1] for each parameter
    void clear();
    
    // Access
    const Individual& operator[](int index) const;
    Individual& getBest();
    int getBestIndex();
    int size() const { return static_cast<int>(individuals.size()); }
    
    // Statistics (efficient with caching)
    float getBestFitness();
    float getAverageFitness();
    float getWorstFitness();
    
    // Replacement/modification
    void replace(int index, const Individual& newIndividual);
    void markDirty();  // Call when external code modifies individuals
    
private:
    std::vector<Individual> individuals;
    int parameterCount;
    
    // Cached statistics
    int bestIndex = -1;
    bool statisticsDirty = true;
    float cachedAvgFitness = 0.0f;
    float cachedWorstFitness = 0.0f;
    
    void updateStatistics();
};

