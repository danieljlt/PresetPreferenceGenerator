#include <catch2/catch_test_macros.hpp>
#include "GA/Individual.h"
#include "GA/MutationOperators.h"
#include "GA/CrossoverOperators.h"
#include "GA/SelectionOperators.h"
#include "GA/Population.h"
#include <juce_core/juce_core.h>

TEST_CASE("Mutation keeps parameters in valid range")
{
    Individual individual(17);
    for (int i = 0; i < 17; ++i)
    {
        individual.setParameter(i, 0.5f);
    }
    
    juce::Random rng(42);
    UniformMutation mutation;
    mutation.mutationRate = 1.0f;  // Mutate all parameters
    mutation.mutationStrength = 1.0f;  // Maximum perturbation
    
    mutation(individual, rng);
    
    for (int i = 0; i < 17; ++i)
    {
        float value = individual.getParameter(i);
        REQUIRE(value >= 0.0f);
        REQUIRE(value <= 1.0f);
    }
}

TEST_CASE("Crossover produces valid offspring")
{
    Individual parent1(17);
    Individual parent2(17);
    
    for (int i = 0; i < 17; ++i)
    {
        parent1.setParameter(i, 0.0f);
        parent2.setParameter(i, 1.0f);
    }
    
    juce::Random rng(42);
    UniformCrossover crossover;
    
    Individual offspring = crossover(parent1, parent2, rng);
    
    REQUIRE(offspring.getParameterCount() == 17);
    
    for (int i = 0; i < 17; ++i)
    {
        float value = offspring.getParameter(i);
        REQUIRE((value == 0.0f || value == 1.0f));
    }
}

TEST_CASE("Tournament selection returns valid index")
{
    Population pop(10, 17);
    pop.initializeRandom();
    
    // Set different fitness values
    for (int i = 0; i < 10; ++i)
    {
        Individual& ind = const_cast<Individual&>(pop[i]);
        ind.setFitness(static_cast<float>(i) / 10.0f);
    }
    pop.markDirty();
    
    juce::Random rng(42);
    TournamentSelection selector;
    selector.tournamentSize = 3;
    
    int selected = selector(pop, rng);
    
    REQUIRE(selected >= 0);
    REQUIRE(selected < 10);
}
