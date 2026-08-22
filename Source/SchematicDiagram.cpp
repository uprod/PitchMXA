#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "PitchEngine.h"

namespace pitchmxa
{

namespace
{
    // Epaisseur de trait proportionnelle a une quantite 0..1 : la geometrie
    // porte la valeur, jamais la couleur seule.
    float weightFor (float amount01)
    {
        return 0.7f + 2.4f * juce::jlimit (0.0f, 1.0f, amount01);
    }

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawDashedLine (juce::Graphics& g, juce::Line<float> line, float thickness)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (line, dashes, 2, thickness);
    }

    // Etiquette imprimee qui interrompt le trait qu'elle chevauche.
    void drawLabelOverLine (juce::Graphics& g, const juce::String& text,
                            juce::Rectangle<float> area, juce::Justification just)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (fonts::lettering (9.0f), text);
        auto knockout = area.withSizeKeepingCentre (tw + 10.0f, area.getHeight());
        g.setColour (palette::film);
        g.fillRect (knockout);
        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText (text, area, just);
    }

    // Croix de sommateur dans un cercle (jonction "+" du schema).
    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }

    // Rail pondere : epaisseur = quantite ; a zero il degenere en tirete fin.
    void drawWeightedLine (juce::Graphics& g, juce::Line<float> line, float amount01)
    {
        if (amount01 < 0.005f)
            drawDashedLine (g, line, 0.7f);
        else
            g.drawLine (line, weightFor (amount01));
    }
}

