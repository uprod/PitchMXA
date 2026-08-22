#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchEngine.h"

namespace pitchmxa
{

class PitchProcessor : public juce::AudioProcessor
{
public:
    PitchProcessor();
    ~PitchProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PitchMXA"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Rapport reel et phase de tete pour l'affichage temps reel de l'editeur.
    float getRatioLive() const noexcept  { return engine.getRatioLive(); }
    float getTapPhase01() const noexcept { return engine.getTapPhase01(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParameterUpdatesToEngine();

    juce::AudioProcessorValueTreeState apvts;
    PitchEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchProcessor)
};

}
