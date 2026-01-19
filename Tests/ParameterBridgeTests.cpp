#include <catch2/catch_test_macros.hpp>
#include "GA/ParameterBridge.h"

TEST_CASE("ParameterBridge starts with no data")
{
    ParameterBridge bridge;
    
    REQUIRE(bridge.hasData() == false);
}

TEST_CASE("ParameterBridge push makes data available")
{
    ParameterBridge bridge;
    std::vector<float> params = {0.1f, 0.5f, 0.9f};
    
    bridge.push(params, 0.8f);
    
    REQUIRE(bridge.hasData() == true);
}

TEST_CASE("ParameterBridge pop retrieves pushed values")
{
    ParameterBridge bridge;
    std::vector<float> input = {0.1f, 0.5f, 0.9f};
    float inputFitness = 0.75f;
    
    bridge.push(input, inputFitness);
    
    std::vector<float> output;
    float outputFitness = 0.0f;
    
    bool result = bridge.pop(output, outputFitness);
    
    REQUIRE(result == true);
    REQUIRE(output.size() == 3);
    REQUIRE(output[0] == 0.1f);
    REQUIRE(output[1] == 0.5f);
    REQUIRE(output[2] == 0.9f);
    REQUIRE(outputFitness == 0.75f);
}

TEST_CASE("ParameterBridge pop clears data")
{
    ParameterBridge bridge;
    std::vector<float> params = {0.5f};
    
    bridge.push(params, 0.5f);
    
    std::vector<float> output;
    float fitness;
    bridge.pop(output, fitness);
    
    REQUIRE(bridge.hasData() == false);
}

TEST_CASE("ParameterBridge pop returns false when empty")
{
    ParameterBridge bridge;
    
    std::vector<float> output;
    float fitness = 0.0f;
    
    bool result = bridge.pop(output, fitness);
    
    REQUIRE(result == false);
}

TEST_CASE("ParameterBridge second push overwrites first")
{
    ParameterBridge bridge;
    
    bridge.push({0.1f}, 0.5f);
    bridge.push({0.9f}, 0.95f);
    
    std::vector<float> output;
    float fitness;
    bridge.pop(output, fitness);
    
    REQUIRE(output[0] == 0.9f);
    REQUIRE(fitness == 0.95f);
}

TEST_CASE("ParameterBridge clear removes pending data")
{
    ParameterBridge bridge;
    
    bridge.push({0.5f}, 0.5f);
    bridge.clear();
    
    REQUIRE(bridge.hasData() == false);
}

