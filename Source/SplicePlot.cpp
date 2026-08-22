#include "SplicePlot.h"
#include "ManualStyle.h"
#include "PitchEngine.h"

namespace pitchmxa
{

namespace
{
    constexpr int kPoints = 300;
}

SplicePlot::SplicePlot (PitchProcessor& proc)
    : processor (proc)
{
    grain = processor.getAPVTS().getRawParameterValue ("grain");

    setInterceptsMouseClicks (false, false);
}

void SplicePlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    const float grainV = grain->load();
    const float ratio  = processor.getRatioLive();
    const float phase  = processor.getTapPhase01();

    // Vitesse des tetes (fenetres/seconde) et fenetre de defilement : deux
    // periodes d'epissure, bornees pour rester lisibles.
    const float rate    = (1.0f - ratio) / (grainV * 0.001f);   // dp/dt
    const float absRate = std::abs (rate);
    const float windowS = absRate < 0.05f ? 1.0f
                        : juce::jlimit (0.25f, 3.0f, 2.0f / absRate);

    auto yForMs = [&] (float ms)
    {
        return box.getBottom() - 8.0f - (ms / grainV) * (box.getHeight() - 16.0f);
    };
    auto xForTimeBack = [&] (float tBack)
    {
        return box.getRight() - 1.0f - tBack / windowS * (box.getWidth() - 2.0f);
    };

    // --- Grille ------------------------------------------------------------
    for (int i = 1; i <= 3; ++i)
    {
        g.setColour (palette::inkFaint);
        g.drawVerticalLine ((int) xForTimeBack ((float) i * windowS * 0.25f),
                            box.getY() + 1.0f, box.getBottom() - 1.0f);
    }
    static const float fracs[] = { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f };
    for (const float fr : fracs)
    {
        g.setColour (fr == 0.0f ? palette::inkMid.withAlpha (0.65f) : palette::inkFaint);
        g.drawHorizontalLine ((int) yForMs (fr * grainV), box.getX() + 1.0f, box.getRight() - 1.0f);
    }

    // --- Trajectoires : dents de scie reconstruites au taux reel ----------------
    // La dent de scie casse au bouclage : on coupe le trace a chaque tour.
    auto buildTap = [&] (float phaseOffset, juce::Path& p)
    {
        float prev = -1.0f;
        for (int i = 0; i < kPoints; ++i)
        {
            const float tBack = windowS * (1.0f - (float) i / (float) (kPoints - 1));
            float ph = phase + phaseOffset - tBack * rate;
            ph -= std::floor (ph);
            const float px = xForTimeBack (tBack);
            const float py = yForMs (ph * grainV);

            if (prev < 0.0f || std::abs (ph - prev) > 0.5f)
                p.startNewSubPath (px, py);
            else
                p.lineTo (px, py);
            prev = ph;
        }
    };

