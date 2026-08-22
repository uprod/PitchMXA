#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace pitchmxa
{

// FIG. 2 - Le chemin du signal dessine comme dans le manuel : IN, sommateur de
// feedback, le tambour d'epissure — tete d'ecriture fixe, deux tetes de
// lecture dessinees A LEURS POSITIONS REELLES qui tournent a la vitesse
// relative de la transposition —, boucle de retour dont l'epaisseur EST le
// feedback, rails dry/wet ponderes par le mix, rapport reel imprime. La
// quantite est dessinee en geometrie : le schema est la valeur.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (PitchProcessor&);

    void paint (juce::Graphics&) override;

private:
    PitchProcessor& processor;

    std::atomic<float>* grain = nullptr;
    std::atomic<float>* fb    = nullptr;
    std::atomic<float>* mix   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
