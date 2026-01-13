/*
  ==============================================================================
    PreferenceModel.cpp
    Created: 12 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "PreferenceModel.h"

PreferenceModel::PreferenceModel(const std::vector<juce::String>& names)
    : parameterNames(names)
{
    // Store in the User's Desktop directory for easy access
    datasetFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("feedback_dataset.csv");

    // Initialize or validate existing file
    validateSchema();
}

juce::String PreferenceModel::getHeaderString() const
{
    juce::String header;
    for (const auto& name : parameterNames)
        header += name + ",";
    header += "rating,playTimeSeconds";
    return header;
}

void PreferenceModel::validateSchema()
{
    const juce::ScopedLock lock(fileLock);
    
    bool shouldCreateNew = false;
    juce::String currentHeader = getHeaderString();
    
    if (datasetFile.existsAsFile())
    {
        // Read first line to check if headers match
        juce::StringArray lines;
        datasetFile.readLines(lines);
        
        if (lines.size() > 0)
        {
            juce::String fileHeader = lines[0];
            
            // Allow for potential whitespace differences or newline chars
            if (fileHeader.trim() != currentHeader.trim())
            {
                // Schema mismatch! Rotate file
                juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
                juce::File backupFile = datasetFile.getSiblingFile("feedback_dataset_backup_" + timestamp + ".csv");
                datasetFile.moveFileTo(backupFile);
                
                DBG("Schema changed. Rotated old dataset to " << backupFile.getFileName());
                shouldCreateNew = true;
            }
        }
    }
    else
    {
        shouldCreateNew = true;
    }
    
    if (shouldCreateNew)
    {
        datasetFile.create();
        datasetFile.appendText(currentHeader + "\n");
    }
}

PreferenceModel::~PreferenceModel()
{
}

float PreferenceModel::evaluate(const std::vector<float>& genome)
{
    // rng is not thread-safe; protect with lock
    const juce::ScopedLock lock(rngLock);
    return rng.nextFloat();
}

void PreferenceModel::sendFeedback(const std::vector<float>& genome, const Feedback& feedback)
{
    const juce::ScopedLock lock(fileLock);

    juce::String line;
    
    // Append parameters
    for (float param : genome)
    {
        line += juce::String(param, 6) + ",";
    }
    
    // Append rating and play time
    line += juce::String(feedback.rating, 1) + ",";
    line += juce::String(feedback.playTimeSeconds, 2);
    
    // Write to file
    datasetFile.appendText(line + "\n");
    
    DBG("Feedback saved to " << datasetFile.getFullPathName());
}
