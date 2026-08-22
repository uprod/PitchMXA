#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace pitchmxa
{

namespace IDs
{
    constexpr auto pitch = "pitch";
    constexpr auto fine  = "fine";
    constexpr auto grain = "grain";
    constexpr auto snap  = "snap";
    constexpr auto fb    = "fb";
    constexpr auto mix   = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout PitchProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Affichages "tally" a la machine a ecrire, aussi bien dans l'editeur que
    // dans les lignes d'automation de l'hote.
    const auto stAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int)
            { return juce::String (v >= 0.05f ? "+" : "") + juce::String (v, 1) + " st"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto ctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int)
            { return juce::String (v >= 0.5f ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto msAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::pitch, 1 },
        "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, stAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::fine, 1 },
        "Fine", juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f, ctAttr));

    // Fenetre d'epissure : courte = percussif net, longue = nappe lisse.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::grain, 1 },
        "Grain", juce::NormalisableRange<float> (20.0f, 120.0f, 1.0f, 0.7f), 60.0f, msAttr));

    // Quantification du potard PITCH : libre, demi-tons, ou octaves.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::snap, 1 }, "Snap",
        juce::StringArray { "Free", "Semitone", "Octave" }, 1));

    // Feedback : chaque repetition se transpose a nouveau (la spirale).
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::fb, 1 },
        "Feedback", juce::NormalisableRange<float> (0.0f, 0.7f, 0.001f), 0.0f, pctAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::mix, 1 },
        "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, pctAttr));

    return { params.begin(), params.end() };
}

PitchProcessor::PitchProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

PitchProcessor::~PitchProcessor() = default;

void PitchProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engine.reset();
}

void PitchProcessor::releaseResources()
{
    engine.reset();
}

bool PitchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void PitchProcessor::pushParameterUpdatesToEngine()
{
    engine.setPitchSemis (apvts.getRawParameterValue (IDs::pitch)->load());
    engine.setFineCents  (apvts.getRawParameterValue (IDs::fine)->load());
    engine.setGrainMs    (apvts.getRawParameterValue (IDs::grain)->load());
    engine.setSnap       (juce::roundToInt (apvts.getRawParameterValue (IDs::snap)->load()));
    engine.setFeedback   (apvts.getRawParameterValue (IDs::fb)->load());
    engine.setMix        (apvts.getRawParameterValue (IDs::mix)->load());
}

void PitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();

    // Le pitch shifter agit en place, dry/wet compris : pas de copie de travail.
    engine.process (buffer);
}

juce::AudioProcessorEditor* PitchProcessor::createEditor()
{
    return new PitchEditor (*this);
}

void PitchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PitchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pitchmxa::PitchProcessor();
}
