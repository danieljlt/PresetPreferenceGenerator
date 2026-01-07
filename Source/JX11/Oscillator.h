/*
  ==============================================================================
    Oscillator.h
    Created: 1 Dec 2024 4:47:28pm
    Author:  Daniel Lister

    A numerically stable oscillator class capable of generating band-limited
    signals for use in subtractive synthesis. Includes logic for sinusoidal
    wave generation and a related square wave offset oscillator.
  ==============================================================================
*/

#pragma once

#include <cmath>

// Constants for phase and trigonometric calculations
const float PI_OVER_4 = 0.7853981633974483f;
const float PI = 3.1415926535897932f;
const float TWO_PI = 6.2831853071795864f;

class Oscillator
{
public:
    // Public parameters
    float period = 0.0f;      // Controls oscillator frequency (in cycles/sample)
    float amplitude = 1.0f;   // Amplitude of the waveform
    float modulation = 1.0f;  // Modulation factor applied to the period

    // Resets the oscillator state to initial conditions
    void reset()
    {
        inc = 0.0f;
        phase = 0.0f;

        sin0 = 0.0f;
        sin1 = 0.0f;
        dsin = 0.0f;

        dc = 0.0f;
    }

    // Generates the next sample of the waveform using recursive sinusoid update
    float nextSample()
    {
        float output = 0.0f;
        phase += inc;

        // If phase is within startup region, reinitialize the oscillator
        if (phase <= PI_OVER_4)
        {
            // Calculate half the waveform period with modulation
            float halfPeriod = (period / 2.0f) * modulation;

            // Find the maximum phase value in radians (snap to nearest 0.5)
            phaseMax = std::floor(0.5f + halfPeriod) - 0.5f;
            dc = 0.5f * amplitude / phaseMax; // Precomputed DC offset for normalization
            phaseMax *= PI;

            // Increment per sample
            inc = phaseMax / halfPeriod;
            phase = -phase;

            // Initialize sin recursion with current phase
            sin0 = amplitude * std::sin(phase);
            sin1 = amplitude * std::sin(phase - inc);
            dsin = 2.0f * std::cos(inc);

            // Use sinc-like formulation to avoid divide-by-zero near phase = 0
            if (phase * phase > 1e-9)
                output = sin0 / phase;
            else
                output = amplitude;
        }
        else
        {
            // Reflect phase and reverse direction if past max phase
            if (phase > phaseMax)
            {
                phase = phaseMax + phaseMax - phase;
                inc = -inc;
            }

            // Recursive oscillator update using previous values
            float sinp = dsin * sin0 - sin1;
            sin1 = sin0;
            sin0 = sinp;

            // Normalize output using current phase
            output = sinp / phase;
        }

        // Remove DC offset and return sample
        return output - dc;
    }

    // Creates a square wave oscillator by mirroring another oscillator's phase and increment
    void squareWave(Oscillator& other, float newPeriod)
    {
        reset();

        // Use phase relationship from reference oscillator
        if (other.inc > 0.0f)
        {
            phase = other.phaseMax + other.phaseMax - other.phase;
            inc = -other.inc;
        }
        else if (other.inc < 0.0f)
        {
            phase = other.phase;
            inc = other.inc;
        }
        else
        {
            // Default phase for silence / no increment
            phase = -PI;
            inc = PI;
        }

        // Offset by half a period to ensure square wave alignment
        phase += PI * newPeriod / 2.0f;
        phaseMax = phase;
    }

private:
    // Internal phase state
    float phase;      // Current phase of the oscillator
    float phaseMax;   // Maximum phase before reflection
    float inc;        // Phase increment per sample

    // Recursive sine generation state
    float sin0;       // sin(θ) at current sample
    float sin1;       // sin(θ - inc)
    float dsin;       // Multiplier for recursive sine update

    // Precomputed DC offset (for removing bias)
    float dc;
};
