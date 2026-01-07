/*
  ==============================================================================
    Individual.cpp
    Created: 1 Oct 2025
    Author:  Daniel Lister

    Simple implementation of Individual class for genetic algorithm.
  ==============================================================================
*/

#include "Individual.h"

Individual::Individual()
{
}

Individual::Individual(int parameterCount) : parameters(parameterCount, 0.0f)
{
}

Individual::Individual(const std::vector<float>& parameters) : parameters(parameters)
{
}

void Individual::setParameterCount(int count)
{
    parameters.resize(count, 0.0f);
    isEvaluated = false; // Parameters changed, fitness no longer valid
}

void Individual::setParameter(int index, float value)
{
    if (index >= 0 && index < static_cast<int>(parameters.size()))
    {
        parameters[index] = value;
        isEvaluated = false; // Parameter changed, fitness no longer valid
    }
}

float Individual::getParameter(int index) const
{
    if (index >= 0 && index < static_cast<int>(parameters.size()))
        return parameters[index];
    return 0.0f;
}

void Individual::setParameters(const std::vector<float>& params)
{
    parameters = params;
    isEvaluated = false; // Parameters changed, fitness no longer valid
}
