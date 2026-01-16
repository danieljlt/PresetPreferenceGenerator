#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GA/Population.h"

TEST_CASE("Population getBest returns highest fitness individual")
{
    Population pop(5, 17);
    pop.initializeRandom();
    
    // Set known fitness values
    for (int i = 0; i < 5; ++i)
    {
        Individual& ind = const_cast<Individual&>(pop[i]);
        ind.setFitness(static_cast<float>(i) * 0.2f);
    }
    pop.markDirty();
    
    int bestIndex = pop.getBestIndex();
    
    REQUIRE(bestIndex == 4);  // Highest fitness at index 4
    REQUIRE_THAT(pop.getBestFitness(), Catch::Matchers::WithinAbs(0.8f, 0.001f));
}

TEST_CASE("Population statistics update after replacement")
{
    Population pop(5, 17);
    pop.initializeRandom();
    
    for (int i = 0; i < 5; ++i)
    {
        Individual& ind = const_cast<Individual&>(pop[i]);
        ind.setFitness(0.5f);
    }
    pop.markDirty();
    
    // Replace with a better individual
    Individual better(17);
    better.setFitness(1.0f);
    pop.replace(0, better);
    
    REQUIRE(pop.getBestIndex() == 0);
    REQUIRE_THAT(pop.getBestFitness(), Catch::Matchers::WithinAbs(1.0f, 0.001f));
}
