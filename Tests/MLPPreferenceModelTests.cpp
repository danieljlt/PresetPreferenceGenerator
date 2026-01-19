#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GA/MLPPreferenceModel.h"

namespace
{
    juce::File getTestDir()
    {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("MLPPreferenceModelTests");
        tempDir.createDirectory();
        return tempDir;
    }
}

TEST_CASE("MLPPreferenceModel evaluate returns value in valid range")
{
    std::vector<juce::String> names = {"p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
                                        "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "p16"};
    auto testDir = getTestDir();
    MLPPreferenceModel model(names, testDir);
    
    std::vector<float> genome(17, 0.5f);
    
    float score = model.evaluate(genome);
    
    REQUIRE(score >= 0.0f);
    REQUIRE(score <= 1.0f);
}

TEST_CASE("MLPPreferenceModel sendFeedback updates predictions")
{
    std::vector<juce::String> names = {"p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
                                        "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "p16"};
    auto testDir = getTestDir();
    MLPPreferenceModel model(names, testDir);
    
    std::vector<float> genome(17, 0.5f);
    
    float before = model.evaluate(genome);
    
    // Send multiple "like" feedbacks
    IFitnessModel::Feedback feedback{1.0f, 5.0f};
    for (int i = 0; i < 20; ++i)
    {
        model.sendFeedback(genome, feedback);
    }
    
    float after = model.evaluate(genome);
    
    // Prediction should increase toward 1.0 after training toward "like"
    REQUIRE(after > before);
}

TEST_CASE("MLPPreferenceModel handles dislike feedback")
{
    std::vector<juce::String> names = {"p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
                                        "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "p16"};
    auto testDir = getTestDir();
    MLPPreferenceModel model(names, testDir);
    
    std::vector<float> genome(17, 0.5f);
    
    // Train toward "like" first
    IFitnessModel::Feedback likeFeedback{1.0f, 5.0f};
    for (int i = 0; i < 10; ++i)
    {
        model.sendFeedback(genome, likeFeedback);
    }
    
    float afterLikes = model.evaluate(genome);
    
    // Then train toward "dislike"
    IFitnessModel::Feedback dislikeFeedback{0.0f, 5.0f};
    for (int i = 0; i < 20; ++i)
    {
        model.sendFeedback(genome, dislikeFeedback);
    }
    
    float afterDislikes = model.evaluate(genome);
    
    // Prediction should decrease
    REQUIRE(afterDislikes < afterLikes);
}