    // Tete B : encre pale, trait tirete (l'etat survit au niveau de gris).
    {
        juce::Path b, dashed;
        buildTap (0.5f, b);
        const float dashes[] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.1f).createDashedStroke (dashed, b, dashes, 2);
        g.setColour (palette::ink.withAlpha (0.85f));
        g.fillPath (dashed);
    }

    // Repere de l'instant present : index vertical au bord droit du cadre.
    {
        const float sx = box.getRight() - 5.0f;
        g.setColour (palette::spot.withAlpha (0.30f));
        g.drawVerticalLine ((int) sx, box.getY() + 1.0f, box.getBottom() - 1.0f);

        juce::Path idx;   // petit index triangulaire en haut
        idx.addTriangle (sx - 3.5f, box.getY() + 1.0f, sx + 3.5f, box.getY() + 1.0f, sx, box.getY() + 7.0f);
        g.setColour (palette::spot);
        g.fillPath (idx);
    }

    // Tete A : encre spot, trait plein.
    {
        juce::Path a;
        buildTap (0.0f, a);
        g.setColour (palette::spot);
        g.strokePath (a, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
    }

    // Points "instant present" : plein (A), anneau (B).
    {
        const float dx = box.getRight() - 5.0f;
        float p2 = phase + 0.5f;
        p2 -= std::floor (p2);
        g.setColour (palette::spot);
        g.fillEllipse (dx - 2.2f, yForMs (phase * grainV) - 2.2f, 4.4f, 4.4f);
        g.drawEllipse (dx - 2.4f, yForMs (p2 * grainV) - 2.4f, 4.8f, 4.8f, 1.1f);
    }

    // --- Echelles ----------------------------------------------------------
    // Chaque chiffre est pose sur un cartouche film : ni le grain ni la grille
    // ne peuvent corrompre une valeur que le regard doit pouvoir croire.
    auto drawFigure = [&g] (const juce::String& text, juce::Point<float> anchor,
                            juce::Justification just)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f).withCentre (anchor);
        if (just.testFlags (juce::Justification::left))
            area.setX (anchor.x - 3.0f);
        else if (just.testFlags (juce::Justification::right))
            area.setX (anchor.x - tw - 3.0f);

        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    };

    for (int i = 1; i <= 3; ++i)
    {
        const float tBack = (float) i * windowS * 0.25f;
        drawFigure ("-" + juce::String (tBack, 2),
                    { xForTimeBack (tBack), box.getBottom() - 8.0f },
                    juce::Justification::centred);
    }
    drawFigure ("s", { box.getRight() - 6.0f, box.getBottom() - 8.0f }, juce::Justification::right);

    drawFigure (juce::String (juce::roundToInt (grainV)) + " ms",
                { box.getX() + 6.0f, yForMs (grainV) + 7.0f }, juce::Justification::left);
    drawFigure (juce::String (juce::roundToInt (grainV * 0.5f)),
                { box.getX() + 6.0f, yForMs (grainV * 0.5f) - 6.0f }, juce::Justification::left);
    drawFigure ("0", { box.getX() + 6.0f, yForMs (0.0f) - 6.0f }, juce::Justification::left);

    // Legende des tetes en haut a gauche.
    {
        float lx = box.getX() + 34.0f;
        const float ly = box.getY() + 12.0f;
        g.setFont (fonts::mono (9.0f));

        g.setColour (palette::spot);
        g.drawLine (lx, ly, lx + 14.0f, ly, 1.6f);
        g.setColour (palette::inkMid);
        g.drawText ("HEAD A", juce::Rectangle<float> (50.0f, 10.0f).withPosition (lx + 18.0f, ly - 5.0f),
                    juce::Justification::centredLeft);

        lx += 84.0f;
        g.setColour (palette::ink.withAlpha (0.85f));
        g.drawLine (lx, ly, lx + 4.0f, ly, 1.1f);
        g.drawLine (lx + 7.0f, ly, lx + 11.0f, ly, 1.1f);
        g.setColour (palette::inkMid);
        g.drawText ("HEAD B", juce::Rectangle<float> (50.0f, 10.0f).withPosition (lx + 15.0f, ly - 5.0f),
                    juce::Justification::centredLeft);
    }

    // Tallies : transposition reelle (issue du rapport lisse) et fenetre.
    {
        const float semis = PitchEngine::semisFor (ratio);
        auto tally = juce::Rectangle<float> (120.0f, 12.0f)
                         .withPosition (box.getRight() - 126.0f, box.getY() + 6.0f);
        g.setColour (palette::film);
        g.fillRect (tally.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("shift", tally.removeFromLeft (42.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText ((semis >= 0.0f ? "+" : "") + juce::String (semis, 2) + " st",
                    tally, juce::Justification::centredLeft);

        auto tally2 = juce::Rectangle<float> (120.0f, 12.0f)
                          .withPosition (box.getRight() - 126.0f, box.getY() + 20.0f);
        g.setColour (palette::film);
        g.fillRect (tally2.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("ratio", tally2.removeFromLeft (42.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (juce::String (ratio, 3), tally2, juce::Justification::centredLeft);
    }

    // --- Cadre + legende de figure ------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - SPLICE HEAD TRAJECTORIES, AT LIVE RATE";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
