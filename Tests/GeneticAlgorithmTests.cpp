#include <catch2/catch_test_macros.hpp>
#include "GA/GeneticAlgorithm.h"
#include "GA/ParameterBridge.h"
#include "GA/IFitnessModel.h"
#include <juce_core/juce_core.h>

// Simple mock fitness model that returns constant fitness
class MockFitnessModel : public IFitnessModel
{
public:
    float evaluate(const std::vector<float>& /*genome*/) override
    {
        return 0.5f;
    }
    
    void sendFeedback(const std::vector<float>& /*genome*/, const Feedback& /*feedback*/) override
    {
    }
};

TEST_CASE("GeneticAlgorithm starts and stops cleanly")
{
    MockFitnessModel model;
    GeneticAlgorithm ga(model);
    
    REQUIRE(ga.isGARunning() == false);
    
    ga.startGA();
    juce::Thread::sleep(50);  // Let thread start
    
    REQUIRE(ga.isGARunning() == true);
    
    ga.stopGA();
    
    REQUIRE(ga.isGARunning() == false);
}

TEST_CASE("GeneticAlgorithm pause and resume work correctly")
{
    MockFitnessModel model;
    GeneticAlgorithm ga(model);
    
    ga.startGA();
    juce::Thread::sleep(50);
    
    REQUIRE(ga.isGAPaused() == false);
    
    ga.pauseGA();
    
    REQUIRE(ga.isGAPaused() == true);
    REQUIRE(ga.isGARunning() == true);  // Still running, just paused
    
    ga.resumeGA();
    
    REQUIRE(ga.isGAPaused() == false);
    
    ga.stopGA();
}

TEST_CASE("GeneticAlgorithm produces parameter updates")
{
    MockFitnessModel model;
    GeneticAlgorithm ga(model);
    
    ga.startGA();
    
    // Wait for first preset to be available
    juce::Thread::sleep(200);
    
    auto* bridge = ga.getParameterBridge();
    REQUIRE(bridge != nullptr);
    REQUIRE(bridge->hasData() == true);
    
    std::vector<float> params;
    float fitness;
    bool popped = bridge->pop(params, fitness);
    
    REQUIRE(popped == true);
    REQUIRE(params.size() == 17);
    
    ga.stopGA();
}

TEST_CASE("GeneticAlgorithm double start is safe")
{
    MockFitnessModel model;
    GeneticAlgorithm ga(model);
    
    ga.startGA();
    ga.startGA();  // Should not crash or double-start
    
    REQUIRE(ga.isGARunning() == true);
    
    ga.stopGA();
}

TEST_CASE("GeneticAlgorithm double stop is safe")
{
    MockFitnessModel model;
    GeneticAlgorithm ga(model);
    
    ga.startGA();
    juce::Thread::sleep(50);
    
    ga.stopGA();
    ga.stopGA();  // Should not crash
    
    REQUIRE(ga.isGARunning() == false);
}

