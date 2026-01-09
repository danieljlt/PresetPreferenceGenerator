/*
  ==============================================================================
    IFitnessModel.h
    Created: 8 Jan 2026
    Author:  Daniel Lister

    Interface for fitness evaluation models (Dummy, MLP, etc).
  ==============================================================================
*/

#pragma once

#include <vector>

class IFitnessModel
{
public:
    virtual ~IFitnessModel() = default;

    /**
     * Evaluates a genome and returns a fitness score.
     * @param genome The parameter vector to evaluate.
     * @return Fitness value (typically 0.0 to 1.0).
     */
    virtual float evaluate(const std::vector<float>& genome) = 0;

    /**
     * Trigger feedback mechanism (e.g. logging, model training).
     * Currently a placeholder for future MLP integration.
     */
    virtual void sendFeedback() = 0;
};
