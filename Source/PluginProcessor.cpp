/*
  ==============================================================================
    This file contains the main plugin processor implementation for JX11.
    It manages initialization, parameter updates, audio rendering, and MIDI input.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "JX11/Utils.h"
#include "GA/ParameterBridge.h"
#include "GA/PreferenceModel.h"

//==============================================================================
JX11AudioProcessor::JX11AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Initialize all parameter pointers using a casting utility
    castParameter(apvts, ParameterID::oscMix, oscMixParam);
    castParameter(apvts, ParameterID::oscTune, oscTuneParam);
    castParameter(apvts, ParameterID::oscFine, oscFineParam);
    castParameter(apvts, ParameterID::glideMode, glideModeParam);
    castParameter(apvts, ParameterID::glideRate, glideRateParam);
    castParameter(apvts, ParameterID::glideBend, glideBendParam);
    castParameter(apvts, ParameterID::filterFreq, filterFreqParam);
    castParameter(apvts, ParameterID::filterReso, filterResoParam);
    castParameter(apvts, ParameterID::filterEnv, filterEnvParam);
    castParameter(apvts, ParameterID::filterLFO, filterLFOParam);
    castParameter(apvts, ParameterID::filterVelocity, filterVelocityParam);
    castParameter(apvts, ParameterID::filterAttack, filterAttackParam);
    castParameter(apvts, ParameterID::filterDecay, filterDecayParam);
    castParameter(apvts, ParameterID::filterSustain, filterSustainParam);
    castParameter(apvts, ParameterID::filterRelease, filterReleaseParam);
    castParameter(apvts, ParameterID::envAttack, envAttackParam);
    castParameter(apvts, ParameterID::envDecay, envDecayParam);
    castParameter(apvts, ParameterID::envSustain, envSustainParam);
    castParameter(apvts, ParameterID::envRelease, envReleaseParam);
    castParameter(apvts, ParameterID::lfoRate, lfoRateParam);
    castParameter(apvts, ParameterID::vibrato, vibratoParam);
    castParameter(apvts, ParameterID::noise, noiseParam);
    castParameter(apvts, ParameterID::octave, octaveParam);
    castParameter(apvts, ParameterID::tuning, tuningParam);
    castParameter(apvts, ParameterID::outputLevel, outputLevelParam);
    castParameter(apvts, ParameterID::polyMode, polyModeParam);
    
    createPrograms();      // Populate preset list
    setCurrentProgram(0);  // Load the default preset
    
    // Initialize fitness model with parameter names for CSV schema
    std::vector<juce::String> paramNames;
    for (const auto& pid : getGAParameterIDs())
        paramNames.push_back(pid.getParamID());
        
    fitnessModel = std::make_unique<PreferenceModel>(paramNames);
    
    // Initialize GA engine with reference to fitness model
    gaEngine = std::make_unique<GeneticAlgorithm>(*fitnessModel);
    
    // Initialize parameter smoothing vectors (17 GA parameters)
    targetParameters.resize(17, 0.0f);
    currentParameters.resize(17, 0.0f);
    
    // Start timer for parameter bridge polling (50ms = 20Hz)
    startTimer(static_cast<int>(timerInterval * 1000.0f));

    apvts.state.addListener(this); // Listen for parameter tree changes
}

JX11AudioProcessor::~JX11AudioProcessor()
{
    stopTimer();
    apvts.state.removeListener(this);
}

//==============================================================================
// Plugin metadata and capabilities

const juce::String JX11AudioProcessor::getName() const { return JucePlugin_Name; }

bool JX11AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JX11AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JX11AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JX11AudioProcessor::getTailLengthSeconds() const { return 0.0; }

//==============================================================================
// Preset program handling

int JX11AudioProcessor::getNumPrograms() { return int(presets.size()); }

int JX11AudioProcessor::getCurrentProgram() { return currentProgram; }

void JX11AudioProcessor::setCurrentProgram (int index)
{
    currentProgram = index;

    // Array of all parameter pointers (to apply preset values)
    juce::RangedAudioParameter* params[NUM_PARAMS] =
    {
        oscMixParam, oscTuneParam, oscFineParam, glideModeParam, glideRateParam,
        glideBendParam, filterFreqParam, filterResoParam, filterEnvParam,
        filterLFOParam, filterVelocityParam, filterAttackParam, filterDecayParam,
        filterSustainParam, filterReleaseParam, envAttackParam, envDecayParam,
        envSustainParam, envReleaseParam, lfoRateParam, vibratoParam, noiseParam,
        octaveParam, tuningParam, outputLevelParam, polyModeParam,
    };

    const Preset& preset = presets[index];

    // Set all parameters to preset values (converted to normalized)
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        params[i]->setValueNotifyingHost(params[i]->convertTo0to1(preset.param[i]));
    }

    reset(); // Reset DSP engine after loading preset
}

const juce::String JX11AudioProcessor::getProgramName (int index)
{
    return presets[index].name;
}

void JX11AudioProcessor::changeProgramName (int, const juce::String&)
{
    // Program renaming not implemented
}

//==============================================================================
// DSP preparation and reset

void JX11AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    DBG("Plugin loaded and DBG active");
    synth.allocateResources(sampleRate, samplesPerBlock);
    parametersChanged.store(true); // Mark for update on next block
    reset();
}

void JX11AudioProcessor::releaseResources()
{
    synth.deallocateResources();
}

void JX11AudioProcessor::reset()
{
    synth.reset();
    // Smooth gain ramping on output level
    synth.outputLevelSmoother.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(outputLevelParam->get())
    );
}

//==============================================================================
// Audio channel layout support

#ifndef JucePlugin_PreferredChannelConfigurations
bool JX11AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    // Only stereo output supported
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    // Ensure input and output layouts match (for FX plugins)
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
// Main audio + MIDI processing

void JX11AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels beyond the number of inputs
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Apply parameter updates if needed (deferred for thread safety)
    bool expected = true;
    if (isNonRealtime() || parametersChanged.compare_exchange_strong(expected, false))
        update();

    // Render audio in chunks around MIDI events
    splitBufferByEvents(buffer, midiMessages);
}

// Recalculates internal synth parameters when a plugin parameter changes
void JX11AudioProcessor::update()
{
    float sampleRate = float(getSampleRate());
    float inverseSampleRate = 1.0f / sampleRate;

    // Convert ADSR times using exponential scaling for natural-feeling envelopes
    synth.envAttack = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envAttackParam->get()));
    synth.envDecay = std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envDecayParam->get()));
    synth.envSustain = envSustainParam->get() / 100.0f;

    float envRelease = envReleaseParam->get();
    synth.envRelease = (envRelease < 1.0f) ? 0.75f
                                           : std::exp(-inverseSampleRate * std::exp(5.5f - 0.075f * envRelease));

    // Nonlinear scaling for noise mix to increase control resolution at low values
    float noiseMix = noiseParam->get() / 100.0f;
    noiseMix *= noiseMix;
    synth.noiseMix = noiseMix * 0.06f;

    // Oscillator mix and tuning
    synth.oscMix = oscMixParam->get() / 100.0f;
    float semi = oscTuneParam->get();
    float cent = oscFineParam->get();
    synth.detune = std::pow(1.059463094359f, -semi - 0.01f * cent); // convert semitone + cents to ratio

    // Calculate base tuning frequency (Hz) from octave/tuning parameters
    float octave = octaveParam->get();
    float tuning = tuningParam->get();
    float tuneInSemi = -36.3763f - 12.0f * octave - tuning / 100.0f;
    synth.tune = sampleRate * std::exp(0.05776226505f * tuneInSemi);

    // Set mono or polyphonic voice count
    synth.numVoices = (polyModeParam->getIndex() == 0) ? 1 : Synth::MAX_VOICES;

    // Filter modulation and resonance
    float filterLFO = filterLFOParam->get() / 100.0f;
    synth.filterLFODepth = 2.5f * filterLFO * filterLFO;

    float filterReso = filterResoParam->get() / 100.0f;
    synth.filterQ = std::exp(3.0f * filterReso);

    // Output gain normalization (volume trim compensation)
    synth.volumeTrim = 0.0008f * (3.2f - synth.oscMix - 25.0f * synth.noiseMix) * (1.5f - 0.5f * filterReso);

    // Target level smoothing for volume control
    synth.outputLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->get()));

    // Velocity sensitivity (OFF if less than -90)
    float filterVelocity = filterVelocityParam->get();
    if (filterVelocity < -90.0f)
    {
        synth.velocitySensitivity = 0.0f;
        synth.ignoreVelocity = true;
    }
    else
    {
        synth.velocitySensitivity = 0.0005f * filterVelocity;
        synth.ignoreVelocity = false;
    }

    // LFO rate (in radians/sample)
    const float inverseUpdateRate = inverseSampleRate * synth.LFO_MAX;
    float lfoRate = std::exp(7.0f * lfoRateParam->get() - 4.0f);
    synth.lfoInc = lfoRate * inverseUpdateRate * float(TWO_PI);

    // Vibrato and PWM depth (nonlinear scale)
    float vibrato = vibratoParam->get() / 200.0f;
    synth.vibrato = 0.2f * vibrato * vibrato;
    synth.pwmDepth = synth.vibrato;
    if (vibrato < 0.0f) synth.vibrato = 0.0f;

    // Glide (portamento) rate and bend amount
    synth.glideMode = glideModeParam->getIndex();
    float glideRate = glideRateParam->get();
    synth.glideRate = (glideRate < 2.0f) ? 1.0f
                                         : 1.0f - std::exp(-inverseUpdateRate * std::exp(6.0f - 0.07f * glideRate));
    synth.glideBend = glideBendParam->get();

    // Filter envelope and tracking
    synth.filterKeyTracking = 0.08f * filterFreqParam->get() - 1.5f;
    synth.filterAttack = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterAttackParam->get()));
    synth.filterDecay = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterDecayParam->get()));
    float filterSustain = filterSustainParam->get() / 100.0f;
    synth.filterSustain = filterSustain * filterSustain;
    synth.filterRelease = std::exp(-inverseUpdateRate * std::exp(5.5f - 0.075f * filterReleaseParam->get()));
    synth.filterEnvDepth = 0.06f * filterEnvParam->get();
}


// Splits the audio buffer based on incoming MIDI events
void JX11AudioProcessor::splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    int bufferOffset = 0;

    // Iterate through MIDI messages
    for (const auto metadata : midiMessages)
    {
        int samplesThisSegment = metadata.samplePosition - bufferOffset;
        if (samplesThisSegment > 0)
        {
            render(buffer, samplesThisSegment, bufferOffset);
            bufferOffset += samplesThisSegment;
        }

        // Ignore messages longer than 3 bytes (e.g. sysex)
        if (metadata.numBytes <= 3)
        {
            uint8_t data1 = (metadata.numBytes >= 2) ? metadata.data[1] : 0;
            uint8_t data2 = (metadata.numBytes == 3) ? metadata.data[2] : 0;
            handleMIDI(metadata.data[0], data1, data2);
        }
    }

    // Render the final segment (or whole buffer if no MIDI events)
    int samplesLastSegment = buffer.getNumSamples() - bufferOffset;
    if (samplesLastSegment > 0)
    {
        render(buffer, samplesLastSegment, bufferOffset);
    }

    midiMessages.clear(); // Clear buffer after processing
}


// Handles raw MIDI events (used for pitch bend, CC, etc.)
void JX11AudioProcessor::handleMIDI(uint8_t data0, uint8_t data1, uint8_t data2)
{
    // Control Change - Volume
    if ((data0 & 0xF0) == 0xB0)
    {
        if (data1 == 0x07)
        {
            float volumeCtl = float(data2) / 127.0f;
            outputLevelParam->beginChangeGesture();
            outputLevelParam->setValueNotifyingHost(volumeCtl);
            outputLevelParam->endChangeGesture();
        }
    }
    
    // Program Change - Switch Presets
    if ((data0 & 0xF0) == 0xC0)
    {
        if (data1 < presets.size())
        {
            setCurrentProgram(data1);
        }
    }
    
    // Forward all MIDI to the synth
    synth.midiMessage(data0, data1, data2);
}


// Renders audio for a given buffer range using active voices
void JX11AudioProcessor::render(juce::AudioBuffer<float> &buffer, int sampleCount, int bufferOffset)
{
    float* outputBuffers[2] = { nullptr, nullptr };
    outputBuffers[0] = buffer.getWritePointer(0) + bufferOffset;
    if (getTotalNumOutputChannels() > 1)
    {
        outputBuffers[1] = buffer.getWritePointer(1) + bufferOffset;
    }
    synth.render(outputBuffers, sampleCount);
}

//==============================================================================
// Editor (GUI)

bool JX11AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JX11AudioProcessor::createEditor()
{
    return new JX11AudioProcessorEditor(*this);
}

//==============================================================================
// State Management

void JX11AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void JX11AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        parametersChanged.store(true);
    }
}

//==============================================================================
// Parameter and preset management

// Initializes List of Factory Presets
void JX11AudioProcessor::createPrograms()
{
    presets.emplace_back("Init", 0.00f, -12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 100.00f, 15.00f, 50.00f, 0.00f, 0.00f, 0.00f, 30.00f, 0.00f, 25.00f, 0.00f, 50.00f, 100.00f, 30.00f, 0.81f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("5th Sweep Pad", 100.00f, -7.00f, -6.30f, 1.00f, 32.00f, 0.00f, 90.00f, 60.00f, -76.00f, 0.00f, 0.00f, 90.00f, 89.00f, 90.00f, 73.00f, 0.00f, 50.00f, 100.00f, 71.00f, 0.81f, 30.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Echo Pad [SA]", 88.00f, 0.00f, 0.00f, 0.00f, 49.00f, 0.00f, 46.00f, 76.00f, 38.00f, 10.00f, 38.00f, 100.00f, 86.00f, 76.00f, 57.00f, 30.00f, 80.00f, 68.00f, 66.00f, 0.79f, -74.00f, 25.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Space Chimes [SA]", 88.00f, 0.00f, 0.00f, 0.00f, 49.00f, 0.00f, 49.00f, 82.00f, 32.00f, 8.00f, 78.00f, 85.00f, 69.00f, 76.00f, 47.00f, 12.00f, 22.00f, 55.00f, 66.00f, 0.89f, -32.00f, 0.00f, 2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Solid Backing", 100.00f, -12.00f, -18.70f, 0.00f, 35.00f, 0.00f, 30.00f, 25.00f, 40.00f, 0.00f, 26.00f, 0.00f, 35.00f, 0.00f, 25.00f, 0.00f, 50.00f, 100.00f, 30.00f, 0.81f, 0.00f, 50.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Velocity Backing [SA]", 41.00f, 0.00f, 9.70f, 0.00f, 8.00f, -1.68f, 49.00f, 1.00f, -32.00f, 0.00f, 86.00f, 61.00f, 87.00f, 100.00f, 93.00f, 11.00f, 48.00f, 98.00f, 32.00f, 0.81f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Rubber Backing [ZF]", 29.00f, 12.00f, -5.60f, 0.00f, 18.00f, 5.06f, 35.00f, 15.00f, 54.00f, 14.00f, 8.00f, 0.00f, 42.00f, 13.00f, 21.00f, 0.00f, 56.00f, 0.00f, 32.00f, 0.20f, 16.00f, 22.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("808 State Lead", 100.00f, 7.00f, -7.10f, 2.00f, 34.00f, 12.35f, 65.00f, 63.00f, 50.00f, 16.00f, 0.00f, 0.00f, 30.00f, 0.00f, 25.00f, 17.00f, 50.00f, 100.00f, 3.00f, 0.81f, 0.00f, 0.00f, 1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Mono Glide", 0.00f, -12.00f, 0.00f, 2.00f, 46.00f, 0.00f, 51.00f, 0.00f, 0.00f, 0.00f, -100.00f, 0.00f, 30.00f, 0.00f, 25.00f, 37.00f, 50.00f, 100.00f, 38.00f, 0.81f, 24.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Detuned Techno Lead", 84.00f, 0.00f, -17.20f, 2.00f, 41.00f, -0.15f, 54.00f, 1.00f, 16.00f, 21.00f, 34.00f, 0.00f, 9.00f, 100.00f, 25.00f, 20.00f, 85.00f, 100.00f, 30.00f, 0.83f, -82.00f, 40.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Hard Lead [SA]", 71.00f, 12.00f, 0.00f, 0.00f, 24.00f, 36.00f, 56.00f, 52.00f, 38.00f, 19.00f, 40.00f, 100.00f, 14.00f, 65.00f, 95.00f, 7.00f, 91.00f, 100.00f, 15.00f, 0.84f, -34.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Bubble", 0.00f, -12.00f, -0.20f, 0.00f, 71.00f, -0.00f, 23.00f, 77.00f, 60.00f, 32.00f, 26.00f, 40.00f, 18.00f, 66.00f, 14.00f, 0.00f, 38.00f, 65.00f, 16.00f, 0.48f, 0.00f, 0.00f, 1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Monosynth", 62.00f, -12.00f, 0.00f, 1.00f, 35.00f, 0.02f, 64.00f, 39.00f, 2.00f, 65.00f, -100.00f, 7.00f, 52.00f, 24.00f, 84.00f, 13.00f, 30.00f, 76.00f, 21.00f, 0.58f, -40.00f, 0.00f, -1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Moogcury Lite", 81.00f, 24.00f, -9.80f, 1.00f, 15.00f, -0.97f, 39.00f, 17.00f, 38.00f, 40.00f, 24.00f, 0.00f, 47.00f, 19.00f, 37.00f, 0.00f, 50.00f, 20.00f, 33.00f, 0.38f, 6.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Gangsta Whine", 0.00f, 0.00f, 0.00f, 2.00f, 44.00f, 0.00f, 41.00f, 46.00f, 0.00f, 0.00f, -100.00f, 0.00f, 0.00f, 100.00f, 25.00f, 15.00f, 50.00f, 100.00f, 32.00f, 0.81f, -2.00f, 0.00f, 2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Higher Synth [ZF]", 48.00f, 0.00f, -8.80f, 0.00f, 0.00f, 0.00f, 50.00f, 47.00f, 46.00f, 30.00f, 60.00f, 0.00f, 10.00f, 0.00f, 7.00f, 0.00f, 42.00f, 0.00f, 22.00f, 0.21f, 18.00f, 16.00f, 2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("303 Saw Bass", 0.00f, 0.00f, 0.00f, 1.00f, 49.00f, 0.00f, 55.00f, 75.00f, 38.00f, 35.00f, 0.00f, 0.00f, 56.00f, 0.00f, 56.00f, 0.00f, 80.00f, 100.00f, 24.00f, 0.26f, -2.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("303 Square Bass", 75.00f, 0.00f, 0.00f, 1.00f, 49.00f, 0.00f, 55.00f, 75.00f, 38.00f, 35.00f, 0.00f, 14.00f, 49.00f, 0.00f, 39.00f, 0.00f, 80.00f, 100.00f, 24.00f, 0.26f, -2.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Analog Bass", 100.00f, -12.00f, -10.90f, 1.00f, 19.00f, 0.00f, 30.00f, 51.00f, 70.00f, 9.00f, -100.00f, 0.00f, 88.00f, 0.00f, 21.00f, 0.00f, 50.00f, 100.00f, 46.00f, 0.81f, 0.00f, 0.00f, -1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Analog Bass 2", 100.00f, -12.00f, -10.90f, 0.00f, 19.00f, 13.44f, 48.00f, 43.00f, 88.00f, 0.00f, 60.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 61.00f, 100.00f, 32.00f, 0.81f, 0.00f, 0.00f, -1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Low Pulses", 97.00f, -12.00f, -3.30f, 0.00f, 35.00f, 0.00f, 80.00f, 40.00f, 4.00f, 0.00f, 0.00f, 0.00f, 77.00f, 0.00f, 25.00f, 0.00f, 50.00f, 100.00f, 30.00f, 0.81f, -68.00f, 0.00f, -2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Sine Infra-Bass", 0.00f, -12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 33.00f, 76.00f, 6.00f, 0.00f, 0.00f, 0.00f, 30.00f, 0.00f, 25.00f, 0.00f, 55.00f, 25.00f, 30.00f, 0.81f, 4.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Wobble Bass [SA]", 100.00f, -12.00f, -8.80f, 0.00f, 82.00f, 0.21f, 72.00f, 47.00f, -32.00f, 34.00f, 64.00f, 20.00f, 69.00f, 100.00f, 15.00f, 9.00f, 50.00f, 100.00f, 7.00f, 0.81f, -8.00f, 0.00f, -1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Squelch Bass", 100.00f, -12.00f, -8.80f, 0.00f, 35.00f, 0.00f, 67.00f, 70.00f, -48.00f, 0.00f, 0.00f, 48.00f, 69.00f, 100.00f, 15.00f, 0.00f, 50.00f, 100.00f, 7.00f, 0.81f, -8.00f, 0.00f, -1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Rubber Bass [ZF]", 49.00f, -12.00f, 1.60f, 1.00f, 35.00f, 0.00f, 36.00f, 15.00f, 50.00f, 20.00f, 0.00f, 0.00f, 38.00f, 0.00f, 25.00f, 0.00f, 60.00f, 100.00f, 22.00f, 0.19f, 0.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Soft Pick Bass", 37.00f, 0.00f, 7.80f, 0.00f, 22.00f, 0.00f, 33.00f, 47.00f, 42.00f, 16.00f, 18.00f, 0.00f, 0.00f, 0.00f, 25.00f, 4.00f, 58.00f, 0.00f, 22.00f, 0.15f, -12.00f, 33.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Fretless Bass", 50.00f, 0.00f, -14.40f, 1.00f, 34.00f, 0.00f, 51.00f, 0.00f, 16.00f, 0.00f, 34.00f, 0.00f, 9.00f, 0.00f, 25.00f, 20.00f, 85.00f, 0.00f, 30.00f, 0.81f, 40.00f, 0.00f, -2.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Whistler", 23.00f, 0.00f, -0.70f, 0.00f, 35.00f, 0.00f, 33.00f, 100.00f, 0.00f, 0.00f, 0.00f, 0.00f, 29.00f, 0.00f, 25.00f, 68.00f, 39.00f, 58.00f, 36.00f, 0.81f, 28.00f, 38.00f, 2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Very Soft Pad", 39.00f, 0.00f, -4.90f, 2.00f, 12.00f, 0.00f, 35.00f, 78.00f, 0.00f, 0.00f, 0.00f, 0.00f, 30.00f, 0.00f, 25.00f, 35.00f, 50.00f, 80.00f, 70.00f, 0.81f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Pizzicato", 0.00f, -12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 23.00f, 20.00f, 50.00f, 0.00f, 0.00f, 0.00f, 22.00f, 0.00f, 25.00f, 0.00f, 47.00f, 0.00f, 30.00f, 0.81f, 0.00f, 80.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Synth Strings", 100.00f, 0.00f, -7.10f, 0.00f, 0.00f, -0.97f, 42.00f, 26.00f, 50.00f, 14.00f, 38.00f, 0.00f, 67.00f, 55.00f, 97.00f, 82.00f, 70.00f, 100.00f, 42.00f, 0.84f, 34.00f, 30.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Synth Strings 2", 75.00f, 0.00f, -3.80f, 0.00f, 49.00f, 0.00f, 55.00f, 16.00f, 38.00f, 8.00f, -60.00f, 76.00f, 29.00f, 76.00f, 100.00f, 46.00f, 80.00f, 100.00f, 39.00f, 0.79f, -46.00f, 0.00f, 1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Leslie Organ", 0.00f, 0.00f, 0.00f, 0.00f, 13.00f, -0.38f, 38.00f, 74.00f, 8.00f, 20.00f, -100.00f, 0.00f, 55.00f, 52.00f, 31.00f, 0.00f, 17.00f, 73.00f, 28.00f, 0.87f, -52.00f, 0.00f, -1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Click Organ", 50.00f, 12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 44.00f, 50.00f, 30.00f, 16.00f, -100.00f, 0.00f, 0.00f, 18.00f, 0.00f, 0.00f, 75.00f, 80.00f, 0.00f, 0.81f, -2.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Hard Organ", 89.00f, 19.00f, -0.90f, 0.00f, 35.00f, 0.00f, 51.00f, 62.00f, 8.00f, 0.00f, -100.00f, 0.00f, 37.00f, 0.00f, 100.00f, 4.00f, 8.00f, 72.00f, 4.00f, 0.77f, -2.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Bass Clarinet", 100.00f, 0.00f, 0.00f, 1.00f, 0.00f, 0.00f, 51.00f, 10.00f, 0.00f, 11.00f, 0.00f, 0.00f, 0.00f, 0.00f, 25.00f, 35.00f, 65.00f, 65.00f, 32.00f, 0.79f, -2.00f, 20.00f, -1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Trumpet", 0.00f, 0.00f, 0.00f, 1.00f, 6.00f, 0.00f, 57.00f, 0.00f, -36.00f, 15.00f, 0.00f, 21.00f, 15.00f, 0.00f, 25.00f, 24.00f, 60.00f, 80.00f, 10.00f, 0.75f, 10.00f, 25.00f, 1.00f, 0.00f, 0.00f, 0.00f);
    presets.emplace_back("Soft Horn", 12.00f, 19.00f, 1.90f, 0.00f, 35.00f, 0.00f, 50.00f, 21.00f, -42.00f, 12.00f, 20.00f, 0.00f, 35.00f, 36.00f, 25.00f, 8.00f, 50.00f, 100.00f, 27.00f, 0.83f, 2.00f, 10.00f, -1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Brass Section", 43.00f, 12.00f, -7.90f, 0.00f, 28.00f, -0.79f, 50.00f, 0.00f, 18.00f, 0.00f, 0.00f, 24.00f, 16.00f, 91.00f, 8.00f, 17.00f, 50.00f, 80.00f, 45.00f, 0.81f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Synth Brass", 40.00f, 0.00f, -6.30f, 0.00f, 30.00f, -3.07f, 39.00f, 15.00f, 50.00f, 0.00f, 0.00f, 39.00f, 30.00f, 82.00f, 25.00f, 33.00f, 74.00f, 76.00f, 41.00f, 0.81f, -6.00f, 23.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Detuned Syn Brass [ZF]", 68.00f, 0.00f, 31.80f, 0.00f, 31.00f, 0.50f, 26.00f, 7.00f, 70.00f, 0.00f, 32.00f, 0.00f, 83.00f, 0.00f, 5.00f, 0.00f, 75.00f, 54.00f, 32.00f, 0.76f, -26.00f, 29.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Power PWM", 100.00f, -12.00f, -8.80f, 0.00f, 35.00f, 0.00f, 82.00f, 13.00f, 50.00f, 0.00f, -100.00f, 24.00f, 30.00f, 88.00f, 34.00f, 0.00f, 50.00f, 100.00f, 48.00f, 0.71f, -26.00f, 0.00f, -1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Water Velocity [SA]", 76.00f, 0.00f, -1.40f, 0.00f, 49.00f, 0.00f, 87.00f, 67.00f, 100.00f, 32.00f, -82.00f, 95.00f, 56.00f, 72.00f, 100.00f, 4.00f, 76.00f, 11.00f, 46.00f, 0.88f, 44.00f, 0.00f, -1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Ghost [SA]", 75.00f, 0.00f, -7.10f, 2.00f, 16.00f, -0.00f, 38.00f, 58.00f, 50.00f, 16.00f, 62.00f, 0.00f, 30.00f, 40.00f, 31.00f, 37.00f, 50.00f, 100.00f, 54.00f, 0.85f, 66.00f, 43.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Soft E.Piano", 31.00f, 0.00f, -0.20f, 0.00f, 35.00f, 0.00f, 34.00f, 26.00f, 6.00f, 0.00f, 26.00f, 0.00f, 22.00f, 0.00f, 39.00f, 0.00f, 80.00f, 0.00f, 44.00f, 0.81f, 2.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Thumb Piano", 72.00f, 15.00f, 50.00f, 0.00f, 35.00f, 0.00f, 37.00f, 47.00f, 8.00f, 0.00f, 0.00f, 0.00f, 45.00f, 0.00f, 39.00f, 0.00f, 39.00f, 0.00f, 48.00f, 0.81f, 20.00f, 0.00f, 1.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Steel Drums [ZF]", 81.00f, 12.00f, -12.00f, 0.00f, 18.00f, 2.30f, 40.00f, 30.00f, 8.00f, 17.00f, -20.00f, 0.00f, 42.00f, 23.00f, 47.00f, 12.00f, 48.00f, 0.00f, 49.00f, 0.53f, -28.00f, 34.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Car Horn", 57.00f, -1.00f, -2.80f, 0.00f, 35.00f, 0.00f, 46.00f, 0.00f, 36.00f, 0.00f, 0.00f, 46.00f, 30.00f, 100.00f, 23.00f, 30.00f, 50.00f, 100.00f, 31.00f, 1.00f, -24.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Helicopter", 0.00f, -12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 8.00f, 36.00f, 38.00f, 100.00f, 0.00f, 100.00f, 100.00f, 0.00f, 100.00f, 96.00f, 50.00f, 100.00f, 92.00f, 0.97f, 0.00f, 100.00f, -2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Arctic Wind", 0.00f, -12.00f, 0.00f, 0.00f, 35.00f, 0.00f, 16.00f, 85.00f, 0.00f, 28.00f, 0.00f, 37.00f, 30.00f, 0.00f, 25.00f, 89.00f, 50.00f, 100.00f, 89.00f, 0.24f, 0.00f, 100.00f, 2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Thip", 100.00f, -7.00f, 0.00f, 0.00f, 35.00f, 0.00f, 0.00f, 100.00f, 94.00f, 0.00f, 0.00f, 2.00f, 20.00f, 0.00f, 20.00f, 0.00f, 46.00f, 0.00f, 30.00f, 0.81f, 0.00f, 78.00f, 0.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Synth Tom", 0.00f, -12.00f, 0.00f, 0.00f, 76.00f, 24.53f, 30.00f, 33.00f, 52.00f, 0.00f, 36.00f, 0.00f, 59.00f, 0.00f, 59.00f, 10.00f, 50.00f, 0.00f, 50.00f, 0.81f, 0.00f, 70.00f, -2.00f, 0.00f, 0.00f, 1.00f);
    presets.emplace_back("Squelchy Frog", 50.00f, -5.00f, -7.90f, 2.00f, 77.00f, -36.00f, 40.00f, 65.00f, 90.00f, 0.00f, 0.00f, 33.00f, 50.00f, 0.00f, 25.00f, 0.00f, 70.00f, 65.00f, 18.00f, 0.32f, 100.00f, 0.00f, -2.00f, 0.00f, 0.00f, 1.00f);
}

// Defines & returns the full list of parameter definitions for the plugin
juce::AudioProcessorValueTreeState::ParameterLayout JX11AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
                                                            ParameterID::polyMode,
                                                            "Polyphony",
                                                            juce::StringArray{ "Mono", "Poly" },
                                                            1));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::oscTune,
                                                           "Osc Tune",
                                                           juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f),
                                                           -12.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("semi")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::oscFine,
                                                           "Osc Fine",
                                                           juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f, 0.3f, true),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("cent")));
    
    auto oscMixStringFromValue = [](float value, int)
    {
        char s[16] = { 0 };
        snprintf(s, 16, "%4.0f:%2.0f", 100.0 - 0.5f * value, 0.5f * value);
        return juce::String(s);
    };
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::oscMix,
                                                           "Osc Mix",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%").withStringFromValueFunction(oscMixStringFromValue)));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
                                                            ParameterID::glideMode,
                                                            "Glide Mode",
                                                            juce::StringArray { "Off", "Legato", "Always" },
                                                            0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::glideRate,
                                                           "Glide Rate",
                                                           juce::NormalisableRange<float>(0.0f, 100.f, 1.0f),
                                                           35.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::glideBend,
                                                           "Glide Bend",
                                                           juce::NormalisableRange<float>(-36.0f, 36.0f, 0.01f, 0.4f, true),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("semi")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterFreq,
                                                           "Filter Freq",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
                                                           100.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterReso,
                                                           "Filter Reso",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           15.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterEnv,
                                                           "Filter Env",
                                                           juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
                                                           50.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterLFO,
                                                           "Filter LFO",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    auto filterVelocityStringFromValue = [](float value, int)
    {
        if (value < -90.0f)
        {
            return juce::String("OFF");
        }
        else
        {
            return juce::String(value);
        }
    };
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterVelocity,
                                                           "Velocity",
                                                           juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes()
                                                               .withLabel("%")
                                                               .withStringFromValueFunction(filterVelocityStringFromValue)));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterAttack,
                                                           "Filter Attack",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterDecay,
                                                           "Filter Decay",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           30.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterSustain,
                                                           "Filter Sustain",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::filterRelease,
                                                           "Filter Release",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           25.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::envAttack,
                                                           "Env Attack",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::envDecay,
                                                           "Env Decay",
                                                           juce::NormalisableRange<float>(15.0f, 100.0f, 1.0f),
                                                           50.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::envSustain,
                                                           "Env Sustain",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           100.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::envRelease,
                                                           "Env Release",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           30.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    auto lfoRateStringFromValue = [](float value, int)
    {
        float lfoHz = std::exp(7.0f * value - 4.0f);
        return juce::String(lfoHz, 3);
    };
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::lfoRate,
                                                           "LFO Rate",
                                                           juce::NormalisableRange<float>(),
                                                           0.81f,
                                                           juce::AudioParameterFloatAttributes()
                                                               .withLabel("Hz")
                                                               .withStringFromValueFunction(lfoRateStringFromValue)));
    
    auto vibratoStringFromValue = [](float value, int)
    {
        if (value < 0.0f)
        {
            return "PWM" + juce::String(-value, 1);
        }
        else
        {
            return juce::String(value, 1);
        }
    };
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::vibrato,
                                                           "Vibrato",
                                                           juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes()
                                                               .withLabel("%")
                                                               .withStringFromValueFunction(vibratoStringFromValue)));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::noise,
                                                           "Noise",
                                                           juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("%")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::octave,
                                                           "Octave",
                                                           juce::NormalisableRange<float>(-2.0f, 2.0f, 1.0f),
                                                           0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::tuning,
                                                           "Tuning",
                                                           juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("cent")));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                           ParameterID::outputLevel,
                                                           "Output Level",
                                                           juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
                                                           0.0f,
                                                           juce::AudioParameterFloatAttributes().withLabel("dB")));
    
    return layout;
}

const std::vector<juce::ParameterID>& JX11AudioProcessor::getGAParameterIDs()
{
    static const std::vector<juce::ParameterID> params = 
    {
        ParameterID::oscMix, ParameterID::oscFine,
        ParameterID::filterFreq, ParameterID::filterReso, ParameterID::filterEnv, ParameterID::filterLFO,
        ParameterID::filterAttack, ParameterID::filterDecay, ParameterID::filterSustain, ParameterID::filterRelease,
        ParameterID::envAttack, ParameterID::envDecay, ParameterID::envSustain, ParameterID::envRelease,
        ParameterID::lfoRate, ParameterID::vibrato, ParameterID::noise
    };
    
    return params;
}


//==============================================================================
// GA Control Methods

void JX11AudioProcessor::startGA()
{
    // Reset fitness tracker
    lastGAFitness = 0.0f;
    
    if (gaEngine)
    {
        // Clear any old candidates from previous runs
        if (auto* bridge = gaEngine->getParameterBridge())
            bridge->clear();
            
        gaEngine->startGA();
        
        // Load the first candidate immediately so tracking begins with a GA preset
        fetchNextPreset();
    }
}

void JX11AudioProcessor::stopGA()
{
    if (gaEngine)
        gaEngine->stopGA();
}

void JX11AudioProcessor::pauseGA()
{
    if (gaEngine)
        gaEngine->pauseGA();
}

void JX11AudioProcessor::resumeGA()
{
    if (gaEngine)
        gaEngine->resumeGA();
}

bool JX11AudioProcessor::isGARunning() const
{
    return gaEngine ? gaEngine->isGARunning() : false;
}

bool JX11AudioProcessor::isGAPaused() const
{
    return gaEngine ? gaEngine->isGAPaused() : false;
}

void JX11AudioProcessor::debugLogQueue()
{
    if (gaEngine)
        gaEngine->debugDumpQueue();
}

void JX11AudioProcessor::logFeedback(const IFitnessModel::Feedback& feedback)
{
    if (fitnessModel)
    {
        // Capture current parameter values directly from the plugin state
        const auto& gaParams = getGAParameterIDs();
        std::vector<float> currentGenome;
        currentGenome.reserve(gaParams.size());
        
        for (const auto& pid : gaParams)
        {
            if (auto* param = apvts.getParameter(pid.getParamID()))
            {
                currentGenome.push_back(param->getValue()); // Normalized 0..1
            }
            else
            {
                // Should not happen if IDs are correct
                currentGenome.push_back(0.0f);
            }
        }
        
        fitnessModel->sendFeedback(currentGenome, feedback);
    }
}

//==============================================================================
// Parameter Bridge Integration

void JX11AudioProcessor::timerCallback()
{
    if (!gaEngine || !gaEngine->getParameterBridge())
        return;
             
    // Gradually interpolate toward target
    if (isInterpolating)
    {
        interpolateAndApplyParameters();
    }
}

bool JX11AudioProcessor::fetchNextPreset()
{
    if (!gaEngine || !gaEngine->getParameterBridge())
        return false;
        
    std::vector<float> params;
    float fitness;
    
    // Pop a single solution from the queue
    if (gaEngine->getParameterBridge()->pop(params, fitness))
    {
        targetParameters = params;
        isInterpolating = true;
        lastGAFitness = fitness;
        presetLoadTime = juce::Time::getCurrentTime();
        return true;
    }
    
    return false;
}

float JX11AudioProcessor::getPlayTimeSeconds() const
{
    return static_cast<float>((juce::Time::getCurrentTime() - presetLoadTime).inSeconds());
}

int JX11AudioProcessor::getNumCandidatesAvailable() const
{
    if (!gaEngine || !gaEngine->getParameterBridge())
        return 0;
        
    return gaEngine->getParameterBridge()->getNumAvailable();
}

void JX11AudioProcessor::interpolateAndApplyParameters()
{
    // Calculate exponential smoothing coefficient
    // alpha = 1 - e^(-dt/tau) where dt = timer interval, tau = smoothing time
    float alpha = 1.0f - std::exp(-timerInterval / parameterSmoothingTime);
    
    bool allParamsReached = true;
    const float threshold = 0.001f;  // Consider "reached" if within 0.1%
    
    // Array of 17 GA-controlled parameter pointers (matches HeadlessParam order)
    // Excluded: oscTune, glideMode, glideRate, glideBend, filterVelocity, octave, tuning, outputLevel, polyMode
    juce::RangedAudioParameter* params[17] =
    {
        oscMixParam, oscFineParam,
        filterFreqParam, filterResoParam, filterEnvParam, filterLFOParam,
        filterAttackParam, filterDecayParam, filterSustainParam, filterReleaseParam,
        envAttackParam, envDecayParam, envSustainParam, envReleaseParam,
        lfoRateParam, vibratoParam, noiseParam
    };
    
    // Interpolate and apply each GA parameter
    for (int i = 0; i < 17; ++i)
    {
        // Exponential smoothing toward target
        currentParameters[i] += (targetParameters[i] - currentParameters[i]) * alpha;
        
        // Check if we've reached the target
        if (std::abs(targetParameters[i] - currentParameters[i]) > threshold)
        {
            allParamsReached = false;
        }
        
        // Apply to APVTS (already normalized [0,1])
        params[i]->setValueNotifyingHost(currentParameters[i]);
    }
    
    // If all parameters reached target, stop interpolating to save CPU
    if (allParamsReached)
    {
        isInterpolating = false;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JX11AudioProcessor();
}
