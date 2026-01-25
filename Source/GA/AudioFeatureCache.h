/*
  ==============================================================================
    AudioFeatureCache.h
    Created: 21 Jan 2026
    Author:  Daniel Lister

    Caches audio features for genomes to avoid redundant rendering.
    Uses LRU eviction when cache is full.
    Features are normalized to [0, 1] for MLP input.
  ==============================================================================
*/

#pragma once

#include "HeadlessSynth.h"
#include "FeatureExtractor.h"
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>

class AudioFeatureCache
{
public:
    // Audio feature count: 10 MFCCs mean + 10 std + centroid mean/std + attack + RMS
    static constexpr int AUDIO_FEATURE_COUNT = 24;
    
    explicit AudioFeatureCache(double sampleRate = 44100.0);
    
    // Get normalized features for a genome, rendering audio if not cached
    std::vector<float> getFeatures(const std::vector<float>& genome);
    
    // Check if genome is cached without triggering render
    bool hasCached(const std::vector<float>& genome) const;
    
    // Update sample rate (clears cache, reinitializes synth/extractor)
    void setSampleRate(double newSampleRate);
    
    // Clear cache
    void clear();
    
    // Cache stats for debugging
    size_t getCacheSize() const { return cache.size(); }
    size_t getCacheHits() const { return cacheHits; }
    size_t getCacheMisses() const { return cacheMisses; }

private:
    std::unique_ptr<HeadlessSynth> synth;
    std::unique_ptr<FeatureExtractor> extractor;
    double sampleRate;
    
    static constexpr size_t maxCacheSize = 128;
    
    // LRU cache: genome hash -> feature vector
    std::unordered_map<size_t, std::vector<float>> cache;
    std::list<size_t> lruOrder;  // Most recently used at front
    std::unordered_map<size_t, std::list<size_t>::iterator> lruMap;
    
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    
    // Phrase duration for multi-note rendering
    static constexpr int totalDurationMs = 2000;
    
    // Normalization ranges (empirically derived)
    static constexpr float mfccMin = -50.0f;
    static constexpr float mfccMax = 50.0f;
    static constexpr float centroidMin = 100.0f;
    static constexpr float centroidMax = 8000.0f;
    static constexpr float attackMin = 0.0f;
    static constexpr float attackMax = 0.5f;
    static constexpr float rmsMin = 0.0f;
    static constexpr float rmsMax = 0.3f;
    
    size_t hashGenome(const std::vector<float>& genome) const;
    std::vector<float> extractFeatures(const std::vector<float>& genome);
    void normalizeFeatures(std::vector<float>& features);
    void evictLRU();
    
    static float normalizeValue(float value, float min, float max);
};
