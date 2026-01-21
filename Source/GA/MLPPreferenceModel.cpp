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
                                       const juce::File& baseDirectory)
    : parameterNames(names)
{
    // Use provided directory or fall back to Application Support
    if (baseDirectory.isDirectory())
        baseDir = baseDirectory;
    else
    {
        baseDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                      .getChildFile("Library/Application Support/PresetPreferenceGenerator");
        baseDir.createDirectory();
    }
    
    datasetFile = baseDir.getChildFile("feedback_dataset.csv");
    weightsFile = baseDir.getChildFile("mlp_weights.bin");
    
    // Try to load existing weights, otherwise MLP stays with random init
    loadWeights();
    
    // Initialize CSV file
    initCSV();
}

MLPPreferenceModel::~MLPPreferenceModel()
{
    // Save weights on destruction as safety net
    saveWeights();
}

float MLPPreferenceModel::evaluate(const std::vector<float>& genome)
{
    std::lock_guard<std::mutex> lock(mlpMutex);
    return mlp.predict(genome);
}

void MLPPreferenceModel::sendFeedback(const std::vector<float>& genome, const Feedback& feedback)
{
    ++sampleCount;
    
    float prediction;
    {
        std::lock_guard<std::mutex> lock(mlpMutex);
        prediction = mlp.predict(genome);
    }
    
    // Add to replay buffer
    if (replayBuffer.size() < maxBufferSize)
        replayBuffer.push_back({genome, feedback});
    else
    {
        replayBuffer[bufferIndex] = {genome, feedback};
        bufferIndex = (bufferIndex + 1) % maxBufferSize;
    }
    
    {
        std::lock_guard<std::mutex> lock(mlpMutex);
        mlp.train(genome, feedback.rating, learningRate);
        replayTrain();
    }
    
    saveWeights();
    appendToCSV(genome, feedback, prediction, sampleCount);
}

void MLPPreferenceModel::loadWeights()
{
    if (!weightsFile.existsAsFile())
    {
        DBG("No weights file found, starting with random initialization");
        return;
    }
    
    juce::FileInputStream stream(weightsFile);
    if (!stream.openedOk())
    {
        DBG("Failed to open weights file");
        return;
    }
    
    // Read weight count
    uint32_t count = 0;
    stream.read(&count, sizeof(count));
    
    if (static_cast<int>(count) != MLP::getWeightCount())
    {
        DBG("Weight count mismatch, starting fresh");
        return;
    }
    
    // Read weights
    std::vector<float> weights(count);
    stream.read(weights.data(), count * sizeof(float));
    
    if (mlp.setWeights(weights))
        DBG("Loaded MLP weights from " << weightsFile.getFullPathName());
    else
        DBG("Failed to set weights, using random initialization");
}

void MLPPreferenceModel::saveWeights()
{
    juce::FileOutputStream stream(weightsFile);
    if (!stream.openedOk())
    {
        DBG("Failed to open weights file for writing");
        return;
    }
    
    stream.setPosition(0);
    stream.truncate();
    
    auto weights = mlp.getWeights();
    uint32_t count = static_cast<uint32_t>(weights.size());
    
    stream.write(&count, sizeof(count));
    stream.write(weights.data(), count * sizeof(float));
    
    stream.flush();
}

void MLPPreferenceModel::initCSV()
{
    const juce::ScopedLock lock(fileLock);
    
    if (datasetFile.existsAsFile())
    {
        // Check if header matches
        juce::StringArray lines;
        datasetFile.readLines(lines);
        
        if (lines.size() > 0 && lines[0].trim() == getHeaderString().trim())
            return;  // Schema matches, keep existing file
        
        // Schema mismatch, rotate file
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::File backupFile = datasetFile.getSiblingFile("feedback_dataset_backup_" + timestamp + ".csv");
        datasetFile.moveFileTo(backupFile);
        DBG("Schema changed. Rotated old dataset to " << backupFile.getFileName());
    }
    
    // Create new file with header
    datasetFile.create();
    datasetFile.appendText(getHeaderString() + "\n");
}

juce::String MLPPreferenceModel::getHeaderString() const
{
    juce::String header;
    for (const auto& name : parameterNames)
        header += name + ",";
    header += "rating,playTimeSeconds,sampleIndex,mlpPrediction,configFlags,timestamp";
    return header;
}

void MLPPreferenceModel::appendToCSV(const std::vector<float>& genome, const Feedback& feedback,
                                      float mlpPrediction, size_t sampleIndex)
{
    const juce::ScopedLock lock(fileLock);
    
    juce::String line;
    for (float param : genome)
        line += juce::String(param, 6) + ",";
    
    line += juce::String(feedback.rating, 1) + ",";
    line += juce::String(feedback.playTimeSeconds, 2) + ",";
    line += juce::String(static_cast<int>(sampleIndex)) + ",";
    line += juce::String(mlpPrediction, 6) + ",";
    line += configFlags + ",";
    line += juce::Time::getCurrentTime().toISO8601(true);
    
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
        mlp.train(sample.first, sample.second.rating, learningRate);
    }
}
