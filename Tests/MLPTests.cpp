#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GA/MLP.h"

static constexpr int GENOME_INPUT_SIZE = 17;

TEST_CASE("MLP initial prediction is neutral")
{
    MLP mlp;
    std::vector<float> input(GENOME_INPUT_SIZE, 0.5f);
    
    float prediction = mlp.predict(input);
    
    REQUIRE_THAT(prediction, Catch::Matchers::WithinAbs(0.5f, 0.01f));
}

TEST_CASE("MLP training moves prediction toward target")
{
    MLP mlp;
    std::vector<float> input(GENOME_INPUT_SIZE, 0.5f);
    
    float initial = mlp.predict(input);
    
    // Train toward 1.0 (Like)
    for (int i = 0; i < 10; ++i)
    {
        mlp.train(input, 1.0f, 0.1f);
    }
    
    float afterTraining = mlp.predict(input);
    
    REQUIRE(afterTraining > initial);
}

TEST_CASE("MLP weight persistence round-trips correctly")
{
    MLP mlp1;
    std::vector<float> input(GENOME_INPUT_SIZE, 0.3f);
    
    // Train to create non-trivial weights
    for (int i = 0; i < 5; ++i)
    {
        mlp1.train(input, 1.0f, 0.1f);
    }
    
    float prediction1 = mlp1.predict(input);
    auto weights = mlp1.getWeights();
    
    MLP mlp2;
    REQUIRE(mlp2.setWeights(weights));
    
    float prediction2 = mlp2.predict(input);
    
    REQUIRE_THAT(prediction2, Catch::Matchers::WithinAbs(prediction1, 0.0001f));
}

TEST_CASE("MLP setWeights returns false for wrong size")
{
    MLP mlp;
    
    std::vector<float> tooFew(10, 0.0f);
    REQUIRE(mlp.setWeights(tooFew) == false);
    
    std::vector<float> tooMany(mlp.getWeightCount() + 100, 0.0f);
    REQUIRE(mlp.setWeights(tooMany) == false);
}

TEST_CASE("MLP sample weight affects training magnitude")
{
    MLP mlp1;
    std::vector<float> input(GENOME_INPUT_SIZE, 0.5f);
    
    // Copy weights to second MLP so both start identical
    MLP mlp2;
    mlp2.setWeights(mlp1.getWeights());
    
    float initial = mlp1.predict(input);
    
    // Train mlp1 with low sample weight
    mlp1.train(input, 1.0f, 0.1f, 0.1f);
    float lowWeightDelta = std::abs(mlp1.predict(input) - initial);
    
    // Train mlp2 with high sample weight
    mlp2.train(input, 1.0f, 0.1f, 2.0f);
    float highWeightDelta = std::abs(mlp2.predict(input) - initial);
    
    // Higher sample weight should cause larger change
    REQUIRE(highWeightDelta > lowWeightDelta);
}

TEST_CASE("MLP handles extreme input values without NaN")
{
    MLP mlp;
    
    // All zeros
    std::vector<float> zeros(GENOME_INPUT_SIZE, 0.0f);
    float predZero = mlp.predict(zeros);
    REQUIRE(std::isfinite(predZero));
    
    // All ones
    std::vector<float> ones(GENOME_INPUT_SIZE, 1.0f);
    float predOne = mlp.predict(ones);
    REQUIRE(std::isfinite(predOne));
    
    // Train with extremes
    mlp.train(zeros, 0.0f, 0.1f);
    mlp.train(ones, 1.0f, 0.1f);
    
    // Should still produce finite values
    REQUIRE(std::isfinite(mlp.predict(zeros)));
    REQUIRE(std::isfinite(mlp.predict(ones)));
}

TEST_CASE("MLP with custom input size works correctly")
{
    // Test with audio feature size (24)
    MLP mlpAudio(24, 32);
    std::vector<float> audioInput(24, 0.5f);
    
    float prediction = mlpAudio.predict(audioInput);
    REQUIRE_THAT(prediction, Catch::Matchers::WithinAbs(0.5f, 0.01f));
    
    // Train and verify learning
    float initial = prediction;
    for (int i = 0; i < 10; ++i)
    {
        mlpAudio.train(audioInput, 1.0f, 0.1f);
    }
    
    REQUIRE(mlpAudio.predict(audioInput) > initial);
}

TEST_CASE("Audio MLP learns with realistic feature distribution")
{
    // Test that audio MLP can learn with varied feature values
    // simulating normalized audio features (not uniform 0.5)
    MLP mlpAudio(24, 32);
    
    // Create realistic normalized audio feature vector
    // MFCCs mean (10): varied values around 0.4-0.6
    // MFCCs std (10): smaller values 0.1-0.3
    // Centroid mean/std: 0.3, 0.2
    // Attack: 0.1, RMS: 0.4
    std::vector<float> features = {
        0.45f, 0.52f, 0.48f, 0.55f, 0.42f, 0.58f, 0.47f, 0.53f, 0.44f, 0.56f,  // MFCC mean
        0.15f, 0.18f, 0.12f, 0.22f, 0.14f, 0.20f, 0.16f, 0.19f, 0.13f, 0.21f,  // MFCC std
        0.35f, 0.25f,  // Centroid mean/std
        0.08f, 0.42f   // Attack, RMS
    };
    
    float initial = mlpAudio.predict(features);
    
    // Train toward "like" (1.0)
    for (int i = 0; i < 20; ++i)
    {
        mlpAudio.train(features, 1.0f, 0.1f);
    }
    
    float afterLike = mlpAudio.predict(features);
    REQUIRE(afterLike > initial);
    
    // Train toward "dislike" (0.0)
    for (int i = 0; i < 40; ++i)
    {
        mlpAudio.train(features, 0.0f, 0.1f);
    }
    
    float afterDislike = mlpAudio.predict(features);
    REQUIRE(afterDislike < afterLike);
}

