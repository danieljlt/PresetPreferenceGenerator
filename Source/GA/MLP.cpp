/*
  ==============================================================================
    MLP.cpp
    Created: 14 Jan 2026
    Author:  Daniel Lister
  ==============================================================================
*/

#include "MLP.h"

MLP::MLP()
    : weightsIH(INPUT_SIZE * HIDDEN_SIZE)
    , biasH(HIDDEN_SIZE, 0.0f)
    , weightsHO(HIDDEN_SIZE)
    , biasO(0.0f)
    , mIH(INPUT_SIZE * HIDDEN_SIZE, 0.0f)
    , vIH(INPUT_SIZE * HIDDEN_SIZE, 0.0f)
    , mBiasH(HIDDEN_SIZE, 0.0f)
    , vBiasH(HIDDEN_SIZE, 0.0f)
    , mHO(HIDDEN_SIZE, 0.0f)
    , vHO(HIDDEN_SIZE, 0.0f)
    , mBiasO(0.0f)
    , vBiasO(0.0f)
    , timestep(0)
    , zHidden(HIDDEN_SIZE)
    , aHidden(HIDDEN_SIZE)
    , zOutput(0.0f)
    , aOutput(0.5f)
{
    initializeWeights();
}

void MLP::initializeWeights()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Xavier initialization for input->hidden
    float scaleIH = std::sqrt(2.0f / (INPUT_SIZE + HIDDEN_SIZE));
    std::uniform_real_distribution<float> distIH(-scaleIH, scaleIH);
    for (float& w : weightsIH)
        w = distIH(gen);
    
    // Neutral initialization for output layer (all zeros)
    // This ensures initial predictions are exactly 0.5 (sigmoid(0))
    // preventing the GA from optimizing on random noise before user feedback.
    std::fill(weightsHO.begin(), weightsHO.end(), 0.0f);
    
    // Biases start at zero
    std::fill(biasH.begin(), biasH.end(), 0.0f);
    biasO = 0.0f;
}

float MLP::predict(const std::vector<float>& input)
{
    // Input -> Hidden (with ReLU)
    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        float sum = biasH[j];
        for (int i = 0; i < INPUT_SIZE; ++i)
            sum += input[i] * weightsIH[i * HIDDEN_SIZE + j];
        
        zHidden[j] = sum;
        aHidden[j] = relu(sum);
    }
    
    // Hidden -> Output (with sigmoid)
    float sum = biasO;
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        sum += aHidden[j] * weightsHO[j];
    
    zOutput = sum;
    aOutput = sigmoid(sum);
    
    return aOutput;
}

void MLP::train(const std::vector<float>& input, float target, 
                float learningRate, float sampleWeight)
{
    predict(input);
    ++timestep;
    
    float dOutput = (aOutput - target) * sampleWeight;
    
    // Gradient clipping
    if (dOutput > gradClipThreshold)
        dOutput = gradClipThreshold;
    else if (dOutput < -gradClipThreshold)
        dOutput = -gradClipThreshold;
    
    float bc1 = 1.0f - std::pow(beta1, timestep);
    float bc2 = 1.0f - std::pow(beta2, timestep);
    
    // Backprop to hidden layer (compute before updating weightsHO)
    std::vector<float> dHidden(HIDDEN_SIZE);
    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        float reluGrad = (zHidden[j] > 0.0f) ? 1.0f : 0.0f;
        dHidden[j] = dOutput * weightsHO[j] * reluGrad;
    }
    
    // Update hidden to output weights with Adam
    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        float grad = dOutput * aHidden[j];
        mHO[j] = beta1 * mHO[j] + (1.0f - beta1) * grad;
        vHO[j] = beta2 * vHO[j] + (1.0f - beta2) * grad * grad;
        float mHat = mHO[j] / bc1;
        float vHat = vHO[j] / bc2;
        weightsHO[j] -= learningRate * (mHat / (std::sqrt(vHat) + epsilon) + weightDecay * weightsHO[j]);
    }
    
    // Update output bias with Adam
    mBiasO = beta1 * mBiasO + (1.0f - beta1) * dOutput;
    vBiasO = beta2 * vBiasO + (1.0f - beta2) * dOutput * dOutput;
    float mHatBiasO = mBiasO / bc1;
    float vHatBiasO = vBiasO / bc2;
    biasO -= learningRate * mHatBiasO / (std::sqrt(vHatBiasO) + epsilon);
    
    // Update input to hidden weights with Adam
    for (int j = 0; j < HIDDEN_SIZE; ++j)
    {
        for (int i = 0; i < INPUT_SIZE; ++i)
        {
            int idx = i * HIDDEN_SIZE + j;
            float grad = dHidden[j] * input[i];
            mIH[idx] = beta1 * mIH[idx] + (1.0f - beta1) * grad;
            vIH[idx] = beta2 * vIH[idx] + (1.0f - beta2) * grad * grad;
            float mHat = mIH[idx] / bc1;
            float vHat = vIH[idx] / bc2;
            weightsIH[idx] -= learningRate * (mHat / (std::sqrt(vHat) + epsilon) + weightDecay * weightsIH[idx]);
        }
        
        // Update hidden bias with Adam
        mBiasH[j] = beta1 * mBiasH[j] + (1.0f - beta1) * dHidden[j];
        vBiasH[j] = beta2 * vBiasH[j] + (1.0f - beta2) * dHidden[j] * dHidden[j];
        float mHatB = mBiasH[j] / bc1;
        float vHatB = vBiasH[j] / bc2;
        biasH[j] -= learningRate * mHatB / (std::sqrt(vHatB) + epsilon);
    }
}

