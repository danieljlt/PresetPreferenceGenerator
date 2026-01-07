/*
  ==============================================================================
    Voice.h
    Created: 27 Nov 2024 6:58:44pm
    Author:  Daniel Lister

    Defines a single voice in a polyphonic synthesizer. Each voice includes
    two oscillators, an envelope, a filter, and per-note modulation state.
  ==============================================================================
*/

#pragma once

#include "Oscillator.h"
#include "Envelope.h"
#include "Filter.h"

struct Voice
{
    int note; // MIDI note number assigned to this voice

    // Two oscillators for detuned saw wave generation
    Oscillator osc1;
    Oscillator osc2;
    float saw;        // Current sawtooth waveform value (running blend)
    float period;     // Base period (inverse of frequency) of oscillator

    Envelope env;     // Amplitude envelope
    float panLeft, panRight; // Stereo panning coefficients
    float target;     // Glide target period
    float glideRate;  // Rate at which period glides to target

    Filter filter;    // Per-voice filter
    float cutoff;     // Base filter cutoff (Hz)
    float filterMod;  // LFO or modulation offset to cutoff (log scale)
    float filterQ;    // Filter resonance (Q factor)
    float pitchBend;  // Pitch bend multiplier (affects tuning)

    Envelope filterEnv;     // Filter envelope
    float filterEnvDepth;   // Filter envelope modulation depth

    // Reset voice to default state
    void reset()
    {
        note = 0;
        saw = 0.0f;

        osc1.reset();
        osc2.reset();
        env.reset();

        panLeft = 0.707f;  // Default to center pan (sin(π/4))
        panRight = 0.707f;

        filter.reset();
        filterEnv.reset();
    }

    // Generate one audio sample from this voice
    float render(float input)
    {
        // Generate saw wave from two oscillators with phase difference
        float sample1 = osc1.nextSample();
        float sample2 = osc2.nextSample();
        saw = saw * 0.997f + sample1 - sample2;  // Filtered blend for smoother saw

        float output = saw + input; // Add external input (e.g., noise or FM)

        // Apply filter to combined waveform
        output = filter.render(output);

        // Apply amplitude envelope
        float envelope = env.nextValue();
        return output * envelope;
    }

    // Trigger release phase for envelopes
    void release()
    {
        env.release();
        filterEnv.release();
    }

    // Set stereo panning based on note pitch
    void updatePanning()
    {
        // Map pitch to -1 (left) to 1 (right) over two octaves centered at middle C (MIDI 60)
        float panning = std::clamp((note - 60.0f) / 24.0f, -1.0f, 1.0f);
        panLeft  = std::sin(PI_OVER_4 * (1.0f - panning));
        panRight = std::sin(PI_OVER_4 * (1.0f + panning));
    }

    // Update filter and pitch state based on LFO, glide, and filter envelope
    void updateLFO()
    {
        // Glide period toward target
        period += glideRate * (target - period);

        // Advance filter envelope
        float fenv = filterEnv.nextValue();

        // Calculate modulated filter cutoff
        float modulatedCutoff = cutoff * std::exp(filterMod + filterEnvDepth * fenv) / pitchBend;

        // Clamp to safe audio frequency range
        modulatedCutoff = std::clamp(modulatedCutoff, 30.0f, 20000.0f);

        // Update filter coefficients for this voice
        filter.updateCoefficients(modulatedCutoff, filterQ);
    }
};
