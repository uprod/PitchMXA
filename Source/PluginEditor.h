#pragma once

/*  IMPECCABLE DIRECTION CONTRACT — seed 5bcea053 (roll: assigned)
    THESIS: The panel IS the signal path — a service-manual schematic read as
    the circuit you hear; refuses knobs-on-a-metal-plate.
    OWN-WORLD: Diazo film negative — dark drafting film #17140F, pale ink
    #E6DCC2, spot emerald-green #44C493 (one spot ink per MXA sibling).
    Routed Gothic drafting lettering + Courier Prime figures, double sheet
    border, title block, FIG. captions.
    STORY: A producer reads the schematic, watches the splice heads run
    their sawtooth relay in FIG. 1, and trusts every figure at a glance.
    FIRST VIEWPORT: Header + title block; FIG. 1 live splice-head
    trajectories (the slope IS the shift) full width; FIG. 2 signal path
    with the splice drum — fixed write head, two read heads at their REAL
    rotating positions —, reshifting feedback loop, real ratio printed; six
    schematic dials beneath.
    SIGNATURE: the relay — FIG. 1's sawteeth and FIG. 2's read heads on one
    30 Hz clock, driven by the engine's real tap phase.
    FORM: Service Manual family template, adopted from PhaserMXA, seed 5bcea053.
    FINISH: unreviewed and undocumented is unfinished; this build ends with
    the finish review, the verdict, DESIGN.md, and every shipping raster
    carrying its provenance.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ManualStyle.h"
#include "SplicePlot.h"
#include "SchematicDiagram.h"

namespace pitchmxa
{

class PitchEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit PitchEditor (PitchProcessor& proc);
    ~PitchEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    using APVTS   = juce::AudioProcessorValueTreeState;
    using SAttach = APVTS::SliderAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label  name;
        std::unique_ptr<SAttach> attachment;
    };

    void setupDial (Dial& d, const juce::String& labelText, const juce::String& paramID);
    void timerCallback() override;

    void drawSheetFrame (juce::Graphics& g);
    void drawHeader (juce::Graphics& g);

    PitchProcessor& processor;

    ManualLookAndFeel lookAndFeel;
    juce::Image       filmTexture;

    SplicePlot       plot;
    SchematicDiagram schematic;

    Dial pitchDial, fineDial, grainDial, snapSwitch, fbDial, mixDial;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchEditor)
};

}
