/*
  ==============================================================================
    DummyFitnessModel.cpp
    Created: 8 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "DummyFitnessModel.h"

DummyFitnessModel::DummyFitnessModel(int seed)
    : rng(seed == 0 ? juce::Time::currentTimeMillis() : seed)
{
}

float DummyFitnessModel::evaluate(const std::vector<float>& /*genome*/)
{
    // Return a random value between 0.0 and 1.0
    return rng.nextFloat();
}

void DummyFitnessModel::sendFeedback()
{
    // Placeholder for now
}
