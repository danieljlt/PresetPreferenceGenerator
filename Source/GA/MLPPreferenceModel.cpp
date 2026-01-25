/*
  ==============================================================================
    MLPPreferenceModel.cpp
    Created: 14 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "MLPPreferenceModel.h"
#include <algorithm>
#include <random>

MLPPreferenceModel::MLPPreferenceModel(const std::vector<juce::String>& names,
                                       const juce::File& baseDirectory,
                                       double sampleRate)
    : juce::Thread("MLPTraining")
    , parameterNames(names)
{
    if (baseDirectory.isDirectory())
        baseDir = baseDirectory;
    else
    {
        baseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                      .getChildFile("Library/Application Support/PresetPreferenceGenerator");
        baseDir.createDirectory();
    }
    
    datasetFile = baseDir.getChildFile("feedback_dataset.csv");
    weightsFileGenome = baseDir.getChildFile("mlp_weights_genome.bin");
    weightsFileAudio = baseDir.getChildFile("mlp_weights_audio.bin");
    
    audioFeatureCache = std::make_unique<AudioFeatureCache>(sampleRate);
    
    loadWeights();
    initCSV();
    
    // Start training thread
    startThread(juce::Thread::Priority::low);
}

MLPPreferenceModel::~MLPPreferenceModel()
{
    // Stop training thread first, before destroying any members
    signalThreadShouldExit();
    queueEvent.signal();
    stopThread(2000);
    
    // Final save
    saveWeights();
}

float MLPPreferenceModel::evaluate(const std::vector<float>& genome)
{
    std::lock_guard<std::mutex> lock(mlpMutex);
    
    float genomePred = mlpGenome.predict(genome);
    lastGenomePrediction.store(genomePred);
    
    auto features = audioFeatureCache->getFeatures(genome);
    float audioPred = mlpAudio.predict(features);
    lastAudioPrediction.store(audioPred);
    
    return (inputMode == InputMode::Audio) ? audioPred : genomePred;
}

void MLPPreferenceModel::sendFeedback(const std::vector<float>& genome, const Feedback& feedback)
{
    size_t index = ++sampleCount;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        feedbackQueue.push_back({genome, feedback, index});
    }
    queueEvent.signal();
}

void MLPPreferenceModel::setSampleRate(double newSampleRate)
{
    std::lock_guard<std::mutex> lock(mlpMutex);
    audioFeatureCache->setSampleRate(newSampleRate);
}

void MLPPreferenceModel::run()
{
    while (!threadShouldExit())
    {
        queueEvent.wait(100);
        
        QueuedFeedback item;
        bool hasItem = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!feedbackQueue.empty())
            {
                item = std::move(feedbackQueue.front());
                feedbackQueue.pop_front();
                hasItem = true;
            }
        }
        
        if (hasItem)
        {
            processQueuedFeedback(item);
        }
    }
}

void MLPPreferenceModel::processQueuedFeedback(const QueuedFeedback& item)
{
    const auto& genome = item.genome;
    const auto& feedback = item.feedback;
    
    float genomePrediction, audioPrediction;
    std::vector<float> features;
    
    {
        std::lock_guard<std::mutex> lock(mlpMutex);
        
        // Get predictions before training
        genomePrediction = mlpGenome.predict(genome);
        features = audioFeatureCache->getFeatures(genome);
        audioPrediction = mlpAudio.predict(features);
        
        // Train both MLPs
        mlpGenome.train(genome, feedback.rating, learningRate, feedback.sampleWeight);
        mlpAudio.train(features, feedback.rating, learningRate, feedback.sampleWeight);
    }
    
    // Add to replay buffer (only accessed by this thread)
    if (replayBuffer.size() < maxBufferSize)
        replayBuffer.push_back({genome, feedback});
    else
    {
        replayBuffer[bufferIndex] = {genome, feedback};
        bufferIndex = (bufferIndex + 1) % maxBufferSize;
    }
    
    // Replay training
    {
        std::lock_guard<std::mutex> lock(mlpMutex);
        replayTrain();
    }
    
    // Append to CSV
    appendToCSV(genome, feedback, genomePrediction, audioPrediction, item.sampleIndex);
    
    // Debounced weight saving
    if (item.sampleIndex - lastSaveCount >= saveDebounceCount)
    {
        std::lock_guard<std::mutex> lock(mlpMutex);
        saveWeights();
        lastSaveCount = item.sampleIndex;
    }
}

void MLPPreferenceModel::loadWeights()
{
    if (weightsFileGenome.existsAsFile())
    {
        juce::FileInputStream stream(weightsFileGenome);
        if (stream.openedOk())
        {
            uint32_t count = 0;
            stream.read(&count, sizeof(count));
            
            if (static_cast<int>(count) == mlpGenome.getWeightCount())
            {
                std::vector<float> weights(count);
                stream.read(weights.data(), count * sizeof(float));
                if (mlpGenome.setWeights(weights))
                    DBG("Loaded genome MLP weights");
            }
        }
    }
    
    if (weightsFileAudio.existsAsFile())
    {
        juce::FileInputStream stream(weightsFileAudio);
        if (stream.openedOk())
        {
            uint32_t count = 0;
            stream.read(&count, sizeof(count));
            
            if (static_cast<int>(count) == mlpAudio.getWeightCount())
            {
                std::vector<float> weights(count);
                stream.read(weights.data(), count * sizeof(float));
                if (mlpAudio.setWeights(weights))
                    DBG("Loaded audio MLP weights");
            }
        }
    }
}

void MLPPreferenceModel::saveWeights()
{
    {
        juce::FileOutputStream stream(weightsFileGenome);
        if (stream.openedOk())
        {
            stream.setPosition(0);
            stream.truncate();
            
            auto weights = mlpGenome.getWeights();
            uint32_t count = static_cast<uint32_t>(weights.size());
            
            stream.write(&count, sizeof(count));
            stream.write(weights.data(), count * sizeof(float));
            stream.flush();
        }
    }
    
    {
        juce::FileOutputStream stream(weightsFileAudio);
        if (stream.openedOk())
        {
            stream.setPosition(0);
            stream.truncate();
            
            auto weights = mlpAudio.getWeights();
            uint32_t count = static_cast<uint32_t>(weights.size());
            
            stream.write(&count, sizeof(count));
            stream.write(weights.data(), count * sizeof(float));
            stream.flush();
        }
    }
}

void MLPPreferenceModel::initCSV()
{
    const juce::ScopedLock lock(fileLock);
    
    if (datasetFile.existsAsFile())
    {
        juce::StringArray lines;
        datasetFile.readLines(lines);
        
        if (lines.size() > 0 && lines[0].trim() == getHeaderString().trim())
            return;
        
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::File backupFile = datasetFile.getSiblingFile("feedback_dataset_backup_" + timestamp + ".csv");
        datasetFile.moveFileTo(backupFile);
        DBG("Schema changed. Rotated old dataset to " << backupFile.getFileName());
    }
    
    datasetFile.create();
    datasetFile.appendText(getHeaderString() + "\n");
}

juce::String MLPPreferenceModel::getHeaderString() const
{
    juce::String header;
    for (const auto& name : parameterNames)
        header += name + ",";
    header += "rating,playTimeSeconds,sampleIndex,mlpGenomePrediction,mlpAudioPrediction,configFlags,timestamp,sampleWeight";
    return header;
}

void MLPPreferenceModel::appendToCSV(const std::vector<float>& genome, const Feedback& feedback,
                                      float genomePrediction, float audioPrediction, size_t sampleIndex)
{
    const juce::ScopedLock lock(fileLock);
    
    juce::String line;
    for (float param : genome)
        line += juce::String(param, 6) + ",";
    
    line += juce::String(feedback.rating, 1) + ",";
    line += juce::String(feedback.playTimeSeconds, 2) + ",";
    line += juce::String(static_cast<int>(sampleIndex)) + ",";
    line += juce::String(genomePrediction, 6) + ",";
    line += juce::String(audioPrediction, 6) + ",";
    line += configFlags + ",";
    line += juce::Time::getCurrentTime().toISO8601(true) + ",";
    line += juce::String(feedback.sampleWeight, 2);
    
    datasetFile.appendText(line + "\n");
}

void MLPPreferenceModel::replayTrain()
{
    if (replayBuffer.empty())
        return;
    
    int numSamples = std::min(static_cast<int>(replayBuffer.size()), replayBatchSize);
    
    std::vector<size_t> indices(replayBuffer.size());
    for (size_t i = 0; i < indices.size(); ++i)
        indices[i] = i;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);
    
    for (int i = 0; i < numSamples; ++i)
    {
        const auto& sample = replayBuffer[indices[i]];
        const auto& genome = sample.first;
        float rating = sample.second.rating;
        float weight = sample.second.sampleWeight;
        
        mlpGenome.train(genome, rating, learningRate, weight);
        
        // Only train audio MLP if features are cached (avoid blocking)
        if (audioFeatureCache->hasCached(genome))
        {
            auto features = audioFeatureCache->getFeatures(genome);
            mlpAudio.train(features, rating, learningRate, weight);
        }
    }
}

