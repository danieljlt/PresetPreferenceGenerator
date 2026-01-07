/*
  ==============================================================================
    FeatureExtractor.cpp
    Created: 7 Oct 2025
    Author:  Daniel Lister
  ==============================================================================
*/

#include "FeatureExtractor.h"
#include <cmath>

FeatureExtractor::FeatureExtractor(double sampleRate, int fftSize)
    : sampleRate(sampleRate)
    , fftSize(fftSize)
    , hopSize(fftSize / 4)  // 75% overlap for good temporal resolution
    , numMelBands(26)
    , fft(static_cast<int>(std::log2(fftSize)))
{
    // Pre-allocate all buffers
    fftData.resize(fftSize * 2, 0.0f);
    window.resize(fftSize);
    magnitude.resize(fftSize / 2 + 1);
    melEnergies.resize(numMelBands);
    
    // Note: mfccFrames and centroidFrames are reserved dynamically in extractFeatures
    // based on actual audio buffer size
    
    // Hann window
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
    
    initializeMelFilterbank();
}

FeatureVector FeatureExtractor::extractFeatures(const juce::AudioBuffer<float>& audio)
{
    FeatureVector features;
    
    // Extract temporal features (computed over entire audio)
    features.rmsEnergy = computeRMSEnergy(audio);
    features.attackTime = computeAttackTime(audio);
    
    // Extract spectral features over multiple frames
    extractMultiFrameFeatures(audio, features);
    
    return features;
}

void FeatureExtractor::computeFFT(const float* audioData, int startSample, int numSamples)
{
    // Clear FFT data
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    
    // Copy windowed audio to FFT buffer
    int samplesToUse = std::min(fftSize, numSamples - startSample);
    samplesToUse = std::max(0, samplesToUse);
    
    for (int i = 0; i < samplesToUse; ++i)
        fftData[i] = audioData[startSample + i] * window[i];
    
    // Perform FFT
    fft.performRealOnlyForwardTransform(fftData.data());
    
    // Compute magnitude spectrum
    for (int i = 0; i < (int)magnitude.size(); ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        magnitude[i] = std::sqrt(real * real + imag * imag);
    }
}

float FeatureExtractor::computeSpectralCentroid()
{
    float weightedSum = 0.0f;
    float sum = 0.0f;
    const float freqPerBin = sampleRate / fftSize;
    
    // Compute weighted centroid (frequency * magnitude)
    for (int i = 0; i < (int)magnitude.size(); ++i)
    {
        float mag = magnitude[i];
        weightedSum += (i * freqPerBin) * mag;
        sum += mag;
    }
    
    return sum > 1e-10f ? weightedSum / sum : 0.0f;
}

float FeatureExtractor::computeAttackTime(const juce::AudioBuffer<float>& audio)
{
    const int numSamples = audio.getNumSamples();
    const float* audioData = audio.getReadPointer(0);
    
    // Single-pass algorithm: find peak and track where amplitude exceeds threshold
    float peakAmp = 0.0f;
    int peakIndex = 0;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float amp = std::abs(audioData[i]);
        if (amp > peakAmp)
        {
            peakAmp = amp;
            peakIndex = i;
        }
    }
    
    if (peakAmp < 0.001f)
        return 0.0f;
    
    // Search backwards from peak to find start of attack
    float threshold = peakAmp * 0.05f;
    int startIndex = 0;
    
    for (int i = peakIndex; i >= 0; --i)
    {
        if (std::abs(audioData[i]) <= threshold)
        {
            startIndex = i;
            break;
        }
    }
    
    // Attack time in seconds
    return (peakIndex - startIndex) / (float)sampleRate;
}

float FeatureExtractor::computeRMSEnergy(const juce::AudioBuffer<float>& audio)
{
    const int numSamples = audio.getNumSamples();
    const float* audioData = audio.getReadPointer(0);
    
    if (numSamples == 0)
        return 0.0f;
    
    // Compute sum of squares
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = audioData[i];
        sum += sample * sample;
    }
    
    return std::sqrt(sum / numSamples);
}

