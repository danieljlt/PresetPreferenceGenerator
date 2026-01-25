/*
  ==============================================================================
    MLPPreferenceModel.h
    Created: 14 Jan 2026
    Author:  Daniel Lister

    IFitnessModel implementation using dual MLPs for preference learning.
    Supports both genome-based and audio feature-based prediction.
    Training runs on a background thread for instant UI response.
  ==============================================================================
*/

#pragma once

#include "IFitnessModel.h"
#include "MLP.h"
#include "AudioFeatureCache.h"
#include "GAConfig.h"
#include <juce_core/juce_core.h>
#include <mutex>
#include <deque>
#include <atomic>

class MLPPreferenceModel : public IFitnessModel, private juce::Thread
{
public:
    using InputMode = GAConfig::MLPInputMode;
    
    MLPPreferenceModel(const std::vector<juce::String>& parameterNames,
                       const juce::File& baseDirectory = juce::File(),
                       double sampleRate = 44100.0);
    ~MLPPreferenceModel() override;

    float evaluate(const std::vector<float>& genome) override;

    // Non-blocking: queues feedback for background processing
    void sendFeedback(const std::vector<float>& genome, const Feedback& feedback) override;
    
    // Update sample rate (thread-safe, clears audio cache)
    void setSampleRate(double newSampleRate);
    
    void setConfigFlags(const juce::String& flags) { configFlags = flags; }
    void setInputMode(InputMode mode) { inputMode = mode; }
    InputMode getInputMode() const { return inputMode; }
    
    float getLastGenomePrediction() const { return lastGenomePrediction; }
    float getLastAudioPrediction() const { return lastAudioPrediction; }

private:
    // Training thread run loop
    void run() override;
    
    // Queued feedback for async processing
    struct QueuedFeedback
    {
        std::vector<float> genome;
        Feedback feedback;
        size_t sampleIndex;
    };
    
    std::deque<QueuedFeedback> feedbackQueue;
    std::mutex queueMutex;
    juce::WaitableEvent queueEvent;
    
    // MLPs and cache
    MLP mlpGenome{17, 32};
    MLP mlpAudio{24, 32};
    mutable std::mutex mlpMutex;
    
    std::unique_ptr<AudioFeatureCache> audioFeatureCache;
    InputMode inputMode = InputMode::Genome;
    
    std::atomic<float> lastGenomePrediction{0.5f};
    std::atomic<float> lastAudioPrediction{0.5f};
    
    // Replay buffer - only accessed by training thread
    static constexpr size_t maxBufferSize = 64;
    std::vector<std::pair<std::vector<float>, Feedback>> replayBuffer;
    size_t bufferIndex = 0;
    
    // CSV logging
    juce::File datasetFile;
    const std::vector<juce::String> parameterNames;
    juce::CriticalSection fileLock;
    
    // Weight persistence
    juce::File weightsFileGenome;
    juce::File weightsFileAudio;
    juce::File baseDir;
    
    static constexpr float learningRate = 0.001f;
    static constexpr int replayBatchSize = 8;
    static constexpr int saveDebounceCount = 5;  // Save weights every N samples
    
    std::atomic<size_t> sampleCount{0};
    juce::String configFlags = "baseline";
    size_t lastSaveCount = 0;
    
    void loadWeights();
    void saveWeights();
    void initCSV();
    void appendToCSV(const std::vector<float>& genome, const Feedback& feedback,
                     float genomePrediction, float audioPrediction, size_t sampleIndex);
    juce::String getHeaderString() const;
    void processQueuedFeedback(const QueuedFeedback& item);
    void replayTrain();
};

