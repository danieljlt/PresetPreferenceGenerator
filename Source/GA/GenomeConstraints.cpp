/*
  ==============================================================================
    GenomeConstraints.cpp
    Created: 13 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "GenomeConstraints.h"
#include <algorithm>

namespace GenomeConstraints
{

void repair(std::vector<float>& genome)
{
    if (genome.size() <= FilterEnv)
        return;

    float filterFreq = genome[FilterFreq];
    float filterEnv = genome[FilterEnv];

    // FilterEnv normalized: 0.0 = -100%, 0.5 = 0%, 1.0 = +100%
    // Positive envelope contribution only when filterEnv > 0.5
    float positiveEnvContribution = std::max(0.0f, (filterEnv - 0.5f) * 2.0f);

    // Combined audibility: filterFreq + weighted positive envelope
    // The envelope contribution is weighted because it only opens filter during attack
    constexpr float envWeight = 0.7f;
    float audibilityScore = filterFreq + positiveEnvContribution * envWeight;

    // Minimum combined score for audible output
    constexpr float minAudibility = 0.35f;

    if (audibilityScore < minAudibility)
    {
        // Calculate how much positive envelope we need to reach minimum
        float deficit = minAudibility - filterFreq;
        float requiredPositiveEnv = deficit / envWeight;
        
        // Convert back to normalized filterEnv (0.5 + requiredPositiveEnv/2)
        float requiredFilterEnv = 0.5f + requiredPositiveEnv / 2.0f;
        genome[FilterEnv] = std::min(1.0f, requiredFilterEnv);
    }
}

}
