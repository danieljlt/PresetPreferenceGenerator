/*
  ==============================================================================
    ParameterBridge.h
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Lock-free parameter queue for sending evolved parameters from GA thread
    to the main synth instance on the message thread.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

class ParameterBridge
{
public:
    // Constructor - sets up the queue with specified capacity
    explicit ParameterBridge(int queueCapacity = 32);
    
    // Push a parameter set from GA thread (producer)
    // Returns true if successfully added, false if queue is full
    bool push(const std::vector<float>& params, float fitness);
    
    // Pop a parameter set on message thread (consumer)
    // Returns true if parameters retrieved, false if queue is empty
    bool pop(std::vector<float>& params, float& fitness);
    
    // Check how many items are available in the queue
    int getNumAvailable() const;
    
    // Check if queue is empty
    bool isEmpty() const;
    
    // Clear all pending updates
    void clear();
    
    // Set/get fitness tolerance (0.02 = allow 2% worse fitness)
    void setFitnessTolerance(float tolerance) { fitnessTolerance.store(tolerance); }
    float getFitnessTolerance() const { return fitnessTolerance.load(); }

private:
    // Parameter update entry
    struct ParameterUpdate
    {
        std::vector<float> parameters;
        float fitness = 0.0f;
        
        ParameterUpdate() : parameters(17), fitness(0.0f) {}
    };
    
    // Lock-free FIFO for thread-safe communication
    juce::AbstractFifo fifo;
    
    // Ring buffer storage
    std::vector<ParameterUpdate> buffer;
    
    // Fitness tolerance for updates
    std::atomic<float> fitnessTolerance { 0.02f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterBridge)
};