SchematicDiagram::SchematicDiagram (PitchProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    grain = apvts.getRawParameterValue ("grain");
    fb    = apvts.getRawParameterValue ("fb");
    mix   = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const float grainV = grain->load();
    const float fbV    = fb->load();
    const float mixV   = mix->load();

    const float ratio = processor.getRatioLive();
    const float p1    = processor.getTapPhase01();
    float p2 = p1 + 0.5f;
    p2 -= std::floor (p2);

    // Rangs horizontaux du schema.
    const float fbY   = 10.0f;   // boucle de retour
    const float railY = 36.0f;   // rail principal (tambour)
    const float mixY  = 58.0f;   // sommateur de mix
    const float dryY  = 74.0f;   // rail dry

    // Colonnes.
    const float inX     = 12.0f;
    const float branchX = 36.0f;
    const float sumFbX  = 100.0f;
    const float drumCx  = 250.0f;
    const float drumR   = 16.0f;
    const float tapX    = w * 0.635f;
    const float mixX    = w * 0.86f;
    const float outX    = w - 16.0f;

    const float dryW = weightFor (1.0f - mixV);

    // --- Rail d'entree et derivation dry ------------------------------------
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, railY - 3.0f, 6.0f, 6.0f, 1.1f);              // borne IN
    g.drawLine (inX + 3.0f, railY, sumFbX - 8.0f, railY, 1.2f);
    g.fillEllipse (branchX - 2.2f, railY - 2.2f, 4.4f, 4.4f);                // noeud de derivation

    g.setColour (palette::ink.withAlpha (0.9f));
    g.drawLine (branchX, railY, branchX, dryY, dryW * 0.75f + 0.4f);         // descente dry
    g.drawLine (branchX, dryY, mixX, dryY, dryW);                            // rail dry
    g.drawLine (mixX, dryY, mixX, mixY + 1.0f, dryW);                        // remontee vers le mix

    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("IN", juce::Rectangle<float> (24.0f, 10.0f).withPosition (inX - 8.0f, railY - 17.0f),
                juce::Justification::centredLeft);
    g.drawText ("DRY", juce::Rectangle<float> (30.0f, 10.0f).withPosition (branchX + 8.0f, dryY - 13.0f),
                juce::Justification::centredLeft);

    // --- Sommateur de feedback ----------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (sumFbX + 7.0f, railY, drumCx - drumR, railY, 1.2f);
    drawArrowHead (g, { drumCx - drumR, railY }, { 1.0f, 0.0f }, 6.0f);
    drawSummingNode (g, { sumFbX, railY }, 7.0f);

    // --- Le tambour d'epissure : tetes aux positions reelles -------------------
    {
        const juce::Point<float> c (drumCx, railY);

        g.setColour (palette::film);
        g.fillEllipse (c.x - drumR, c.y - drumR, drumR * 2.0f, drumR * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - drumR, c.y - drumR, drumR * 2.0f, drumR * 2.0f, 1.2f);

        // Tete d'ecriture, fixe en haut du tambour.
        g.drawLine (c.x, c.y - drumR - 6.0f, c.x, c.y - drumR + 1.0f, 1.2f);
        drawArrowHead (g, { c.x, c.y - drumR + 2.0f }, { 0.0f, 1.0f }, 4.5f);
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText ("W", juce::Rectangle<float> (14.0f, 10.0f)
                        .withPosition (c.x + 4.0f, c.y - drumR - 12.0f),
                    juce::Justification::centredLeft);

        // Tetes de lecture A (pleine) et B (anneau), aux phases reelles.
        auto headAt = [&] (float p, bool filled)
        {
            const float a = p * juce::MathConstants<float>::twoPi;
            const juce::Point<float> dir (std::sin (a), -std::cos (a));
            const auto pos = c + dir * (drumR - 4.5f);
            g.setColour (palette::spot);
            if (filled) g.fillEllipse (pos.x - 2.6f, pos.y - 2.6f, 5.2f, 5.2f);
            else        g.drawEllipse (pos.x - 2.8f, pos.y - 2.8f, 5.6f, 5.6f, 1.2f);
        };
        headAt (p1, true);
        headAt (p2, false);

        g.setColour (palette::ink);
        g.fillEllipse (c.x - 2.0f, c.y - 2.0f, 4.0f, 4.0f);

        // La fenetre reelle, imprimee sous le tambour.
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText ("GRAIN " + juce::String (juce::roundToInt (grainV)) + " ms",
                    juce::Rectangle<float> (90.0f, 10.0f).withCentre ({ drumCx, railY + drumR + 8.0f }),
                    juce::Justification::centred);
    }

    g.setColour (palette::ink);
    g.drawLine (drumCx + drumR, railY, tapX, railY, 1.2f);

    // --- Boucle de feedback : l'epaisseur EST le feedback ----------------------
    g.fillEllipse (tapX - 2.2f, railY - 2.2f, 4.4f, 4.4f);                   // noeud de reprise
    {
        const float fbAmt = fbV / 0.7f;
        g.setColour (palette::ink);
        if (fbAmt < 0.005f)
        {
            drawDashedLine (g, { { tapX, railY }, { tapX, fbY } }, 0.7f);
            drawDashedLine (g, { { tapX, fbY }, { sumFbX, fbY } }, 0.7f);
            drawDashedLine (g, { { sumFbX, fbY }, { sumFbX, railY - 7.0f } }, 0.7f);
        }
        else
        {
            const float fbW = weightFor (fbAmt);
            g.drawLine (tapX, railY, tapX, fbY, fbW);
            g.drawLine (tapX, fbY, sumFbX, fbY, fbW);
            g.drawLine (sumFbX, fbY, sumFbX, railY - 7.0f, fbW);
        }
        drawArrowHead (g, { sumFbX, railY - 7.0f }, { 0.0f, 1.0f }, 6.0f);

        // L'etiquette interrompt le trait de la boucle : chaque tour transpose.
        drawLabelOverLine (g, "FEEDBACK " + juce::String (juce::roundToInt (fbV * 100.0f))
                               + " % - RESHIFTED",
                           juce::Rectangle<float> (170.0f, 10.0f)
                               .withCentre ({ (drumCx + tapX) * 0.5f, fbY }),
                           juce::Justification::centred);
    }

    // --- Rail wet vers le sommateur de mix : le rapport reel imprime -----------
    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { tapX, railY }, { mixX, railY } }, mixV);
    drawWeightedLine (g, { { mixX, railY }, { mixX, mixY - 1.0f } }, mixV);

    {
        const float semis = PitchEngine::semisFor (ratio);
        const juce::String note = "RATIO " + juce::String (ratio, 3) + "   "
            + (semis >= 0.0f ? "+" : "") + juce::String (semis, 1) + " ST";
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::ink);
        g.drawText (note, juce::Rectangle<float> (170.0f, 10.0f)
                        .withPosition (tapX + 14.0f, railY - 14.0f),
                    juce::Justification::centredLeft);
    }

    drawSummingNode (g, { mixX, mixY }, 8.0f);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("MIX", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX + 12.0f, mixY - 20.0f),
                juce::Justification::centredLeft);

    // --- Sortie --------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (mixX + 8.0f, mixY, outX - 3.0f, mixY, 1.4f);
    g.fillEllipse (outX - 3.0f, mixY - 3.0f, 6.0f, 6.0f);                    // borne OUT
    g.setColour (palette::inkMid);
    g.drawText ("OUT", juce::Rectangle<float> (28.0f, 10.0f).withPosition (outX - 24.0f, mixY - 17.0f),
                juce::Justification::centredRight);

    // --- Legende de figure ----------------------------------------------------
    const juce::String cap = "FIG. 2 - SIGNAL PATH, ROTATING SPLICE HEADS";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
