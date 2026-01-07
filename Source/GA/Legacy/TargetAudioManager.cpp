/*
  ==============================================================================
    TargetAudioManager.cpp
    Created: 8 Oct 2025
    Author:  Daniel Lister
  ==============================================================================
*/

#include "TargetAudioManager.h"

TargetAudioManager::TargetAudioManager()
{
    // Register standard audio formats
    formatManager.registerBasicFormats();
}

TargetAudioManager::LoadResult TargetAudioManager::loadAndProcessAudioFile(const juce::File& file)
{
    LoadResult result;
    
    // Check if file exists
    if (!file.existsAsFile())
    {
        result.errorMessage = "File does not exist: " + file.getFullPathName();
        return result;
    }
    
    // Create reader for the audio file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader == nullptr)
    {
        result.errorMessage = "Could not read audio file. Unsupported format?";
        return result;
    }
    
    // Get source properties
    double sourceSampleRate = reader->sampleRate;
    int sourceNumChannels = static_cast<int>(reader->numChannels);
    int sourceNumSamples = static_cast<int>(reader->lengthInSamples);
    
    // Input validation
    if (sourceNumSamples <= 0)
    {
        result.errorMessage = "Audio file is empty (0 samples)";
        return result;
    }
    
    if (sourceSampleRate <= 0.0)
    {
        result.errorMessage = "Invalid sample rate: " + juce::String(sourceSampleRate);
        return result;
    }
    
    if (sourceNumChannels <= 0)
    {
        result.errorMessage = "Invalid number of channels: " + juce::String(sourceNumChannels);
        return result;
    }
    
    // Read entire file into temporary buffer
    juce::AudioBuffer<float> tempBuffer(sourceNumChannels, sourceNumSamples);
    reader->read(&tempBuffer, 0, sourceNumSamples, 0, true, true);
    
    // Resample if necessary (use 1 Hz tolerance to avoid unnecessary resampling)
    juce::AudioBuffer<float> resampledBuffer;
    if (std::abs(sourceSampleRate - TARGET_SAMPLE_RATE) > 1.0)
    {
        // Calculate output length after resampling
        int outputLength = static_cast<int>(sourceNumSamples * TARGET_SAMPLE_RATE / sourceSampleRate);
        resampledBuffer.setSize(sourceNumChannels, outputLength);
        resampleBuffer(tempBuffer, resampledBuffer, sourceSampleRate);
    }
    else
    {
        // No resampling needed, use temp buffer directly
        resampledBuffer = std::move(tempBuffer);
    }
    
    // Convert to mono if stereo
    if (resampledBuffer.getNumChannels() > 1)
    {
        convertToMono(resampledBuffer);
    }
    
    // Trim to maximum duration if needed, otherwise move buffer
    int bufferLength = resampledBuffer.getNumSamples();
    
    if (bufferLength > MAX_SAMPLES)
    {
        // Need to trim - copy only the required portion
        result.audioBuffer.setSize(1, MAX_SAMPLES, false, false, false);
        result.audioBuffer.copyFrom(0, 0, resampledBuffer, 0, 0, MAX_SAMPLES);
    }
    else
    {
        // No trimming needed - move buffer to avoid copy
        result.audioBuffer = std::move(resampledBuffer);
    }
    
    result.success = true;
    return result;
}

void TargetAudioManager::convertToMono(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() <= 1)
        return;
    
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    float invChannels = 1.0f / numChannels;
    
    // Use SIMD-optimized vector operations for efficiency
    float* destData = buffer.getWritePointer(0);
    juce::FloatVectorOperations::clear(destData, numSamples);
    
    // Sum all channels into first channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const float* sourceData = buffer.getReadPointer(channel);
        juce::FloatVectorOperations::add(destData, sourceData, numSamples);
    }
    
    // Scale by number of channels to get average
    juce::FloatVectorOperations::multiply(destData, invChannels, numSamples);
    
    // Remove extra channels
    buffer.setSize(1, numSamples, true);
}

void TargetAudioManager::resampleBuffer(const juce::AudioBuffer<float>& source,
                                       juce::AudioBuffer<float>& dest,
                                       double sourceSampleRate)
{
    double ratio = TARGET_SAMPLE_RATE / sourceSampleRate;
    int numChannels = source.getNumChannels();
    int outputLength = dest.getNumSamples();
    
    // Use Lagrange interpolator for each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        juce::LagrangeInterpolator interpolator;
        interpolator.reset();
        
        const float* sourceData = source.getReadPointer(channel);
        float* destData = dest.getWritePointer(channel);
        
        interpolator.process(ratio, sourceData, destData, outputLength);
    }
}
