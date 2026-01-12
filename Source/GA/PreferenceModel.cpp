/*
  ==============================================================================
    PreferenceModel.cpp
    Created: 12 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "PreferenceModel.h"

PreferenceModel::PreferenceModel()
{
    // Store in the User's Desktop directory for easy access
    datasetFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("feedback_dataset.csv");

    // Initialize file with header if it doesn't exist
    if (!datasetFile.existsAsFile())
    {
        datasetFile.create();
        juce::String header;
        for (int i = 0; i < 17; ++i)
            header += "p" + juce::String(i) + ",";
        header += "rating";
        datasetFile.appendText(header + "\n");
    }
}

PreferenceModel::~PreferenceModel()
{
}

float PreferenceModel::evaluate(const std::vector<float>& genome)
{
    // Return random fitness [0, 1]
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
    
    // Append rating
    line += juce::String(feedback.rating, 1);
    
    // Write to file
    datasetFile.appendText(line + "\n");
    
    DBG("Feedback saved to " << datasetFile.getFullPathName());
}
