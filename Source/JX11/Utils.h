/*
  ==============================================================================

    Utils.h
    Created: 30 Nov 2024 2:25:16pm
    Author:  Daniel Lister

    Utility functions for the JX11 synthesizer. Includes a runtime audio
    sanity check to avoid harmful output and a helper for parameter casting.

  ==============================================================================
*/

#pragma once

#include <cstring> // for memset
#include <cmath>   // for std::isnan, std::isinf
#include <juce_audio_processors/juce_audio_processors.h> // for DBG, jassert, JUCE types

//================================================================================
// This function is used to protect the user's ears and speakers from potentially
// dangerous or undefined audio sample values.
//
// It checks a given audio buffer for NaNs, infinities, or extreme values.
// - If any sample is NaN, Inf, or far outside the [-2.0, 2.0] range,
//   it silences the entire buffer.
// - If any sample is slightly out of bounds (e.g. above 1.0 or below -1.0),
//   it clamps that sample and prints a warning only once.
//================================================================================
inline void protectYourEars(float* buffer, int sampleCount)
{
    if (buffer == nullptr) return;

    bool firstWarning = true;

    for (int i = 0; i < sampleCount; ++i)
    {
        float x = buffer[i];
        bool silence = false;

        // Detect and silence buffer if NaN or Inf
        if (std::isnan(x))
        {
            DBG("!!! WARNING: nan detected in audio buffer, silencing !!!");
            silence = true;
        }
        else if (std::isinf(x))
        {
            DBG("!!! WARNING: inf detected in audio buffer, silencing !!!");
            silence = true;
        }
        // Extremely loud sample, likely an error
        else if (x < -2.0f || x > 2.0f)
        {
            DBG("!!! WARNING: sample out of range, silencing !!!");
            silence = true;
        }
        // Mildly out-of-range, clamp to [-1.0, 1.0]
        else if (x < -1.0f)
        {
            if (firstWarning)
            {
                DBG("!!! WARNING: sample out of range, clamping !!!");
                firstWarning = false;
            }
            buffer[i] = -1.0f;
        }
        else if (x > 1.0f)
        {
            if (firstWarning)
            {
                DBG("!!! WARNING: sample out of range, clamping !!!");
                firstWarning = false;
            }
            buffer[i] = 1.0f;
        }

        // If something serious went wrong, silence entire buffer and return
        if (silence)
        {
            memset(buffer, 0, sampleCount * sizeof(float));
            return;
        }
    }
}

//================================================================================
// Templated helper function that safely casts a parameter from the APVTS
// to a specific type (e.g., AudioParameterFloat*).
//
// - Performs a dynamic_cast to verify the parameter type at runtime.
// - Sets the destination pointer.
// - Asserts that the cast was successful for safety during development.
//================================================================================
template<typename T>
inline static void castParameter(juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id, T& destination)
{
    destination = dynamic_cast<T>(apvts.getParameter(id.getParamID()));
    jassert(destination); // Fails loudly if the cast was invalid (debug mode only)
}
