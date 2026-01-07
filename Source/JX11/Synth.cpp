/*
  ==============================================================================
    Synth.cpp
    Created: 27 Nov 2024 6:58:30pm
    Author:  Daniel Lister

    Implements the core synthesizer logic, including voice rendering, LFOs,
    pitch/glide, filter control, and MIDI handling.
  ==============================================================================
*/

#include "Synth.h"
#include "Utils.h"

// Analog detune and special marker for sustained notes
static const float ANALOG = 0.002f;
static const int SUSTAIN = -1;

Synth::Synth()
{
    sampleRate = 44100.0f;
}

// Set sample rate and propagate to filters
void Synth::allocateResources(double sampleRate_, int)
{
    sampleRate = static_cast<float>(sampleRate_);
    for (int v = 0; v < MAX_VOICES; ++v)
        voices[v].filter.sampleRate = sampleRate;
}

void Synth::deallocateResources() {} // No heap allocations to free

// Reset all voices and modulation state
void Synth::reset()
{
    for (int v = 0; v < MAX_VOICES; ++v)
        voices[v].reset();

    noiseGen.reset();
    pitchBend = 1.0f;
    outputLevelSmoother.reset(sampleRate, 0.05f);
    sustainPedalPressed = false;
    lfo = 0.0f;
    lfoStep = 0;
    modWheel = 0.0f;
    lastNote = 0;
    resonanceCtl = 1.0f;
    filterCtl = 0.0f;
    pressure = 0.0f;
    filterZip = 0.0f;
}

// Render stereo output for N samples
void Synth::render(float** outputBuffers, int sampleCount)
{
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];

    // Preprocess active voices
    for (int v = 0; v < MAX_VOICES; ++v)
    {
        Voice& voice = voices[v];
        if (voice.env.isActive())
        {
            updatePeriod(voice);
            voice.glideRate = glideRate;
            voice.filterQ = filterQ * resonanceCtl;
            voice.pitchBend = pitchBend;
            voice.filterEnvDepth = filterEnvDepth;
        }
    }

    // Audio rendering loop
    for (int sample = 0; sample < sampleCount; ++sample)
    {
        updateLFO();
        float noise = noiseGen.nextValue() * noiseMix;

        float outputLeft = 0.0f;
        float outputRight = 0.0f;

        for (int v = 0; v < MAX_VOICES; ++v)
        {
            Voice& voice = voices[v];
            if (voice.env.isActive())
            {
                float output = voice.render(noise);
                outputLeft += output * voice.panLeft;
                outputRight += output * voice.panRight;
            }
        }

        float outputLevel = outputLevelSmoother.getNextValue();
        outputLeft *= outputLevel;
        outputRight *= outputLevel;

        if (outputBufferRight != nullptr)
        {
            outputBufferLeft[sample] = outputLeft;
            outputBufferRight[sample] = outputRight;
        }
        else
        {
            outputBufferLeft[sample] = 0.5f * (outputLeft + outputRight);
        }
    }

    // Reset inactive voices to free them
    for (int v = 0; v < MAX_VOICES; ++v)
    {
        if (!voices[v].env.isActive())
        {
            voices[v].env.reset();
            voices[v].filter.reset();
        }
    }

    // Optional: prevent NaNs/clipping
    protectYourEars(outputBufferLeft, sampleCount);
    protectYourEars(outputBufferRight, sampleCount);
}

// Handle incoming note-on event
void Synth::noteOn(int note, int velocity)
{
    if (ignoreVelocity) velocity = 80;

    if (numVoices == 1)
    {
        if (voices[0].note > 0)
        {
            shiftQueuedNotes();
            restartMonoVoice(note, velocity);
            return;
        }
    }

    int v = (numVoices == 1) ? 0 : findFreeVoice();
    startVoice(v, note, velocity);
}

// Start or restart a voice with a given note and velocity
void Synth::startVoice(int v, int note, int velocity)
{
    float period = calcPeriod(v, note);
    Voice& voice = voices[v];
    voice.target = period;

    int noteDistance = 0;
    if (lastNote > 0 && (glideMode == 2 || (glideMode == 1 && isPlayingLegatoStyle())))
        noteDistance = note - lastNote;

    voice.period = period * std::pow(1.0594631f, float(noteDistance) - glideBend);
    voice.period = std::max(voice.period, 6.0f); // Limit low pitch

    lastNote = note;
    voice.note = note;
    voice.updatePanning();

    voice.cutoff = sampleRate / (period * PI);
    voice.cutoff *= std::exp(velocitySensitivity * float(velocity - 64));

    float vel = 0.004f * float((velocity + 64) * (velocity + 64)) - 8.0f;
    voice.osc1.amplitude = volumeTrim * vel;
    voice.osc2.amplitude = voice.osc1.amplitude * oscMix;

    // Use osc2 as PWM if vibrato is zero
    if (vibrato == 0.0f && pwmDepth > 0.0f)
        voice.osc2.squareWave(voice.osc1, voice.period);

    // Start envelopes
    voice.env.attackMultiplier = envAttack;
    voice.env.decayMultiplier = envDecay;
    voice.env.sustainLevel = envSustain;
    voice.env.releaseMultiplier = envRelease;
    voice.env.attack();

    voice.filterEnv.attackMultiplier = filterAttack;
    voice.filterEnv.decayMultiplier = filterDecay;
    voice.filterEnv.sustainLevel = filterSustain;
    voice.filterEnv.releaseMultiplier = filterRelease;
    voice.filterEnv.attack();
}

