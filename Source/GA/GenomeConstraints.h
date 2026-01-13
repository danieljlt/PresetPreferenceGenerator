/*
  ==============================================================================
    GenomeConstraints.h
    Created: 13 Jan 2026
    Author:  Daniel Lister

    Ensures genome parameter combinations produce audible sound.
    Repairs problematic combinations (e.g., closed filter with no envelope).
  ==============================================================================
*/

#pragma once

#include <vector>

namespace GenomeConstraints
{
    // Parameter indices in the GA genome (matches getGAParameterIDs order)
    enum ParamIndex
    {
        OscMix = 0,
        OscFine = 1,
        FilterFreq = 2,
        FilterReso = 3,
        FilterEnv = 4,
        FilterLFO = 5,
        FilterAttack = 6,
        FilterDecay = 7,
        FilterSustain = 8,
        FilterRelease = 9,
        EnvAttack = 10,
        EnvDecay = 11,
        EnvSustain = 12,
        EnvRelease = 13,
        LfoRate = 14,
        Vibrato = 15,
        Noise = 16
    };

    /**
     * Repairs genome to ensure audible output.
     * When filter cutoff is very low, ensures envelope depth is positive and
     * sufficient to open the filter during attack.
     */
    void repair(std::vector<float>& genome);
}