void FeatureExtractor::initializeMelFilterbank()
{
    // Create triangular mel-scale filters using sparse representation
    // Most filter coefficients are zero, so we only store non-zero values
    float minMel = hzToMel(0.0f);
    float maxMel = hzToMel(sampleRate / 2.0f);
    
    melFilters.resize(numMelBands);
    
    const float minWeight = 1e-6f;  // Ignore near-zero weights
    
    // Create filter bank with proper boundary handling
    for (int i = 0; i < numMelBands; ++i)
    {
        // Mel frequencies for this filter
        float leftMel = minMel + (maxMel - minMel) * i / (numMelBands + 1);
        float centerMel = minMel + (maxMel - minMel) * (i + 1) / (numMelBands + 1);
        float rightMel = minMel + (maxMel - minMel) * (i + 2) / (numMelBands + 1);
        
        float leftHz = melToHz(leftMel);
        float centerHz = melToHz(centerMel);
        float rightHz = melToHz(rightMel);
        
        // Convert Hz to FFT bin indices for faster lookup
        int leftBin = static_cast<int>(leftHz * fftSize / sampleRate);
        int centerBin = static_cast<int>(centerHz * fftSize / sampleRate);
        int rightBin = static_cast<int>(rightHz * fftSize / sampleRate);
        
        // Clamp to valid range
        leftBin = std::max(0, std::min(leftBin, (int)magnitude.size() - 1));
        centerBin = std::max(0, std::min(centerBin, (int)magnitude.size() - 1));
        rightBin = std::max(0, std::min(rightBin, (int)magnitude.size() - 1));
        
        // Reserve approximate space (triangular filter typically spans 50-150 bins)
        melFilters[i].indices.reserve(rightBin - leftBin + 1);
        melFilters[i].weights.reserve(rightBin - leftBin + 1);
        
        // Build triangular filter, storing only non-zero values
        for (int j = leftBin; j <= rightBin; ++j)
        {
            float freq = j * sampleRate / fftSize;
            float weight = 0.0f;
            
            if (freq >= leftHz && freq <= centerHz)
            {
                float denom = centerHz - leftHz;
                weight = (denom > 0.0f) ? (freq - leftHz) / denom : 0.0f;
            }
            else if (freq > centerHz && freq <= rightHz)
            {
                float denom = rightHz - centerHz;
                weight = (denom > 0.0f) ? (rightHz - freq) / denom : 0.0f;
            }
            
            // Only store non-negligible weights
            if (weight > minWeight)
            {
                melFilters[i].indices.push_back(j);
                melFilters[i].weights.push_back(weight);
            }
        }
    }
}

void FeatureExtractor::applyMelFilterbank(std::vector<float>& melEnergies)
{
    // No allocation - assumes melEnergies is already sized correctly
    // Use sparse representation: only process non-zero filter coefficients
    for (int i = 0; i < numMelBands; ++i)
    {
        float energy = 0.0f;
        const auto& filter = melFilters[i];
        
        // Iterate only over non-zero filter coefficients (typically ~50-150 vs ~1024)
        for (size_t k = 0; k < filter.indices.size(); ++k)
            energy += magnitude[filter.indices[k]] * filter.weights[k];
        
        // Log compression
        melEnergies[i] = std::log(energy + 1e-10f);
    }
}

void FeatureExtractor::applyDCT(const std::vector<float>& melEnergies, std::array<float, 10>& mfccs)
{
    // Discrete Cosine Transform to get MFCCs
    for (int i = 0; i < 10; ++i)
    {
        mfccs[i] = 0.0f;
        for (int j = 0; j < numMelBands; ++j)
        {
            mfccs[i] += melEnergies[j] * std::cos(juce::MathConstants<float>::pi * i * (j + 0.5f) / numMelBands);
        }
    }
}

