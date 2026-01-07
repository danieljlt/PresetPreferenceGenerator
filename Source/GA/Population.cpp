/*
  ==============================================================================
    Population.cpp
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Implementation of population container for genetic algorithm.
  ==============================================================================
*/

#include "Population.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <limits>

Population::Population(int size, int parameterCount)
    : parameterCount(parameterCount)
{
    individuals.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        individuals.emplace_back(parameterCount);
    }
}

void Population::initializeRandom()
{
    juce::Random random;
    
    for (auto& individual : individuals)
    {
        std::vector<float> params(parameterCount);
        for (int i = 0; i < parameterCount; ++i)
        {
            params[i] = random.nextFloat();  // [0, 1]
        }
        individual.setParameters(params);
        individual.invalidateFitness();
    }
    
    markDirty();
}

void Population::clear()
{
    for (auto& individual : individuals)
    {
        std::vector<float> params(parameterCount, 0.0f);
        individual.setParameters(params);
        individual.invalidateFitness();
    }
    
    markDirty();
}

const Individual& Population::operator[](int index) const
{
    jassert(index >= 0 && index < static_cast<int>(individuals.size()));
    return individuals[index];
}

Individual& Population::getBest()
{
    if (statisticsDirty)
    {
        updateStatistics();
    }
    
    jassert(bestIndex >= 0 && bestIndex < static_cast<int>(individuals.size()));
    return individuals[bestIndex];
}

int Population::getBestIndex()
{
    if (statisticsDirty)
    {
        updateStatistics();
    }
    
    jassert(bestIndex >= 0);
    return bestIndex;
}

float Population::getBestFitness()
{
    if (statisticsDirty)
    {
        updateStatistics();
    }
    
    return bestIndex >= 0 ? individuals[bestIndex].getFitness() : 0.0f;
}

float Population::getAverageFitness()
{
    if (statisticsDirty)
    {
        updateStatistics();
    }
    
    return cachedAvgFitness;
}

float Population::getWorstFitness()
{
    if (statisticsDirty)
    {
        updateStatistics();
    }
    
    return cachedWorstFitness;
}

void Population::replace(int index, const Individual& newIndividual)
{
    if (index >= 0 && index < static_cast<int>(individuals.size()))
    {
        individuals[index] = newIndividual;
        markDirty();
    }
}

void Population::markDirty()
{
    statisticsDirty = true;
}

void Population::updateStatistics()
{
    if (individuals.empty())
    {
        bestIndex = -1;
        cachedAvgFitness = 0.0f;
        cachedWorstFitness = 0.0f;
        statisticsDirty = false;
        return;
    }
    
    float bestFitness = -std::numeric_limits<float>::max();
    float worstFitness = std::numeric_limits<float>::max();
    float sumFitness = 0.0f;
    int evaluatedCount = 0;
    
    bestIndex = -1;
    
    for (int i = 0; i < static_cast<int>(individuals.size()); ++i)
    {
        if (individuals[i].hasBeenEvaluated())
        {
            float fitness = individuals[i].getFitness();
            sumFitness += fitness;
            ++evaluatedCount;
            
            if (fitness > bestFitness)
            {
                bestFitness = fitness;
                bestIndex = i;
            }
            
            if (fitness < worstFitness)
            {
                worstFitness = fitness;
            }
        }
    }
    
    cachedAvgFitness = evaluatedCount > 0 ? sumFitness / evaluatedCount : 0.0f;
    cachedWorstFitness = evaluatedCount > 0 ? worstFitness : 0.0f;
    
    statisticsDirty = false;
}

