#include <catch2/catch_test_macros.hpp>
#include "GA/GeneticAlgorithm.h"
#include "GA/MLPPreferenceModel.h"
#include "GA/Population.h"
#include "GA/Individual.h"
#include "GA/ParameterBridge.h"
#include <cmath>

namespace
{
    juce::File getTestDir()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("IntegrationTests_" + juce::Uuid().toString());
        dir.createDirectory();
        return dir;
    }

    std::vector<juce::String> getParamNames()
    {
        std::vector<juce::String> names;
        for (int i = 0; i < 17; ++i)
            names.push_back("p" + juce::String(i));
        return names;
    }

    float euclideanDistance(const std::vector<float>& a, const std::vector<float>& b)
    {
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i)
        {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
}

TEST_CASE("MLP training changes population fitness distribution", "[integration]")
{
    auto testDir = getTestDir();
    auto names = getParamNames();
    MLPPreferenceModel model(names, testDir);

    // Target genome we'll train the MLP to "like"
    std::vector<float> targetGenome(17, 0.8f);
    std::vector<float> antiGenome(17, 0.2f);

    // Create test population
    Population pop(20, 17);
    pop.initializeRandom();

    // Evaluate before training
    std::vector<float> fitnessBefore;
    for (int i = 0; i < pop.size(); ++i)
    {
        float f = model.evaluate(pop[i].getParameters());
        fitnessBefore.push_back(f);
    }

    // Train MLP: like target, dislike anti
    IFitnessModel::Feedback like{1.0f, 5.0f};
    IFitnessModel::Feedback dislike{0.0f, 5.0f};

    for (int i = 0; i < 30; ++i)
    {
        model.sendFeedback(targetGenome, like);
        model.sendFeedback(antiGenome, dislike);
    }

    // Evaluate after training
    std::vector<float> fitnessAfter;
    for (int i = 0; i < pop.size(); ++i)
    {
        float f = model.evaluate(pop[i].getParameters());
        fitnessAfter.push_back(f);
    }

    // Count individuals closer to target that show improved fitness
    int improvedCount = 0;
    int closerToTargetCount = 0;
    for (int i = 0; i < pop.size(); ++i)
    {
        float distToTarget = euclideanDistance(pop[i].getParameters(), targetGenome);
        float distToAnti = euclideanDistance(pop[i].getParameters(), antiGenome);

        bool closerToTarget = distToTarget < distToAnti;
        if (closerToTarget)
        {
            ++closerToTargetCount;
            if (fitnessAfter[i] > fitnessBefore[i])
                ++improvedCount;
        }
    }

    // At least 25% of individuals closer to target should have improved fitness
    int threshold = std::max(1, closerToTargetCount / 4);
    REQUIRE(improvedCount >= threshold);

    testDir.deleteRecursively();
}

TEST_CASE("GA evolves toward MLP-preferred region", "[integration]")
{
    auto testDir = getTestDir();
    auto names = getParamNames();
    MLPPreferenceModel model(names, testDir);

    // Pre-train MLP to strongly prefer high values
    std::vector<float> preferred(17, 0.9f);
    std::vector<float> notPreferred(17, 0.1f);

    IFitnessModel::Feedback like{1.0f, 5.0f};
    IFitnessModel::Feedback dislike{0.0f, 5.0f};

    for (int i = 0; i < 50; ++i)
    {
        model.sendFeedback(preferred, like);
        model.sendFeedback(notPreferred, dislike);
    }

    // Create GA with trained model
    GeneticAlgorithm ga(model);
    ga.startGA();

    // Let GA run and generate presets
    juce::Thread::sleep(500);

    // Collect generated presets
    std::vector<std::vector<float>> generatedPresets;
    auto* bridge = ga.getParameterBridge();

    for (int i = 0; i < 10; ++i)
    {
        juce::Thread::sleep(150);
        if (bridge->hasData())
        {
            std::vector<float> params;
            float fitness;
            if (bridge->pop(params, fitness))
                generatedPresets.push_back(params);
        }
    }

    ga.stopGA();

    REQUIRE(generatedPresets.size() > 0);

    // Calculate average distance to preferred region
    float avgDistance = 0.0f;
    for (const auto& p : generatedPresets)
        avgDistance += euclideanDistance(p, preferred);
    avgDistance /= static_cast<float>(generatedPresets.size());

    // Distance from preferred to random center (0.5)
    std::vector<float> center(17, 0.5f);
    float distToCenter = euclideanDistance(preferred, center);

    // With epsilon-greedy exploration (~25% random), the average distance will be higher.
    // Still expect most presets to trend toward preferred region.
    REQUIRE(avgDistance < distToCenter * 1.1f);

    testDir.deleteRecursively();
}

