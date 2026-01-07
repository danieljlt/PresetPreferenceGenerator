/*
  ==============================================================================
    TargetAudioManager.h
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Handles loading and preprocessing of target audio files for GA fitness evaluation.
    Performs resampling, stereo-to-mono conversion, and length trimming.
  ==============================================================================
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

class TargetAudioManager
{
public:
    struct LoadResult
    {
        juce::AudioBuffer<float> audioBuffer;  // Processed audio (mono, 44.1kHz, ≤4 sec)
        bool success;
        juce::String errorMessage;
        
        LoadResult() : success(false) {}
    };
    
    TargetAudioManager();
    
    // Load and preprocess audio file
    // Returns processed buffer: mono, 44.1kHz, maximum 4 seconds
    LoadResult loadAndProcessAudioFile(const juce::File& file);
    
private:
    juce::AudioFormatManager formatManager;
    
    static constexpr double TARGET_SAMPLE_RATE = 44100.0;
    static constexpr int MAX_DURATION_SECONDS = 4;
    static constexpr int MAX_SAMPLES = static_cast<int>(TARGET_SAMPLE_RATE * MAX_DURATION_SECONDS);
    
    // Helper methods
    void convertToMono(juce::AudioBuffer<float>& buffer);
    void resampleBuffer(const juce::AudioBuffer<float>& source, 
                       juce::AudioBuffer<float>& dest,
                       double sourceSampleRate);
};
