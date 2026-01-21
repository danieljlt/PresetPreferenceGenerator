/*
  ==============================================================================
    GAConfig.h
    Created: 20 Jan 2026
    Author:  Daniel Lister

    Configuration struct for toggleable GA experiment flags.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

struct GAConfig
{
    // Adaptive exploration: epsilon decays from epsilonMax to epsilonMin
    bool adaptiveExploration = false;
    float epsilonMax = 0.5f;
    float epsilonMin = 0.05f;
    float epsilonDecay = 0.99f;

    // Novelty bonus: reward individuals different from current population
    bool noveltyBonus = false;
    int noveltyK = 5;  // Number of nearest neighbors for novelty calculation

    // Multi-objective: combine MLP fitness with novelty
    bool multiObjective = false;
    float noveltyWeight = 0.3f;  // Weight for novelty (1 - noveltyWeight for MLP)
    
    juce::String toString() const
    {
        juce::String result;
        if (!adaptiveExploration && !noveltyBonus && !multiObjective)
            return "baseline";
        
        if (adaptiveExploration)
            result += "adaptive";
        if (noveltyBonus)
            result += (result.isEmpty() ? "" : "+") + juce::String("novelty");
        if (multiObjective)
            result += (result.isEmpty() ? "" : "+") + juce::String("multiobjective");
        
        return result;
    }
};