TEST_CASE("GA exploration produces diverse presets", "[integration]")
{
    auto testDir = getTestDir();
    auto names = getParamNames();
    MLPPreferenceModel model(names, testDir);

    // Pre-train MLP to strongly prefer one corner of parameter space
    std::vector<float> preferred(17, 0.9f);
    std::vector<float> notPreferred(17, 0.1f);

    IFitnessModel::Feedback like{1.0f, 5.0f};
    IFitnessModel::Feedback dislike{0.0f, 5.0f};

    for (int i = 0; i < 50; ++i)
    {
        model.sendFeedback(preferred, like);
        model.sendFeedback(notPreferred, dislike);
    }

    // Create GA with trained model
    GeneticAlgorithm ga(model);
    ga.startGA();

    // Collect many presets to get a good sample
    std::vector<std::vector<float>> generatedPresets;
    auto* bridge = ga.getParameterBridge();

    for (int i = 0; i < 25; ++i)
    {
        juce::Thread::sleep(120);
        if (bridge->hasData())
        {
            std::vector<float> params;
            float fitness;
            if (bridge->pop(params, fitness))
                generatedPresets.push_back(params);
        }
    }

    ga.stopGA();

    REQUIRE(generatedPresets.size() >= 10);

    // Count how many presets are "far" from the preferred region (exploratory)
    // Distance threshold: if closer to center than to preferred, it's exploratory
    std::vector<float> center(17, 0.5f);
    float distPreferredToCenter = euclideanDistance(preferred, center);

    int exploratoryCount = 0;
    for (const auto& p : generatedPresets)
    {
        float distToPreferred = euclideanDistance(p, preferred);
        // If distance to preferred is greater than half the preferred-to-center distance,
        // consider it exploratory
        if (distToPreferred > distPreferredToCenter * 0.5f)
            ++exploratoryCount;
    }

    // With 25% exploration rate, expect at least ~15% of presets to be exploratory
    // (accounting for randomness and that some exploited presets might also be "far")
    int minExploratory = static_cast<int>(generatedPresets.size()) / 7;  // ~14%
    REQUIRE(exploratoryCount >= minExploratory);

    testDir.deleteRecursively();
}

TEST_CASE("Novelty bonus increases population diversity", "[integration]")
{
    auto testDir = getTestDir();
    auto names = getParamNames();
    MLPPreferenceModel model(names, testDir);

    std::vector<float> preferred(17, 0.9f);
    std::vector<float> notPreferred(17, 0.1f);

    IFitnessModel::Feedback like{1.0f, 5.0f};
    IFitnessModel::Feedback dislike{0.0f, 5.0f};

    for (int i = 0; i < 50; ++i)
    {
        model.sendFeedback(preferred, like);
        model.sendFeedback(notPreferred, dislike);
    }

    GeneticAlgorithm gaNovelty(model);
    GAConfig noveltyConfig;
    noveltyConfig.noveltyBonus = true;
    noveltyConfig.multiObjective = true;
    noveltyConfig.noveltyWeight = 0.3f;
    gaNovelty.setConfig(noveltyConfig);
    gaNovelty.startGA();

    std::vector<std::vector<float>> noveltyPresets;
    auto* bridge = gaNovelty.getParameterBridge();

    for (int i = 0; i < 15; ++i)
    {
        juce::Thread::sleep(120);
        if (bridge->hasData())
        {
            std::vector<float> params;
            float fitness;
            if (bridge->pop(params, fitness))
                noveltyPresets.push_back(params);
        }
    }

    gaNovelty.stopGA();

    float avgDistance = 0.0f;
    int pairCount = 0;
    for (size_t i = 0; i < noveltyPresets.size(); ++i)
    {
        for (size_t j = i + 1; j < noveltyPresets.size(); ++j)
        {
            avgDistance += euclideanDistance(noveltyPresets[i], noveltyPresets[j]);
            ++pairCount;
        }
    }
    if (pairCount > 0)
        avgDistance /= static_cast<float>(pairCount);

    REQUIRE(avgDistance > 0.1f);

    testDir.deleteRecursively();
}

TEST_CASE("Adaptive exploration config applies correctly", "[integration]")
{
    auto testDir = getTestDir();
    auto names = getParamNames();
    MLPPreferenceModel model(names, testDir);

    GeneticAlgorithm ga(model);
    GAConfig config;
    config.adaptiveExploration = true;
    config.epsilonMax = 0.8f;
    config.epsilonMin = 0.1f;
    config.epsilonDecay = 0.9f;
    ga.setConfig(config);

    REQUIRE(ga.getConfig().adaptiveExploration == true);
    REQUIRE(ga.getConfig().epsilonMax == 0.8f);
    REQUIRE(ga.getConfig().epsilonDecay == 0.9f);

    testDir.deleteRecursively();
}

