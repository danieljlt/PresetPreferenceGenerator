/*
  ==============================================================================
    Synth.h
    Created: 27 Nov 2024 6:58:30pm
    Author:  Daniel Lister

    Synthesizer engine class. Manages voice allocation, modulation, oscillator
    and filter configuration, and rendering audio to output buffers.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Voice.h"
#include "NoiseGenerator.h"

class Synth
{
public:
    Synth();

    // Oscillator and tuning settings
    float oscMix;       // Mix between osc1 and osc2
    float detune;       // Ratio between osc1 and osc2 frequencies
    float tune;         // Base tuning frequency in Hz
    float noiseMix;     // Noise level

    // Amplitude envelope parameters
    float envAttack;
    float envDecay;
    float envSustain;
    float envRelease;

    // Voice management
    static constexpr int MAX_VOICES = 8;
    int numVoices;      // Current polyphony (1 for mono, MAX_VOICES for poly)

    // Output and gain control
    float volumeTrim;   // Internal gain control for loudness balance
    juce::LinearSmoothedValue<float> outputLevelSmoother; // Smoothed gain to avoid clicks

    // Velocity response
    float velocitySensitivity;
    bool ignoreVelocity;

    // LFO settings
    const int LFO_MAX = 32; // Number of steps per LFO cycle
    float lfoInc;           // LFO increment per sample
    float vibrato;          // Vibrato modulation depth
    float pwmDepth;         // Used for PWM or vibrato routing

    // Glide/portamento
    int glideMode;          // 0 = Off, 1 = Legato, 2 = Always
    float glideRate;        // Rate of pitch change between notes
    float glideBend;        // Glide bend amount in semitones

    // Filter control
    float filterKeyTracking;    // Filter cutoff modulation based on pitch
    float filterQ;              // Filter resonance
    float resonanceCtl;         // Legacy control value (not used directly)
    float filterLFODepth;       // Depth of LFO to filter cutoff
    float filterCtl;            // Additional filter control
    float pressure;             // Aftertouch pressure input (if any)

    // Filter envelope parameters
    float filterAttack, filterDecay, filterSustain, filterRelease;
    float filterEnvDepth;   // Depth of filter envelope modulation

    // === Lifecycle ===

    // Called during plugin prepareToPlay()
    void allocateResources(double sampleRate, int samplesPerBlock);

    // Called during plugin releaseResources()
    void deallocateResources();

    // Reset internal state (voices, envelopes, etc.)
    void reset();

    // Render audio from all active voices into stereo buffers
    void render(float** outputBuffers, int sampleCount);

    // Handle incoming MIDI messages (note on/off, CC, etc.)
    void midiMessage(uint8_t data0, uint8_t data1, uint8_t data2);

    // Trigger a specific voice with a MIDI note and velocity
    void startVoice(int v, int note, int velocity);

private:
    float sampleRate;

    // Array of all voices (synth is voice-managed)
    std::array<Voice, MAX_VOICES> voices;

    NoiseGenerator noiseGen; // Noise source for noiseMix

    // MIDI & modulation state
    float pitchBend;       // Pitch bend multiplier
    bool sustainPedalPressed;
    int lfoStep;           // Current LFO step (integer clock)
    float lfo;             // LFO signal value [0..1] or [-1..1]
    float modWheel;        // Modulation wheel value
    int lastNote;          // Most recent note-on
    float filterZip;       // Internal smoothing control

    // === Note event handling ===

    void noteOn(int note, int velocity);
    void noteOff(int note);

    // Calculate oscillator period (inverse frequency) for a voice
    float calcPeriod(int v, int note) const;

    // Find an unused or stealable voice
    int findFreeVoice() const;

    // Handle MIDI CC (control change) messages
    void controlChange(uint8_t data1, uint8_t data2);

    // In mono mode, retrigger the same voice
    void restartMonoVoice(int note, int velocity);

    // Shift queued notes after a note-off (for mono legato)
    void shiftQueuedNotes();

    // Return next note in monophonic queue (if any)
    int nextQueuedNote();

    // Advance LFO state by one sample
    void updateLFO();

    // Determine whether notes should be retriggered or glided
    bool isPlayingLegatoStyle() const;

    // Update oscillator periods with pitch bend and detune applied
    inline void updatePeriod(Voice& voice)
    {
        voice.osc1.period = voice.period * pitchBend;
        voice.osc2.period = voice.osc1.period * detune;
    }
};