// Handle note-off event
void Synth::noteOff(int note)
{
    if (numVoices == 1 && voices[0].note == note)
    {
        int queuedNote = nextQueuedNote();
        if (queuedNote > 0)
            restartMonoVoice(queuedNote, -1);
    }

    for (int v = 0; v < MAX_VOICES; ++v)
    {
        if (voices[v].note == note)
        {
            if (sustainPedalPressed)
                voices[v].note = SUSTAIN;
            else
            {
                voices[v].release();
                voices[v].note = 0;
            }
        }
    }
}

// Interpret incoming MIDI messages
void Synth::midiMessage(uint8_t data0, uint8_t data1, uint8_t data2)
{
    switch (data0 & 0xF0)
    {
        case 0xB0: controlChange(data1, data2); break;            // CC
        case 0xD0: pressure = 0.0001f * float(data1 * data2); break; // Aftertouch
        case 0xE0: pitchBend = std::exp(-0.000014102f * float(data1 + 128 * data2 - 8192)); break;
        case 0x80: noteOff(data1 & 0x7F); break;
        case 0x90:
        {
            uint8_t note = data1 & 0x7F;
            uint8_t velo = data2 & 0x7F;
            (velo > 0) ? noteOn(note, velo) : noteOff(note);
            break;
        }
    }
}

// Compute oscillator period from MIDI note
float Synth::calcPeriod(int v, int note) const
{
    float period = tune * std::exp(-0.05776226505f * (note + ANALOG * v));
    while (period < 6.0f || (period * detune) < 6.0f)
        period += period;
    return period;
}

// Pick voice with lowest envelope level and not attacking
int Synth::findFreeVoice() const
{
    int v = 0;
    float lowest = 100.0f;
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (voices[i].env.level < lowest && !voices[i].env.isInAttack())
        {
            lowest = voices[i].env.level;
            v = i;
        }
    }
    return v;
}

// Handle MIDI CC messages
void Synth::controlChange(uint8_t data1, uint8_t data2)
{
    switch (data1)
    {
        case 0x01: modWheel = 0.000005f * data2 * data2; break;
        case 0x40: sustainPedalPressed = (data2 >= 64);
                   if (!sustainPedalPressed) noteOff(SUSTAIN); break;
        case 0x47: resonanceCtl = 154.0f / float(154 - data2); break;
        case 0x4A: filterCtl = 0.02f * data2; break;
        case 0x4B: filterCtl = -0.03f * data2; break;
        default:
            if (data1 >= 0x78)
            {
                for (auto& v : voices) v.reset();
                sustainPedalPressed = false;
            }
            break;
    }
}

// Restart mono voice (e.g. for legato re-trigger)
void Synth::restartMonoVoice(int note, int velocity)
{
    float period = calcPeriod(0, note);
    Voice& voice = voices[0];
    voice.target = period;
    if (glideMode == 0) voice.period = period;

    voice.env.level += SILENCE + SILENCE;
    voice.note = note;
    voice.updatePanning();

    voice.cutoff = sampleRate / (period * PI);
    if (velocity > 0)
        voice.cutoff *= std::exp(velocitySensitivity * (velocity - 64));
}

// Push note queue in mono mode
void Synth::shiftQueuedNotes()
{
    for (int i = MAX_VOICES - 1; i > 0; --i)
    {
        voices[i].note = voices[i - 1].note;
        voices[i].release();
    }
}

// Return next note in queue (mono mode)
int Synth::nextQueuedNote()
{
    int held = 0;
    for (int v = MAX_VOICES - 1; v > 0; --v)
        if (voices[v].note > 0) held = v;

    if (held > 0)
    {
        int note = voices[held].note;
        voices[held].note = 0;
        return note;
    }
    return 0;
}

// Update LFO and apply to modulation targets
void Synth::updateLFO()
{
    if (--lfoStep <= 0)
    {
        lfoStep = LFO_MAX;
        lfo += lfoInc;
        if (lfo > PI) lfo -= TWO_PI;

        float sine = std::sin(lfo);
        float vibratoMod = 1.0f + sine * (modWheel + vibrato);
        float pwm = 1.0f + sine * (modWheel + pwmDepth);
        float filterMod = filterKeyTracking + filterCtl + (filterLFODepth + pressure) * sine;

        filterZip += 0.005f * (filterMod - filterZip);

        for (int v = 0; v < MAX_VOICES; ++v)
        {
            Voice& voice = voices[v];
            if (voice.env.isActive())
            {
                voice.osc1.modulation = vibratoMod;
                voice.osc2.modulation = pwm;
                voice.filterMod = filterZip;
                voice.updateLFO();
                updatePeriod(voice);
            }
        }
    }
}

// Determine if another note is currently held
bool Synth::isPlayingLegatoStyle() const
{
    int held = 0;
    for (const auto& voice : voices)
        if (voice.note > 0) ++held;

    return held > 0;
}
