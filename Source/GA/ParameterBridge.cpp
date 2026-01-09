/*
  ==============================================================================
    ParameterBridge.cpp
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Implementation of lock-free parameter communication bridge.
  ==============================================================================
*/

#include "ParameterBridge.h"
#include <iostream>

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

void ParameterBridge::debugLogQueueContents() const
{
    juce::String outputBuffer;
    juce::String nl = juce::NewLine::getDefault();
    
    int numReady = fifo.getNumReady();
    if (numReady == 0)
    {
        outputBuffer = "Queue is empty." + nl;
        DBG(outputBuffer);
        return;
    }
    
    // We need to const_cast the fifo to use prepareToRead, but we won't consume.
    auto& nonConstFifo = const_cast<juce::AbstractFifo&>(fifo);
    
    int start1, size1, start2, size2;
    nonConstFifo.prepareToRead(numReady, start1, size1, start2, size2);
    
    outputBuffer << "--- Queue Dump (" << numReady << " items) ---" << nl;
    
    auto logItem = [&](int index)
    {
        const auto& item = buffer[static_cast<size_t>(index)];
        juce::String s;
        s << "Fit: " << juce::String(item.fitness, 4) << " |Params: ";
        // Print first 5 params as signature
        for(int i=0; i<std::min(5, (int)item.parameters.size()); ++i)
            s << juce::String(item.parameters[i], 2) << " ";
        s << nl;
        outputBuffer += s;
    };
    
    if (size1 > 0)
    {
        for (int i = 0; i < size1; ++i)
            logItem(start1 + i);
    }
    if (size2 > 0)
    {
        for (int i = 0; i < size2; ++i)
            logItem(start2 + i);
    }
    
    outputBuffer << "-----------------------------" << nl << nl;
    
    // Log to standard debug output
    DBG(outputBuffer);
}