void FeatureExtractor::computeMFCCs(std::array<float, 10>& mfccs)
{
    // Use pre-allocated member variable for zero-allocation runtime
    applyMelFilterbank(melEnergies);
    applyDCT(melEnergies, mfccs);
}

void FeatureExtractor::extractMultiFrameFeatures(const juce::AudioBuffer<float>& audio, FeatureVector& features)
{
    const int numSamples = audio.getNumSamples();
    const float* audioData = audio.getReadPointer(0);
    
    // Calculate exactly how many frames we need for this audio buffer
    int numFrames = (numSamples / hopSize) + 1;
    
    // Reserve space if needed (only reallocates if buffer grew)
    if (numFrames > (int)mfccFrames.capacity())
    {
        mfccFrames.reserve(numFrames);
        centroidFrames.reserve(numFrames);
    }
    
    // Clear frame storage (reuse allocated memory)
    mfccFrames.clear();
    centroidFrames.clear();
    
    // Process overlapping frames across the entire audio
    for (int startSample = 0; startSample + fftSize <= numSamples; startSample += hopSize)
    {
        // Compute FFT for this frame
        computeFFT(audioData, startSample, numSamples);
        
        // Extract features from this frame
        std::array<float, 10> frameMFCCs;
        computeMFCCs(frameMFCCs);
        mfccFrames.push_back(frameMFCCs);
        
        float frameCentroid = computeSpectralCentroid();
        centroidFrames.push_back(frameCentroid);
    }
    
    // If we didn't get any frames, handle edge case
    if (mfccFrames.empty())
    {
        // Fall back to single frame from center of audio
        int startSample = std::max(0, (numSamples - fftSize) / 2);
        computeFFT(audioData, startSample, numSamples);
        
        std::array<float, 10> frameMFCCs;
        computeMFCCs(frameMFCCs);
        mfccFrames.push_back(frameMFCCs);
        
        float frameCentroid = computeSpectralCentroid();
        centroidFrames.push_back(frameCentroid);
    }
    
    // Compute mean and standard deviation across all frames
    computeMeanAndStd(mfccFrames, features.mfccMean, features.mfccStd);
    computeMeanAndStd(centroidFrames, features.spectralCentroidMean, features.spectralCentroidStd);
}

void FeatureExtractor::computeMeanAndStd(const std::vector<std::array<float, 10>>& frames,
                                         std::array<float, 10>& mean, std::array<float, 10>& std)
{
    const int numFrames = static_cast<int>(frames.size());
    if (numFrames == 0)
    {
        mean.fill(0.0f);
        std.fill(0.0f);
        return;
    }
    
    // Compute mean for each coefficient
    mean.fill(0.0f);
    for (const auto& frame : frames)
    {
        for (int i = 0; i < 10; ++i)
            mean[i] += frame[i];
    }
    for (int i = 0; i < 10; ++i)
        mean[i] /= numFrames;
    
    // Compute standard deviation for each coefficient
    std.fill(0.0f);
    if (numFrames > 1)
    {
        for (const auto& frame : frames)
        {
            for (int i = 0; i < 10; ++i)
            {
                float diff = frame[i] - mean[i];
                std[i] += diff * diff;
            }
        }
        for (int i = 0; i < 10; ++i)
            std[i] = std::sqrt(std[i] / (numFrames - 1));
    }
}

void FeatureExtractor::computeMeanAndStd(const std::vector<float>& values, float& mean, float& std)
{
    const int numValues = static_cast<int>(values.size());
    if (numValues == 0)
    {
        mean = 0.0f;
        std = 0.0f;
        return;
    }
    
    // Compute mean
    mean = 0.0f;
    for (float value : values)
        mean += value;
    mean /= numValues;
    
    // Compute standard deviation
    std = 0.0f;
    if (numValues > 1)
    {
        for (float value : values)
        {
            float diff = value - mean;
            std += diff * diff;
        }
        std = std::sqrt(std / (numValues - 1));
    }
}

