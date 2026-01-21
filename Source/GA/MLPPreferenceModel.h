/*
  ==============================================================================
    MLPPreferenceModel.h
    Created: 14 Jan 2026
    Author:  Daniel Lister

    IFitnessModel implementation using MLP for preference learning.
    Handles online training, weight persistence, and CSV logging.
  ==============================================================================
*/

#pragma once

#include "IFitnessModel.h"
#include "MLP.h"
#include <juce_core/juce_core.h>
#include <mutex>

class MLPPreferenceModel : public IFitnessModel
{
public:
    MLPPreferenceModel(const std::vector<juce::String>& parameterNames,
                       const juce::File& baseDirectory = juce::File());
    ~MLPPreferenceModel() override;

    float evaluate(const std::vector<float>& genome) override;

    /**
     * Online training: update MLP, log to CSV, save weights.
     */
    void sendFeedback(const std::vector<float>& genome, const Feedback& feedback) override;
    
    void setConfigFlags(const juce::String& flags) { configFlags = flags; }

private:
    MLP mlp;
    mutable std::mutex mlpMutex;
    
    // Replay buffer (ring buffer)
    static constexpr size_t maxBufferSize = 64;
    std::vector<std::pair<std::vector<float>, Feedback>> replayBuffer;
    size_t bufferIndex = 0;
    
    // CSV logging
    juce::File datasetFile;
    const std::vector<juce::String> parameterNames;
    juce::CriticalSection fileLock;
    
    // Weight persistence
    juce::File weightsFile;
    juce::File baseDir;
    
    static constexpr float learningRate = 0.001f;
    static constexpr int replayBatchSize = 8;
    
    size_t sampleCount = 0;
    juce::String configFlags = "baseline";
    
    void loadWeights();
    void saveWeights();
    void initCSV();
    void appendToCSV(const std::vector<float>& genome, const Feedback& feedback,
                     float mlpPrediction, size_t sampleIndex);
    juce::String getHeaderString() const;
    void replayTrain();
};

