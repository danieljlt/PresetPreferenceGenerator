/*
  ==============================================================================
    MLP.h
    Created: 14 Jan 2026
    Author:  Daniel Lister

    Single hidden layer neural network for preference learning.
    Runtime-configurable input size for genome (17) or audio features (24).
  ==============================================================================
*/

#pragma once

#include <vector>
#include <cmath>
#include <random>

class MLP
{
public:
    explicit MLP(int inputSize = 17, int hiddenSize = 32);
    
    int getInputSize() const { return inputSize; }
    int getHiddenSize() const { return hiddenSize; }
    
    /**
     * Forward pass: predict preference score [0, 1].
     */
    float predict(const std::vector<float>& input);
    
    /**
     * Train on a single sample using SGD with binary cross-entropy loss.
     * @param input The input vector (normalized [0,1])
     * @param target 1.0 for Like, 0.0 for Dislike
     * @param learningRate Step size for weight updates
     * @param sampleWeight Weight for this sample (based on play time)
     */
    void train(const std::vector<float>& input, float target, 
               float learningRate = 0.05f, float sampleWeight = 1.0f);
    
    std::vector<float> getWeights() const;
    
    // Returns true if size matches, false otherwise
    bool setWeights(const std::vector<float>& weights);
    
    int getWeightCount() const;

private:
    int inputSize;
    int hiddenSize;
    
    // Weights and biases
    std::vector<float> weightsIH;  // Input to Hidden
    std::vector<float> biasH;
    std::vector<float> weightsHO;  // Hidden to Output
    float biasO;
    
    // Adam optimizer state (first and second moment estimates)
    std::vector<float> mIH, vIH;
    std::vector<float> mBiasH, vBiasH;
    std::vector<float> mHO, vHO;
    float mBiasO, vBiasO;
    int timestep;
    
    // Adam hyperparameters
    static constexpr float beta1 = 0.9f;
    static constexpr float beta2 = 0.999f;
    static constexpr float epsilon = 1e-8f;
    static constexpr float weightDecay = 1e-4f;
    static constexpr float gradClipThreshold = 1.0f;
    
    // Cached values for backprop
    std::vector<float> zHidden;
    std::vector<float> aHidden;
    float zOutput;
    float aOutput;
    
    void initializeWeights();
    
    static float relu(float x) { return x > 0.0f ? x : 0.0f; }
    static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
};
