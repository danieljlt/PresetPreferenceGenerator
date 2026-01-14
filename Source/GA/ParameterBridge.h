/*
  ==============================================================================
    ParameterBridge.h
    Created: 8 Oct 2025
    Author:  Daniel Lister

    Mailbox-style parameter bridge for sending evolved parameters from GA thread
    to the main synth instance on the message thread. Always holds only the
    latest "best" preset.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <mutex>

class ParameterBridge
{
public:
    ParameterBridge() = default;
    
    // Push a parameter set from GA thread (producer)
    // Overwrites any existing pending preset
    void push(const std::vector<float>& params, float fitness);
    
    // Pop a parameter set on message thread (consumer)
    // Returns true if a preset was available, false if empty
    bool pop(std::vector<float>& params, float& fitness);
    
    // Check if a preset is ready
    bool hasData() const { return ready.load(std::memory_order_acquire); }
    
    // Clear pending preset
    void clear();

private:
    std::vector<float> parameters;
    float storedFitness = 0.0f;
    
    std::atomic<bool> ready { false };
    std::mutex mtx;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterBridge)
};
