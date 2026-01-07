/*
  ==============================================================================
    HeadlessSynth.cpp
    Created: 6 Oct 2025
    Author:  Daniel Lister

    Implements offline audio rendering with normalized parameter input.
  ==============================================================================
*/

#include "HeadlessSynth.h"
#include "../JX11/Utils.h"
#include <cmath>

HeadlessSynth::HeadlessSynth(double sampleRate_, int blockSize_)
    : sampleRate(sampleRate_), blockSize(blockSize_)
{
    synth.allocateResources(sampleRate, blockSize);
}

void HeadlessSynth::setParameters(const std::vector<float>& normalizedParams)
{
    if (normalizedParams.size() != HeadlessParam::COUNT)
        return;  // Invalid parameter count
    
    updateSynthParameters(normalizedParams);
}

void HeadlessSynth::updateSynthParameters(const std::vector<float>& params)
{
    float inverseSampleRate = 1.0f / static_cast<float>(sampleRate);
    const float inverseUpdateRate = inverseSampleRate * synth.LFO_MAX;
    
    // Map normalized [0,1] to actual parameter ranges (17 GA parameters)
    float oscMixValue = mapParameter(params[HeadlessParam::oscMix], 0.0f, 100.0f);
    float oscFineValue = mapParameter(params[HeadlessParam::oscFine], -50.0f, 50.0f);
    float filterFreqValue = mapParameter(params[HeadlessParam::filterFreq], 0.0f, 100.0f);
    float filterResoValue = mapParameter(params[HeadlessParam::filterReso], 0.0f, 100.0f);
    float filterEnvValue = mapParameter(params[HeadlessParam::filterEnv], -100.0f, 100.0f);
    float filterLFOValue = mapParameter(params[HeadlessParam::filterLFO], 0.0f, 100.0f);
    float filterAttackValue = mapParameter(params[HeadlessParam::filterAttack], 0.0f, 100.0f);
    float filterDecayValue = mapParameter(params[HeadlessParam::filterDecay], 0.0f, 100.0f);
    float filterSustainValue = mapParameter(params[HeadlessParam::filterSustain], 0.0f, 100.0f);
    float filterReleaseValue = mapParameter(params[HeadlessParam::filterRelease], 0.0f, 100.0f);
    float envAttackValue = mapParameter(params[HeadlessParam::envAttack], 0.0f, 100.0f);
    float envDecayValue = mapParameter(params[HeadlessParam::envDecay], 15.0f, 100.0f);
    float envSustainValue = mapParameter(params[HeadlessParam::envSustain], 0.0f, 100.0f);
    float envReleaseValue = mapParameter(params[HeadlessParam::envRelease], 0.0f, 100.0f);
    float lfoRateValue = params[HeadlessParam::lfoRate];  // Already 0-1
    float vibratoValue = mapParameter(params[HeadlessParam::vibrato], -100.0f, 100.0f);
    float noiseValue = mapParameter(params[HeadlessParam::noise], 0.0f, 100.0f);
    
    // Fixed values for excluded parameters (not controlled by GA)
    float oscTuneValue = 0.0f;
    float glideBendValue = 0.0f;
    float filterVelocityValue = 50.0f;
    float octaveValue = 0.0f;
    float tuningValue = 0.0f;
    float outputLevelValue = 0.0f;  // 0 dB
    
    // Amplitude envelope
    synth.envAttack = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envAttackValue));
    synth.envDecay = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envDecayValue));
    synth.envSustain = envSustainValue / 100.0f;
    synth.envRelease = (envReleaseValue < 1.0f) ? 0.75f
                                                : std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envReleaseValue));
    
    // Noise mix (nonlinear scaling)
    float noiseMix = noiseValue / 100.0f;
    noiseMix *= noiseMix;
    synth.noiseMix = noiseMix * 0.06f;
    
    // Oscillator mix and tuning
    synth.oscMix = oscMixValue / 100.0f;
    synth.detune = std::pow(1.059463094359f, -oscTuneValue - 0.01f * oscFineValue);
    
    // Base tuning frequency (using fixed octave and tuning values)
    float tuneInSemi = -36.3763f - 12.0f * octaveValue - tuningValue / 100.0f;
    synth.tune = static_cast<float>(sampleRate) * std::exp(0.05776226505f * tuneInSemi);
    
    // Voice count (fixed to polyphonic)
    synth.numVoices = Synth::MAX_VOICES;
    
    // Filter modulation
    float filterLFO = filterLFOValue / 100.0f;
    synth.filterLFODepth = 2.5f * filterLFO * filterLFO;
    
    float filterReso = filterResoValue / 100.0f;
    synth.filterQ = std::exp(3.0f * filterReso);
    
    // Volume trim compensation
    synth.volumeTrim = 0.0008f * (3.2f - synth.oscMix - 25.0f * synth.noiseMix) * (1.5f - 0.5f * filterReso);
    
    // Output level (fixed to 0 dB)
    synth.outputLevelSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelValue));
    
    // Velocity sensitivity (fixed to moderate)
    synth.velocitySensitivity = 0.0005f * filterVelocityValue;
    synth.ignoreVelocity = false;
    
    // LFO rate
    float lfoRate = std::exp(7.0f * lfoRateValue - 4.0f);
    synth.lfoInc = lfoRate * inverseUpdateRate * float(TWO_PI);
    
    // Vibrato and PWM
    float vibrato = vibratoValue / 200.0f;
    synth.vibrato = 0.2f * vibrato * vibrato;
    synth.pwmDepth = synth.vibrato;
    if (vibrato < 0.0f) synth.vibrato = 0.0f;
    
    // Glide (fixed to off)
    synth.glideMode = 0;
    synth.glideRate = 1.0f;
    synth.glideBend = glideBendValue;
    
    // Filter envelope and tracking
    synth.filterKeyTracking = 0.08f * filterFreqValue - 1.5f;
    synth.filterAttack = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterAttackValue));
    synth.filterDecay = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterDecayValue));
    float filterSustain = filterSustainValue / 100.0f;
    synth.filterSustain = filterSustain * filterSustain;
    synth.filterRelease = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterReleaseValue));
    synth.filterEnvDepth = 0.06f * filterEnvValue;
}

