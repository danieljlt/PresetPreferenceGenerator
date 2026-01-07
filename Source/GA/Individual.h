/*
  ==============================================================================
    Individual.h
    Created: 1 Oct 2025
    Author:  Daniel Lister

    Simple individual class for genetic algorithm population.
    Stores parameters and fitness with variable parameter count support.
  ==============================================================================
*/

#pragma once

#include <vector>

class Individual
{
public:
    // Construction
    Individual();
    Individual(int parameterCount);
    Individual(const std::vector<float>& parameters);
    
    // Parameter management
    void setParameterCount(int count);
    int getParameterCount() const { return static_cast<int>(parameters.size()); }
    
    void setParameter(int index, float value);
    float getParameter(int index) const;
    
    void setParameters(const std::vector<float>& params);
    const std::vector<float>& getParameters() const { return parameters; }
    std::vector<float>& getParameters() { return parameters; }
    
    // Fitness management
    void setFitness(float fitness) { this->fitness = fitness; isEvaluated = true; }
    float getFitness() const { return fitness; }
    bool hasBeenEvaluated() const { return isEvaluated; }
    void invalidateFitness() { isEvaluated = false; }

private:
    std::vector<float> parameters;
    float fitness = 0.0f;
    bool isEvaluated = false;
};
