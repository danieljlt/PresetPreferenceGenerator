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
