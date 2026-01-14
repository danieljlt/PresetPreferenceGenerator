/*
  ==============================================================================
    ParameterBridge.cpp
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Implementation of mailbox-style parameter communication bridge.
  ==============================================================================
*/

#include "ParameterBridge.h"

void ParameterBridge::push(const std::vector<float>& params, float fitness)
{
    std::lock_guard<std::mutex> lock(mtx);
    parameters = params;
    storedFitness = fitness;
    ready.store(true, std::memory_order_release);
}

bool ParameterBridge::pop(std::vector<float>& params, float& fitness)
{
    if (!ready.load(std::memory_order_acquire))
        return false;
    
    std::lock_guard<std::mutex> lock(mtx);
    if (!ready.load(std::memory_order_relaxed))
        return false;
    
    params = parameters;
    fitness = storedFitness;
    ready.store(false, std::memory_order_release);
    return true;
}

void ParameterBridge::clear()
{
    std::lock_guard<std::mutex> lock(mtx);
    ready.store(false, std::memory_order_release);
}
