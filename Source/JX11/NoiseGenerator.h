/*
  ==============================================================================

    NoiseGenerator.h
    Created: 30 Nov 2024 1:54:50pm
    Author:  Daniel Lister

    A simple pseudo-random number generator used to produce white noise.
    This is used in the JX11 synth for adding noise to oscillator output.

  ==============================================================================
*/

#pragma once

class NoiseGenerator
{
public:
    // Resets the internal seed to a fixed starting value
    // This ensures deterministic noise generation if needed
    void reset()
    {
        noiseSeed = 22222;
    }

    // Generates the next pseudorandom noise value between -1.0 and 1.0
    float nextValue()
    {
        // Linear congruential generator step: update the seed
        noiseSeed = noiseSeed * 196314165 + 907633515;

        // Shift down and convert to signed int range
        // >> 7 reduces resolution slightly to ensure symmetric distribution
        int temp = int(noiseSeed >> 7) - 16777216;

        // Normalize to [-1.0, 1.0] float range
        return float(temp) / 16777216.0f;
    }

private:
    // The internal state of the noise generator
    // Uses unsigned int for full 32-bit arithmetic wraparound
    unsigned int noiseSeed;
};
