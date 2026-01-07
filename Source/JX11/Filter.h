/*
  ==============================================================================
    Filter.h
    Created: 22 Jun 2025 3:23:20pm
    Author:  Daniel Lister

    A simple state variable filter (SVF) implementation using the
    trapezoidal integrator method (aka TPT). This filter structure is
    numerically stable, efficient, and suitable for real-time DSP.
  ==============================================================================
*/

#pragma once

class Filter
{
public:
    float sampleRate;  // Sample rate used to compute filter coefficients

    // Update filter coefficients based on cutoff frequency and Q factor
    void updateCoefficients(float cutoff, float Q)
    {
        // Convert normalized frequency (0–1) to bilinear transform variable
        g = std::tan(PI * cutoff / sampleRate); // Pre-warped cutoff frequency
        k = 1.0f / Q;                            // Inverse of quality factor

        // Coefficient calculations for TPT-based state variable filter
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Reset internal state and coefficients
    void reset()
    {
        g = 0.0f;
        k = 0.0f;
        a1 = 0.0f;
        a2 = 0.0f;
        a3 = 0.0f;
        ic1eq = 0.0f;  // Internal integrator state 1
        ic2eq = 0.0f;  // Internal integrator state 2
    }

    // Process a single audio sample through the filter
    float render(float x)
    {
        // Compute intermediate variables using TPT structure
        float v3 = x - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        // Update internal states for next sample
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        // Return the band-pass or low-pass output (depending on design)
        return v2;
    }

private:
    const float PI = 3.1415926535897932f;

    // Filter coefficients (computed from cutoff & Q)
    float g, k, a1, a2, a3;

    // Filter internal states (TPT integrator history)
    float ic1eq, ic2eq;
};
