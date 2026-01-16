#include <catch2/catch_test_macros.hpp>
#include "GA/GenomeConstraints.h"

TEST_CASE("GenomeConstraints repairs low filterFreq with low filterEnv")
{
    std::vector<float> genome(17, 0.5f);
    
    // Low filterFreq (index 2) and low filterEnv (index 4)
    genome[GenomeConstraints::FilterFreq] = 0.1f;
    genome[GenomeConstraints::FilterEnv] = 0.3f;  // Below 0.5 = negative env
    
    GenomeConstraints::repair(genome);
    
    // FilterEnv should be boosted to ensure audibility
    REQUIRE(genome[GenomeConstraints::FilterEnv] > 0.3f);
}

TEST_CASE("GenomeConstraints leaves audible genomes unchanged")
{
    std::vector<float> genome(17, 0.5f);
    
    // High filterFreq = already audible
    genome[GenomeConstraints::FilterFreq] = 0.8f;
    genome[GenomeConstraints::FilterEnv] = 0.3f;
    
    float originalEnv = genome[GenomeConstraints::FilterEnv];
    
    GenomeConstraints::repair(genome);
    
    REQUIRE(genome[GenomeConstraints::FilterEnv] == originalEnv);
}

TEST_CASE("GenomeConstraints handles edge case of very low filterFreq")
{
    std::vector<float> genome(17, 0.5f);
    
    genome[GenomeConstraints::FilterFreq] = 0.0f;
    genome[GenomeConstraints::FilterEnv] = 0.0f;
    
    GenomeConstraints::repair(genome);
    
    // Env should be boosted significantly
    REQUIRE(genome[GenomeConstraints::FilterEnv] >= 0.5f);
}
