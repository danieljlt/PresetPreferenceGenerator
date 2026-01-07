/*
  ==============================================================================
    FitnessEvaluator.h
    Created: 7 Oct 2025
    Author:  Daniel Lister

    Computes fitness by comparing feature vectors using normalized Euclidean
    distance converted to similarity score. Higher fitness = better match.
    
    Thread-safe for parallel evaluation (trivially copyable, const methods).
  ==============================================================================
*/

#pragma once

#include "FeatureExtractor.h"

class FitnessEvaluator
{
public:
    // Construct with target feature vector to match
    explicit FitnessEvaluator(const FeatureVector& target);
    
    // Compute similarity score between candidate and target
    // Higher values = better match (1.0 = perfect match, approaches 0.0 for poor matches)
    // Thread-safe: can be called concurrently from multiple threads
    float computeFitness(const FeatureVector& candidate) const;
    
    // Update target for interactive evolution
    void setTarget(const FeatureVector& newTarget);
    
    // Access current target
    const FeatureVector& getTarget() const { return targetFeatures; }

private:
    FeatureVector targetFeatures;
    
    // Normalization scales for each feature type
    // These ensure equal weighting despite different value ranges
    static constexpr float MFCC_SCALE = 15.0f;           // Log-compressed mel energies
    static constexpr float CENTROID_SCALE = 5000.0f;     // Hz (typical musical range)
    static constexpr float ATTACK_SCALE = 0.5f;          // Seconds (typical envelope)
    static constexpr float RMS_SCALE = 1.0f;             // Normalized amplitude
    
    // Helper: compute squared normalized difference for a single value
    static float squaredNormalizedDiff(float candidate, float target, float scale);
};

