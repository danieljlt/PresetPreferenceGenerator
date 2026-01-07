/*
  ==============================================================================
    FitnessEvaluator.cpp
    Created: 7 Oct 2025
    Author:  Daniel Lister
  ==============================================================================
*/

#include "FitnessEvaluator.h"
#include <cmath>

FitnessEvaluator::FitnessEvaluator(const FeatureVector& target)
    : targetFeatures(target)
{
}

void FitnessEvaluator::setTarget(const FeatureVector& newTarget)
{
    targetFeatures = newTarget;
}

float FitnessEvaluator::computeFitness(const FeatureVector& candidate) const
{
    // Compute normalized Euclidean distance
    // Each feature type is normalized by its typical range for equal weighting
    
    float sumSquaredDiffs = 0.0f;
    
    // MFCC mean (10 coefficients) - captures average timbre
    for (int i = 0; i < 10; ++i)
    {
        sumSquaredDiffs += squaredNormalizedDiff(
            candidate.mfccMean[i], 
            targetFeatures.mfccMean[i], 
            MFCC_SCALE
        );
    }
    
    // MFCC std dev (10 coefficients) - captures timbre variation (modulation)
    for (int i = 0; i < 10; ++i)
    {
        sumSquaredDiffs += squaredNormalizedDiff(
            candidate.mfccStd[i], 
            targetFeatures.mfccStd[i], 
            MFCC_SCALE
        );
    }
    
    // Spectral centroid mean - average brightness
    sumSquaredDiffs += squaredNormalizedDiff(
        candidate.spectralCentroidMean,
        targetFeatures.spectralCentroidMean,
        CENTROID_SCALE
    );
    
    // Spectral centroid std dev - brightness variation (filter sweeps, LFO)
    sumSquaredDiffs += squaredNormalizedDiff(
        candidate.spectralCentroidStd,
        targetFeatures.spectralCentroidStd,
        CENTROID_SCALE
    );
    
    // Attack time - envelope characteristic
    sumSquaredDiffs += squaredNormalizedDiff(
        candidate.attackTime,
        targetFeatures.attackTime,
        ATTACK_SCALE
    );
    
    // RMS energy - overall power
    sumSquaredDiffs += squaredNormalizedDiff(
        candidate.rmsEnergy,
        targetFeatures.rmsEnergy,
        RMS_SCALE
    );
    
    // Convert distance to similarity score (higher = better match)
    // Perfect match (distance = 0) -> fitness = 1.0
    // Poor match (large distance) -> fitness approaches 0.0
    float distance = std::sqrt(sumSquaredDiffs);
    return 1.0f / (1.0f + distance);
}

float FitnessEvaluator::squaredNormalizedDiff(float candidate, float target, float scale)
{
    float normalizedDiff = (candidate - target) / scale;
    return normalizedDiff * normalizedDiff;
}