int MLP::getWeightCount()
{
    // Weights + biases + Adam moments (m and v for each) + timestep
    int baseCount = (INPUT_SIZE * HIDDEN_SIZE) + HIDDEN_SIZE + HIDDEN_SIZE + 1;
    int adamCount = 2 * ((INPUT_SIZE * HIDDEN_SIZE) + HIDDEN_SIZE + HIDDEN_SIZE + 1);
    return baseCount + adamCount + 1;
}

std::vector<float> MLP::getWeights() const
{
    std::vector<float> weights;
    weights.reserve(getWeightCount());
    
    weights.insert(weights.end(), weightsIH.begin(), weightsIH.end());
    weights.insert(weights.end(), biasH.begin(), biasH.end());
    weights.insert(weights.end(), weightsHO.begin(), weightsHO.end());
    weights.push_back(biasO);
    
    // Adam first moments
    weights.insert(weights.end(), mIH.begin(), mIH.end());
    weights.insert(weights.end(), mBiasH.begin(), mBiasH.end());
    weights.insert(weights.end(), mHO.begin(), mHO.end());
    weights.push_back(mBiasO);
    
    // Adam second moments
    weights.insert(weights.end(), vIH.begin(), vIH.end());
    weights.insert(weights.end(), vBiasH.begin(), vBiasH.end());
    weights.insert(weights.end(), vHO.begin(), vHO.end());
    weights.push_back(vBiasO);
    
    weights.push_back(static_cast<float>(timestep));
    
    return weights;
}

bool MLP::setWeights(const std::vector<float>& weights)
{
    if (static_cast<int>(weights.size()) != getWeightCount())
        return false;
    
    int idx = 0;
    
    for (int i = 0; i < INPUT_SIZE * HIDDEN_SIZE; ++i)
        weightsIH[i] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        biasH[j] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        weightsHO[j] = weights[idx++];
    
    biasO = weights[idx++];
    
    // Adam first moments
    for (int i = 0; i < INPUT_SIZE * HIDDEN_SIZE; ++i)
        mIH[i] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        mBiasH[j] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        mHO[j] = weights[idx++];
    
    mBiasO = weights[idx++];
    
    // Adam second moments
    for (int i = 0; i < INPUT_SIZE * HIDDEN_SIZE; ++i)
        vIH[i] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        vBiasH[j] = weights[idx++];
    
    for (int j = 0; j < HIDDEN_SIZE; ++j)
        vHO[j] = weights[idx++];
    
    vBiasO = weights[idx++];
    
    timestep = static_cast<int>(weights[idx++]);
    
    return true;
}
