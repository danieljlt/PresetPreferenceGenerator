#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GA/MLP.h"

TEST_CASE("MLP initial prediction is neutral")
{
    MLP mlp;
    std::vector<float> input(MLP::INPUT_SIZE, 0.5f);
    
    float prediction = mlp.predict(input);
    
    REQUIRE_THAT(prediction, Catch::Matchers::WithinAbs(0.5f, 0.01f));
}

TEST_CASE("MLP training moves prediction toward target")
{
    MLP mlp;
    std::vector<float> input(MLP::INPUT_SIZE, 0.5f);
    
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
    std::vector<float> input(MLP::INPUT_SIZE, 0.3f);
    
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
    
    std::vector<float> tooMany(MLP::getWeightCount() + 100, 0.0f);
    REQUIRE(mlp.setWeights(tooMany) == false);
}

TEST_CASE("MLP sample weight affects training magnitude")
{
    MLP mlp1;
    std::vector<float> input(MLP::INPUT_SIZE, 0.5f);
    
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
    std::vector<float> zeros(MLP::INPUT_SIZE, 0.0f);
    float predZero = mlp.predict(zeros);
    REQUIRE(std::isfinite(predZero));
    
    // All ones
    std::vector<float> ones(MLP::INPUT_SIZE, 1.0f);
    float predOne = mlp.predict(ones);
    REQUIRE(std::isfinite(predOne));
    
    // Train with extremes
    mlp.train(zeros, 0.0f, 0.1f);
    mlp.train(ones, 1.0f, 0.1f);
    
    // Should still produce finite values
    REQUIRE(std::isfinite(mlp.predict(zeros)));
    REQUIRE(std::isfinite(mlp.predict(ones)));
}