juce::AudioBuffer<float> HeadlessSynth::renderNote(int midiNote, int velocity, int durationInSamples, float noteOnDuration)
{
    synth.reset();  // Clear state for consistent results
    
    juce::AudioBuffer<float> buffer(1, durationInSamples);  // Mono (more efficient for feature extraction)
    buffer.clear();
    
    float* outputs[2] = { buffer.getWritePointer(0), buffer.getWritePointer(0) };  // Both outputs point to same buffer
    
    // Send note-on at the beginning
    synth.midiMessage(0x90, static_cast<uint8_t>(midiNote), static_cast<uint8_t>(velocity));
    
    // Calculate note-off position
    int noteOffSample = juce::jlimit(0, durationInSamples, static_cast<int>(durationInSamples * noteOnDuration));
    
    // Render up to note-off
    if (noteOffSample > 0)
        synth.render(outputs, noteOffSample);
    
    // Send note-off
    synth.midiMessage(0x80, static_cast<uint8_t>(midiNote), 0);
    
    // Render remaining samples (release phase)
    int remainingSamples = durationInSamples - noteOffSample;
    if (remainingSamples > 0)
    {
        float* offsetOutputs[2] = {
            outputs[0] + noteOffSample,
            outputs[1] + noteOffSample
        };
        synth.render(offsetOutputs, remainingSamples);
    }
    
    return buffer;
}

juce::AudioBuffer<float> HeadlessSynth::renderSequence(const std::vector<MidiEvent>& events, int totalSamples)
{
    synth.reset();  // Clear state
    
    juce::AudioBuffer<float> buffer(1, totalSamples);  // Mono (more efficient for feature extraction)
    buffer.clear();
    
    float* outputs[2] = { buffer.getWritePointer(0), buffer.getWritePointer(0) };  // Both outputs point to same buffer
    
    int currentSample = 0;
    size_t eventIndex = 0;
    
    while (currentSample < totalSamples)
    {
        // Find next event
        int nextEventSample = totalSamples;
        if (eventIndex < events.size())
            nextEventSample = events[eventIndex].samplePosition;
        
        // Render up to next event
        int samplesToRender = juce::jmin(nextEventSample - currentSample, totalSamples - currentSample);
        if (samplesToRender > 0)
        {
            float* offsetOutputs[2] = {
                outputs[0] + currentSample,
                outputs[1] + currentSample
            };
            synth.render(offsetOutputs, samplesToRender);
            currentSample += samplesToRender;
        }
        
        // Process event if we reached it
        if (eventIndex < events.size() && currentSample == events[eventIndex].samplePosition)
        {
            const auto& event = events[eventIndex];
            synth.midiMessage(event.status, event.note, event.velocity);
            ++eventIndex;
        }
    }
    
    return buffer;
}

