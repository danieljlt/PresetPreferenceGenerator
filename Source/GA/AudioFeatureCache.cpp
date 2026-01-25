/*
  ==============================================================================
    AudioFeatureCache.cpp
    Created: 21 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "AudioFeatureCache.h"
#include <cstring>

AudioFeatureCache::AudioFeatureCache(double sampleRate_)
    : sampleRate(sampleRate_)
{
    synth = std::make_unique<HeadlessSynth>(sampleRate);
    extractor = std::make_unique<FeatureExtractor>(sampleRate);
}

std::vector<float> AudioFeatureCache::getFeatures(const std::vector<float>& genome)
{
    size_t hash = hashGenome(genome);
    
    auto it = cache.find(hash);
    if (it != cache.end())
    {
        // Cache hit - move to front of LRU
        ++cacheHits;
        lruOrder.erase(lruMap[hash]);
        lruOrder.push_front(hash);
        lruMap[hash] = lruOrder.begin();
        return it->second;
    }
    
    // Cache miss - extract features
    ++cacheMisses;
    std::vector<float> features = extractFeatures(genome);
    
    // Evict if necessary
    if (cache.size() >= maxCacheSize)
        evictLRU();
    
    // Add to cache
    cache[hash] = features;
    lruOrder.push_front(hash);
    lruMap[hash] = lruOrder.begin();
    
    return features;
}

bool AudioFeatureCache::hasCached(const std::vector<float>& genome) const
{
    size_t hash = hashGenome(genome);
    return cache.find(hash) != cache.end();
}

void AudioFeatureCache::setSampleRate(double newSampleRate)
{
    if (std::abs(newSampleRate - sampleRate) < 1.0)
        return;  // No significant change
    
    sampleRate = newSampleRate;
    synth = std::make_unique<HeadlessSynth>(sampleRate);
    extractor = std::make_unique<FeatureExtractor>(sampleRate);
    clear();
}

void AudioFeatureCache::clear()
{
    cache.clear();
    lruOrder.clear();
    lruMap.clear();
    cacheHits = 0;
    cacheMisses = 0;
}

size_t AudioFeatureCache::hashGenome(const std::vector<float>& genome) const
{
    // Hash float bits directly for collision-free hashing
    size_t hash = 0;
    for (float val : genome)
    {
        uint32_t bits;
        std::memcpy(&bits, &val, sizeof(bits));
        hash ^= std::hash<uint32_t>{}(bits) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

std::vector<float> AudioFeatureCache::extractFeatures(const std::vector<float>& genome)
{
    synth->setParameters(genome);
    
    // Render using current sample rate
    int totalSamples = static_cast<int>(sampleRate * totalDurationMs / 1000.0);
    int noteDuration = totalSamples / 4;
    
    // Create a phrase: C4-E4-G4-C5 with varied velocities
    std::vector<MidiEvent> events = {
        {0, 0x90, 60, 110},
        {noteDuration - 100, 0x80, 60, 0},
        {noteDuration, 0x90, 64, 80},
        {noteDuration * 2 - 100, 0x80, 64, 0},
        {noteDuration * 2, 0x90, 67, 50},
        {noteDuration * 3 - 100, 0x80, 67, 0},
        {noteDuration * 3, 0x90, 72, 100},
        {totalSamples - 200, 0x80, 72, 0}
    };
    
    juce::AudioBuffer<float> audio = synth->renderSequence(events, totalSamples);
    FeatureVector fv = extractor->extractFeatures(audio);
    
    // Flatten to vector
    std::vector<float> features;
    features.reserve(AUDIO_FEATURE_COUNT);
    
    for (int i = 0; i < 10; ++i)
        features.push_back(fv.mfccMean[i]);
    
    for (int i = 0; i < 10; ++i)
        features.push_back(fv.mfccStd[i]);
    
    features.push_back(fv.spectralCentroidMean);
    features.push_back(fv.spectralCentroidStd);
    features.push_back(fv.attackTime);
    features.push_back(fv.rmsEnergy);
    
    // Normalize all features to [0, 1]
    normalizeFeatures(features);
    
    return features;
}

void AudioFeatureCache::normalizeFeatures(std::vector<float>& features)
{
    // MFCCs mean (indices 0-9)
    for (int i = 0; i < 10; ++i)
        features[i] = normalizeValue(features[i], mfccMin, mfccMax);
    
    // MFCCs std (indices 10-19) - std is always positive, use 0 to mfccMax
    for (int i = 10; i < 20; ++i)
        features[i] = normalizeValue(features[i], 0.0f, mfccMax);
    
    // Spectral centroid mean and std (indices 20-21)
    features[20] = normalizeValue(features[20], centroidMin, centroidMax);
    features[21] = normalizeValue(features[21], 0.0f, centroidMax - centroidMin);
    
    // Attack time (index 22)
    features[22] = normalizeValue(features[22], attackMin, attackMax);
    
    // RMS energy (index 23)
    features[23] = normalizeValue(features[23], rmsMin, rmsMax);
}

float AudioFeatureCache::normalizeValue(float value, float min, float max)
{
    float normalized = (value - min) / (max - min);
    // Clamp to [0, 1]
    return std::max(0.0f, std::min(1.0f, normalized));
}

void AudioFeatureCache::evictLRU()
{
    if (lruOrder.empty())
        return;
    
    size_t lruHash = lruOrder.back();
    lruOrder.pop_back();
    lruMap.erase(lruHash);
    cache.erase(lruHash);
}

