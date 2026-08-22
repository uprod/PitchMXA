#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace pitchmxa
{

// FIG. 1 - Trajectoires des tetes d'epissure, tracees comme la figure d'un
// manuel technique. Ce n'est pas une illustration : chaque dent de scie est
// le retard reel d'une tete de lecture (position x fenetre), reconstruit au
// taux REEL du moteur — la PENTE est la transposition (descendante pour
// monter, montante pour descendre), et les deux tetes se relaient d'une
// demi-fenetre. Le repaint est pilote par le Timer de l'editeur (~30 Hz).
class SplicePlot : public juce::Component
{
public:
    explicit SplicePlot (PitchProcessor&);

    void paint (juce::Graphics&) override;

private:
    PitchProcessor& processor;

    std::atomic<float>* grain = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplicePlot)
};

}
