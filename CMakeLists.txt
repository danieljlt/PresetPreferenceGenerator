cmake_minimum_required(VERSION 3.22)

project(PresetPreferenceGenerator VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include CPM for dependency management
include(cmake/CPM.cmake)

# Add JUCE using CPM
CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.12
    OPTIONS
        "JUCE_BUILD_EXAMPLES OFF"
        "JUCE_BUILD_EXTRAS ON"
)

# Add Catch2 for unit testing
CPMAddPackage("gh:catchorg/Catch2@3.4.0")

# Create the plugin target
juce_add_plugin(PresetPreferenceGenerator
    COMPANY_NAME "DanielLister"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Djls
    PLUGIN_CODE Sam0
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "PresetPreferenceGenerator"
)

# Add source files
target_sources(PresetPreferenceGenerator
    PRIVATE
        # Main plugin files
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h

        # UI components
        Source/UI/PPGLookAndFeel.h
        Source/UI/PPGLookAndFeel.cpp
        Source/UI/SynthControlsPanel.h
        Source/UI/SynthControlsPanel.cpp
        Source/UI/GAControlsPanel.h
        Source/UI/GAControlsPanel.cpp
        
        # JX11 synthesizer files
        Source/JX11/Synth.cpp
        Source/JX11/Synth.h
        Source/JX11/Voice.h
        Source/JX11/Oscillator.h
        Source/JX11/Envelope.h
        Source/JX11/Filter.h
        Source/JX11/NoiseGenerator.h
        Source/JX11/Preset.h
        Source/JX11/Utils.h
        
        # Genetic Algorithm files
        Source/GA/Individual.cpp
        Source/GA/Individual.h
        Source/GA/Population.cpp
        Source/GA/Population.h
        Source/GA/GeneticAlgorithm.cpp
        Source/GA/GeneticAlgorithm.h
        Source/GA/ParameterBridge.cpp
        Source/GA/ParameterBridge.h
        Source/GA/SelectionOperators.cpp
        Source/GA/SelectionOperators.h
        Source/GA/MutationOperators.cpp
        Source/GA/MutationOperators.h
        Source/GA/CrossoverOperators.cpp
        Source/GA/CrossoverOperators.h
        Source/GA/GenomeConstraints.cpp
        Source/GA/GenomeConstraints.h
        
        # Fitness Model
        Source/GA/IFitnessModel.h

        Source/GA/MLP.cpp
        Source/GA/MLP.h
        Source/GA/MLPPreferenceModel.cpp
        Source/GA/MLPPreferenceModel.h
        
        # Audio Feature Extraction
        Source/GA/AudioFeatureCache.cpp
        Source/GA/AudioFeatureCache.h
        Source/GA/HeadlessSynth.cpp
        Source/GA/HeadlessSynth.h
        Source/GA/FeatureExtractor.cpp
        Source/GA/FeatureExtractor.h
)

# Link JUCE modules
target_link_libraries(PresetPreferenceGenerator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler definitions
target_compile_definitions(PresetPreferenceGenerator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Unit test executable
add_executable(Tests
    Tests/MLPTests.cpp
    Tests/GAOperatorsTests.cpp
    Tests/PopulationTests.cpp
    Tests/GenomeConstraintsTests.cpp
    Tests/IndividualTests.cpp
    Tests/ParameterBridgeTests.cpp
    Tests/MLPPreferenceModelTests.cpp
    Tests/GeneticAlgorithmTests.cpp
    Tests/IntegrationTests.cpp
    Tests/AudioFeatureCacheTests.cpp
    Source/GA/MLP.cpp
    Source/GA/MLPPreferenceModel.cpp
    Source/GA/Individual.cpp
    Source/GA/Population.cpp
    Source/GA/GenomeConstraints.cpp
    Source/GA/MutationOperators.cpp
    Source/GA/CrossoverOperators.cpp
    Source/GA/SelectionOperators.cpp
    Source/GA/ParameterBridge.cpp
    Source/GA/GeneticAlgorithm.cpp
    Source/GA/AudioFeatureCache.cpp
    Source/GA/HeadlessSynth.cpp
    Source/GA/FeatureExtractor.cpp
    Source/JX11/Synth.cpp
)

target_include_directories(Tests PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain juce::juce_core juce::juce_audio_basics juce::juce_dsp juce::juce_graphics juce::juce_audio_formats)
