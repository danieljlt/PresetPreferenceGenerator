#include <catch2/catch_test_macros.hpp>
#include "GA/Individual.h"

TEST_CASE("Individual default constructor creates empty individual")
{
    Individual ind;
    
    REQUIRE(ind.getParameterCount() == 0);
    REQUIRE(ind.hasBeenEvaluated() == false);
}

TEST_CASE("Individual count constructor initializes with zeros")
{
    Individual ind(17);
    
    REQUIRE(ind.getParameterCount() == 17);
    
    for (int i = 0; i < 17; ++i)
    {
        REQUIRE(ind.getParameter(i) == 0.0f);
    }
}

TEST_CASE("Individual vector constructor copies parameters")
{
    std::vector<float> params = {0.1f, 0.5f, 0.9f};
    Individual ind(params);
    
    REQUIRE(ind.getParameterCount() == 3);
    REQUIRE(ind.getParameter(0) == 0.1f);
    REQUIRE(ind.getParameter(1) == 0.5f);
    REQUIRE(ind.getParameter(2) == 0.9f);
}

TEST_CASE("Individual setParameter invalidates fitness")
{
    Individual ind(5);
    ind.setFitness(0.8f);
    
    REQUIRE(ind.hasBeenEvaluated() == true);
    
    ind.setParameter(2, 0.5f);
    
    REQUIRE(ind.hasBeenEvaluated() == false);
}

TEST_CASE("Individual invalidateFitness resets evaluation state")
{
    Individual ind(5);
    ind.setFitness(0.5f);
    
    REQUIRE(ind.hasBeenEvaluated() == true);
    
    ind.invalidateFitness();
    
    REQUIRE(ind.hasBeenEvaluated() == false);
}

TEST_CASE("Individual out-of-bounds getParameter returns zero")
{
    Individual ind(3);
    ind.setParameter(0, 0.5f);
    
    REQUIRE(ind.getParameter(-1) == 0.0f);
    REQUIRE(ind.getParameter(10) == 0.0f);
}

