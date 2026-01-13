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
     * Structure to hold user feedback.
     */
    struct Feedback
    {
        float rating;          // 0.0 (Dislike) or 1.0 (Like)
        float playTimeSeconds; // Duration user played current preset before feedback
    };

    /**
     * Evaluates a genome and returns a fitness score.
     * @param genome The parameter vector to evaluate.
     * @return Fitness value (typically 0.0 to 1.0).
     */
    virtual float evaluate(const std::vector<float>& genome) = 0;

    /**
     * Trigger feedback mechanism.
     * @param genome The genome the feedback applies to.
     * @param feedback The feedback data.
     */
    virtual void sendFeedback(const std::vector<float>& genome, const Feedback& feedback) = 0;
};
