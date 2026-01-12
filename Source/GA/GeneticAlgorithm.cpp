/*
  ==============================================================================
    GeneticAlgorithm.cpp
    Created: 3 Oct 2025
    Author:  Daniel Lister

    Basic implementation of genetic algorithm engine.
  ==============================================================================
*/

#include "GeneticAlgorithm.h"
#include "Population.h"
#include "ParameterBridge.h"
#include "Individual.h"
#include "SelectionOperators.h"
#include "CrossoverOperators.h"
#include "MutationOperators.h"
#include "IFitnessModel.h"
#include <algorithm>
#include <vector>

GeneticAlgorithm::GeneticAlgorithm(IFitnessModel& model) 
    : juce::Thread("GeneticAlgorithm"), fitnessModel(model)
{
    // Initialize parameter bridge with default queue capacity of 32
    parameterBridge = std::make_unique<ParameterBridge>(32);
    
    // Set default fitness tolerance (2% worse allowed)
    parameterBridge->setFitnessTolerance(0.02f);
}

GeneticAlgorithm::~GeneticAlgorithm()
{
    stopGA();
}

void GeneticAlgorithm::startGA()
{
    // Don't start if already running
    if (isThreadRunning()) 
    {
        return;
    }
    
    // Reset pause state
    paused.store(false);
    pauseEvent.signal(); // Make sure we're not waiting
    
    // Start the thread with normal priority (good for background processing)
    startThread(juce::Thread::Priority::normal);
}

void GeneticAlgorithm::stopGA()
{
    // Signal the thread to stop
    signalThreadShouldExit();
    
    // Wake up the thread if it's paused
    pauseEvent.signal();
    
    // Wait for the thread to finish (with timeout for safety)
    stopThread(2000); // 2 second timeout
    
    // Reset pause state
    paused.store(false);
    
    // Reset population initialization flag so it will re-initialize on next start
    populationInitialized = false;
    bestFitnessSoFar = 0.0f;
}

void GeneticAlgorithm::pauseGA()
{
    if (isThreadRunning() && !paused.load()) 
    {
        paused.store(true);
        pauseEvent.reset(); // Clear the event so thread will wait
    }
}

void GeneticAlgorithm::resumeGA()
{
    if (isThreadRunning() && paused.load()) 
    {
        paused.store(false);
        pauseEvent.signal(); // Signal the thread to continue
    }
}

void GeneticAlgorithm::initializePopulation()
{
    // Create population with configured size and parameter count
    population = std::make_unique<Population>(POPULATION_SIZE, PARAMETER_COUNT);
    
    // Initialize with random parameters
    population->initializeRandom();
    
    // Evaluate initial population
    for (int i = 0; i < population->size(); ++i)
    {
        if (threadShouldExit())
            return;
            
        Individual& individual = const_cast<Individual&>((*population)[i]);
        float fitness = evaluateIndividual(individual);
        individual.setFitness(fitness);
    }
    
    // Update statistics to find best individual
    population->markDirty();
    float bestFitness = population->getBestFitness();
    bestFitnessSoFar = bestFitness;
    
    populationInitialized = true;
}

float GeneticAlgorithm::evaluateIndividual(const Individual& individual)
{
    return fitnessModel.evaluate(individual.getParameters());
    
    // Fallback if no model (shouldn't happen)
    return 0.0f;
}

void GeneticAlgorithm::debugDumpQueue()
{
    if (parameterBridge)
        parameterBridge->debugLogQueueContents();
}

