#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GA/AudioFeatureCache.h"

TEST_CASE("AudioFeatureCache cache hit returns identical features")
{
    AudioFeatureCache cache(44100.0);
    
    std::vector<float> genome(17, 0.5f);
    
    auto features1 = cache.getFeatures(genome);
    auto features2 = cache.getFeatures(genome);
    
    REQUIRE(cache.getCacheHits() == 1);
    REQUIRE(features1.size() == features2.size());
    
    for (size_t i = 0; i < features1.size(); ++i)
    {
        REQUIRE(features1[i] == features2[i]);
    }
}

TEST_CASE("AudioFeatureCache cache miss increments counter")
{
    AudioFeatureCache cache(44100.0);
    
    std::vector<float> genome1(17, 0.3f);
    std::vector<float> genome2(17, 0.7f);
    
    cache.getFeatures(genome1);
    REQUIRE(cache.getCacheMisses() == 1);
    
    cache.getFeatures(genome2);
    REQUIRE(cache.getCacheMisses() == 2);
}

TEST_CASE("AudioFeatureCache hasCached returns correct state")
{
    AudioFeatureCache cache(44100.0);
    
    std::vector<float> genome(17, 0.5f);
    
    REQUIRE(cache.hasCached(genome) == false);
    
    cache.getFeatures(genome);
    
    REQUIRE(cache.hasCached(genome) == true);
}

TEST_CASE("AudioFeatureCache normalized features are in valid range")
{
    AudioFeatureCache cache(44100.0);
    
    // Test with various genome values
    std::vector<float> genome(17);
    for (int i = 0; i < 17; ++i)
        genome[i] = static_cast<float>(i) / 16.0f;
    
    auto features = cache.getFeatures(genome);
    
    REQUIRE(features.size() == AudioFeatureCache::AUDIO_FEATURE_COUNT);
    
    for (size_t i = 0; i < features.size(); ++i)
    {
        REQUIRE(features[i] >= 0.0f);
        REQUIRE(features[i] <= 1.0f);
    }
}

TEST_CASE("AudioFeatureCache setSampleRate clears cache")
{
    AudioFeatureCache cache(44100.0);
    
    std::vector<float> genome(17, 0.5f);
    cache.getFeatures(genome);
    
    REQUIRE(cache.getCacheSize() == 1);
    
    cache.setSampleRate(48000.0);
    
    REQUIRE(cache.getCacheSize() == 0);
    REQUIRE(cache.hasCached(genome) == false);
}

TEST_CASE("AudioFeatureCache LRU eviction works")
{
    AudioFeatureCache cache(44100.0);
    
    // Fill cache beyond max size (128)
    for (int i = 0; i < 140; ++i)
    {
        std::vector<float> genome(17, static_cast<float>(i) / 140.0f);
        cache.getFeatures(genome);
    }
    
    // Cache should be at max size, not 140
    REQUIRE(cache.getCacheSize() <= 128);
}
