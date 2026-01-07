/*
  ==============================================================================
    HeadlessSynth.h
    Created: 6 Oct 2025
    Author:  Daniel Lister

    Headless synthesizer wrapper for offline audio rendering.
    Designed for genetic algorithm fitness evaluation without UI overhead.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../JX11/Synth.h"

// Simple MIDI event structure for sequencing
struct MidiEvent
{
    int samplePosition;
    uint8_t status;    // 0x90 for note-on, 0x80 for note-off
    uint8_t note;
    uint8_t velocity;
};

// Parameter indices for normalized parameter array (GA parameters only)
// Only includes 17 timbre-affecting parameters
// Excluded from GA: oscTune, glideMode, glideRate, glideBend, filterVelocity, octave, tuning, outputLevel, polyMode
namespace HeadlessParam
{
    enum Index
    {
        oscMix = 0,
        oscFine,
        filterFreq,
        filterReso,
        filterEnv,
        filterLFO,
        filterAttack,
        filterDecay,
        filterSustain,
        filterRelease,
        envAttack,
        envDecay,
        envSustain,
        envRelease,
        lfoRate,
        vibrato,
        noise,
        
        COUNT  // Total number of GA parameters (17)
    };
}

class HeadlessSynth
{
public:
    HeadlessSynth(double sampleRate = 44100.0, int blockSize = 512);
    
    // Set parameters from normalized [0,1] values
    void setParameters(const std::vector<float>& normalizedParams);
    
    // Render a single note for a fixed duration
    // noteOnDuration: how long to hold the note before releasing (0.0-1.0 of total duration)
    juce::AudioBuffer<float> renderNote(int midiNote, int velocity, int durationInSamples, float noteOnDuration = 0.8f);
    
    // Render a sequence of MIDI events
    juce::AudioBuffer<float> renderSequence(const std::vector<MidiEvent>& events, int totalSamples);
    
    // Get expected parameter count
    static int getParameterCount() { return HeadlessParam::COUNT; }
    
private:
    Synth synth;
    double sampleRate;
    int blockSize;
    
    // Convert normalized [0,1] parameters to synth-specific values
    void updateSynthParameters(const std::vector<float>& normalizedParams);
    
    // Map a single normalized value to a parameter range
    static inline float mapParameter(float normalized, float min, float max)
    {
        return min + normalized * (max - min);
    }
};

