/*
  ==============================================================================
    FeatureExtractor.h
    Created: 7 Oct 2025
    Author:  Daniel Lister

    Efficient audio feature extraction for GA fitness evaluation.
    Pre-allocates all buffers for zero-allocation runtime performance.
  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

// Lightweight feature vector for audio comparison
// Captures both average characteristics and temporal variation
struct FeatureVector
{
    std::array<float, 10> mfccMean;      // Average timbre
    std::array<float, 10> mfccStd;       // Timbre variation (modulation, envelope)
    float spectralCentroidMean;          // Average brightness
    float spectralCentroidStd;           // Brightness variation (filter sweeps, LFO)
    float attackTime;                    // Envelope attack (seconds)
    float rmsEnergy;                     // Overall power
    
    FeatureVector()
        : spectralCentroidMean(0.0f)
        , spectralCentroidStd(0.0f)
        , attackTime(0.0f)
        , rmsEnergy(0.0f)
    {
        mfccMean.fill(0.0f);
        mfccStd.fill(0.0f);
    }
};

class FeatureExtractor
{
public:
    FeatureExtractor(double sampleRate, int fftSize = 2048);
    
    // Extract all features from audio buffer (single-pass, pre-allocated)
    FeatureVector extractFeatures(const juce::AudioBuffer<float>& audio);
    
private:
    double sampleRate;
    int fftSize;
    int hopSize;
    int numMelBands;
    
    // Sparse mel filter representation for efficiency
    struct SparseFilter
    {
        std::vector<int> indices;     // Non-zero bin indices
        std::vector<float> weights;   // Corresponding weights
    };
    
    // Pre-allocated buffers (reused for every extraction)
    juce::dsp::FFT fft;
    std::vector<float> fftData;
    std::vector<float> window;
    std::vector<float> magnitude;
    std::vector<float> melEnergies;  // Pre-allocated for zero-allocation runtime
    std::vector<SparseFilter> melFilters;  // Sparse representation (only non-zero values)
    
    // Multi-frame storage (pre-allocated)
    std::vector<std::array<float, 10>> mfccFrames;  // MFCC per frame
    std::vector<float> centroidFrames;              // Centroid per frame
    
    // Feature computation (single frame)
    void computeFFT(const float* audioData, int startSample, int numSamples);
    float computeSpectralCentroid();
    float computeAttackTime(const juce::AudioBuffer<float>& audio);
    float computeRMSEnergy(const juce::AudioBuffer<float>& audio);
    void computeMFCCs(std::array<float, 10>& mfccs);
    
    // Multi-frame analysis
    void extractMultiFrameFeatures(const juce::AudioBuffer<float>& audio, FeatureVector& features);
    void computeMeanAndStd(const std::vector<std::array<float, 10>>& frames, 
                          std::array<float, 10>& mean, std::array<float, 10>& std);
    void computeMeanAndStd(const std::vector<float>& values, float& mean, float& std);
    
    // MFCC helpers
    void initializeMelFilterbank();
    void applyMelFilterbank(std::vector<float>& melEnergies);
    void applyDCT(const std::vector<float>& melEnergies, std::array<float, 10>& mfccs);
    
    // Mel scale conversion
    static float hzToMel(float hz) { return 2595.0f * std::log10(1.0f + hz / 700.0f); }
    static float melToHz(float mel) { return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f); }
};