void GeneticAlgorithm::run()
{
    // Main GA loop - this is called by JUCE's thread system
    while (!threadShouldExit()) 
    {
        // Handle pause state using JUCE's WaitableEvent
        if (paused.load()) 
        {
            pauseEvent.wait(100); // Wait for resume signal or timeout after 100ms
            continue;
        }
        
        // Check if we should exit after handling pause
        if (threadShouldExit()) 
        {
            break;
        }
        
        // Initialize population on first iteration
        if (!populationInitialized)
        {
            DBG("Initializing population");
            initializePopulation();
            
            // Send initial best individual to parameter bridge
            if (population && population->size() > 0)
            {
                Individual& best = population->getBest();
                parameterBridge->push(best.getParameters(), best.getFitness());
            }
            
            if (threadShouldExit())
                break;
                
            continue;  // Go back to start of loop to check for exit/pause
        }
        
        // Check for exit before starting generation
        if (threadShouldExit())
            break;
            
        // ====================================================================
        // STEADY-STATE GA: Generate and evaluate offspring
        // ====================================================================
        
        // Create evolutionary operators
        TournamentSelection selector;
        selector.tournamentSize = 3;
        UniformCrossover crossover;
        UniformMutation mutation;
        mutation.mutationRate = 0.1f;
        mutation.mutationStrength = 0.2f;
        
        // Generate offspring
        std::vector<Individual> offspring;
        offspring.reserve(OFFSPRING_PER_GENERATION);
        
        for (int i = 0; i < OFFSPRING_PER_GENERATION; ++i)
        {
            // Check for pause/exit periodically
            if (threadShouldExit())
                break;
                
            if (paused.load())
            {
                pauseEvent.wait(100);
                continue;
            }
            
            // Select two parents using tournament selection
            int parent1Index = selector(*population, rng);
            int parent2Index = selector(*population, rng);
            
            const Individual& parent1 = (*population)[parent1Index];
            const Individual& parent2 = (*population)[parent2Index];
            
            // Create offspring via crossover
            Individual child = crossover(parent1, parent2, rng);
            
            // Apply mutation
            mutation(child, rng);
            
            // Evaluate offspring fitness
            float fitness = evaluateIndividual(child);
            child.setFitness(fitness);
            
            offspring.push_back(child);
        }
        
        // Check if we should exit after generating offspring
        if (threadShouldExit())
            break;
        
        // ====================================================================
        // REPLACEMENT: Replace worst individuals with offspring
        // ====================================================================
        
        if (!offspring.empty())
        {
            // Find indices of worst individuals
            std::vector<std::pair<int, float>> indexedFitness;
            indexedFitness.reserve(population->size());
            
            for (int i = 0; i < population->size(); ++i)
            {
                const Individual& ind = (*population)[i];
                if (ind.hasBeenEvaluated())
                {
                    indexedFitness.push_back({i, ind.getFitness()});
                }
            }
            
            // Sort by fitness (ascending) to find worst
            std::sort(indexedFitness.begin(), indexedFitness.end(),
                     [](const auto& a, const auto& b) { return a.second < b.second; });
            
            // Replace worst N individuals with offspring
            int numToReplace = std::min(static_cast<int>(offspring.size()), 
                                       static_cast<int>(indexedFitness.size()));
            
            for (int i = 0; i < numToReplace; ++i)
            {
                int worstIndex = indexedFitness[i].first;
                population->replace(worstIndex, offspring[i]);
            }
            
            // Update population statistics
            population->markDirty();
        }
        
        // ====================================================================
        // PARAMETER BRIDGE UPDATE: Push Best Offspring
        // ====================================================================
        
        // Push the best new individual from this generation to ensure diversity.
        // If we only push the population best, we get duplicates until a new best is found.
        if (!offspring.empty())
        {
            auto bestOffspringIt = std::max_element(offspring.begin(), offspring.end(),
                [](const Individual& a, const Individual& b) { return a.getFitness() < b.getFitness(); });
                
            if (bestOffspringIt != offspring.end())
            {
                parameterBridge->push(bestOffspringIt->getParameters(), bestOffspringIt->getFitness());
            }
        }

        // Update stats (best global fitness)
        Individual& globalBest = population->getBest();

        // Update best fitness tracker
        if (globalBest.getFitness() > bestFitnessSoFar)
        {
            bestFitnessSoFar = globalBest.getFitness();
        }
        
        // Small sleep to prevent tight loop if not much work is happening
         juce::Thread::sleep(10);
    }
}

