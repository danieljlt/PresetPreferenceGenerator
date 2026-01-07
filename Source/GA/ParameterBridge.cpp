/*
  ==============================================================================
    ParameterBridge.cpp
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Implementation of lock-free parameter communication bridge.
  ==============================================================================
*/

#include "ParameterBridge.h"

ParameterBridge::ParameterBridge(int queueCapacity)
    : fifo(queueCapacity), buffer(static_cast<size_t>(queueCapacity))
{
    // Pre-allocate all vectors in the ring buffer to avoid allocations during push/pop
    for (auto& update : buffer)
    {
        update.parameters.resize(17);
    }
}

bool ParameterBridge::push(const std::vector<float>& params, float fitness)
{
    // Check if we have space in the FIFO
    if (fifo.getFreeSpace() < 1)
        return false;  // Queue is full
    
    // Get write index
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);
    
    if (size1 > 0)
    {
        // Copy parameters into the ring buffer
        buffer[static_cast<size_t>(start1)].parameters = params;
        buffer[static_cast<size_t>(start1)].fitness = fitness;
        
        // Commit the write
        fifo.finishedWrite(size1);
        return true;
    }
    
    return false;
}

bool ParameterBridge::pop(std::vector<float>& params, float& fitness)
{
    // Check if we have data available
    if (fifo.getNumReady() < 1)
        return false;  // Queue is empty
    
    // Get read index
    int start1, size1, start2, size2;
    fifo.prepareToRead(1, start1, size1, start2, size2);
    
    if (size1 > 0)
    {
        // Copy parameters from the ring buffer
        params = buffer[static_cast<size_t>(start1)].parameters;
        fitness = buffer[static_cast<size_t>(start1)].fitness;
        
        // Commit the read
        fifo.finishedRead(size1);
        return true;
    }
    
    return false;
}

int ParameterBridge::getNumAvailable() const
{
    return fifo.getNumReady();
}

bool ParameterBridge::isEmpty() const
{
    return fifo.getNumReady() == 0;
}

void ParameterBridge::clear()
{
    // Reset the FIFO to empty state
    fifo.reset();
}

