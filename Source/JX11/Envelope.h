/*
  ==============================================================================
    Envelope.h
    Created: 14 May 2025 2:00:41pm
    Author:  Daniel Lister

    This file contains the implementation of the ADSR envelope for JX11.
    The envelope is implemented as a one-pole exponential smoother with
    stage-dependent coefficients.
  ==============================================================================
*/

#pragma once

// Threshold below which the envelope is considered silent/inactive
const float SILENCE = 0.0001f;

class Envelope
{
public:
    // Current envelope level (output amplitude)
    float level;

    // Precomputed exponential coefficients for each envelope stage
    float attackMultiplier;
    float decayMultiplier;
    float sustainLevel;
    float releaseMultiplier;

    // Advance envelope by one sample and return new value
    float nextValue()
    {
        // Exponential smoothing toward the current target
        level = multiplier * (level - target) + target;

        // If still in attack stage and reached or exceeded '2.0', switch to decay
        if (level + target > 3.0f)
        {
            multiplier = decayMultiplier;  // Switch to decay phase
            target = sustainLevel;         // Target is now sustain level
        }

        return level;
    }

    // Reset the envelope to silence and clear state
    void reset()
    {
        level = 0.0f;
        target = 0.0f;
        multiplier = 0.0f;
    }

    // Check if the envelope is still audible (above silence threshold)
    inline bool isActive() const
    {
        return level > SILENCE;
    }

    // Check if the envelope is currently in attack phase
    inline bool isInAttack() const
    {
        return target >= 2.0f;
    }

    // Trigger the envelope's attack phase
    void attack()
    {
        level += SILENCE + SILENCE; // Ensure level is nonzero
        target = 2.0f;              // Arbitrary value >1 to indicate attack phase
        multiplier = attackMultiplier;
    }

    // Trigger the envelope's release phase
    void release()
    {
        target = 0.0f;
        multiplier = releaseMultiplier;
    }

private:
    // Internal state: current target value and stage-specific multiplier
    float multiplier;
    float target;
};
