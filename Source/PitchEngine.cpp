#include "PitchEngine.h"

namespace pitchmxa
{

PitchEngine::PitchEngine() = default;

void PitchEngine::prepare (double newSampleRate, int /*blockSize*/, int numChannels)
{
    sampleRate = newSampleRate;
    numCh = juce::jlimit (1, kMaxCh, numChannels);

    // 250 ms de tampon : au-dela de la fenetre max (120 ms), large marge.
    const int size = (int) (0.250 * sampleRate) + 8;
    for (auto& b : buf)
        b.assign ((size_t) size, 0.0f);

    kSlew = 1.0f - std::exp (-1.0f / (0.030f * (float) sampleRate));
    kSlow = 1.0f - std::exp (-1.0f / (0.010f * (float) sampleRate));

    reset();
}

void PitchEngine::reset()
{
    for (auto& b : buf)
        std::fill (b.begin(), b.end(), 0.0f);
    writePos   = 0;
    tapPhase   = 0.0f;
    ratioState = ratioFor (quantizedSemis (pitchSemis, snap) + fineCents / 100.0f);
    grainState = grainMs;
    mixState   = mix;
    lastOut[0] = lastOut[1] = 0.0f;
}

void PitchEngine::setPitchSemis (float s)   { pitchSemis = juce::jlimit (-12.0f, 12.0f, s); }
void PitchEngine::setFineCents (float c)    { fineCents  = juce::jlimit (-100.0f, 100.0f, c); }
void PitchEngine::setGrainMs (float ms)     { grainMs    = juce::jlimit (20.0f, 120.0f, ms); }
void PitchEngine::setSnap (int m)           { snap       = juce::jlimit (0, 2, m); }
void PitchEngine::setFeedback (float a)     { feedback   = juce::jlimit (0.0f, 0.7f, a); }
void PitchEngine::setMix (float a)          { mix        = juce::jlimit (0.0f, 1.0f, a); }

void PitchEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int chs        = juce::jmin (numCh, buffer.getNumChannels());
    const int size       = (int) buf[0].size();

    const float ratioTarget = ratioFor (quantizedSemis (pitchSemis, snap)
                                        + fineCents / 100.0f);

    auto readTap = [size] (const std::vector<float>& b, int w, float delaySamples)
    {
        float rp = (float) w - delaySamples;
        while (rp < 0.0f) rp += (float) size;
        int i0 = (int) rp;
        if (i0 >= size) i0 -= size;
        const int   i1   = i0 + 1 >= size ? 0 : i0 + 1;
        const float frac = rp - std::floor (rp);
        return b[(size_t) i0] + frac * (b[(size_t) i1] - b[(size_t) i0]);
    };

    for (int n = 0; n < numSamples; ++n)
    {
        ratioState += kSlew * (ratioTarget - ratioState);
        grainState += kSlew * (grainMs - grainState);
        mixState   += kSlow * (mix - mixState);

        const float wSamp = grainState * 0.001f * (float) sampleRate;

        // Les tetes avancent a la vitesse relative (1 - ratio) : vers la tete
        // d'ecriture pour monter, en s'en eloignant pour descendre.
        tapPhase += (1.0f - ratioState) / wSamp;
        tapPhase -= std::floor (tapPhase);

        const float p1 = tapPhase;
        float p2 = tapPhase + 0.5f;
        p2 -= std::floor (p2);

        const float d1 = 1.0f + p1 * wSamp;
        const float d2 = 1.0f + p2 * wSamp;
        const float g1 = fadeGainFor (p1);
        const float g2 = fadeGainFor (p2);

        for (int ch = 0; ch < chs; ++ch)
        {
            const float x = buffer.getSample (ch, n);
            buf[ch][(size_t) writePos] = x + feedback * lastOut[ch];

            const float wet = g1 * readTap (buf[ch], writePos, d1)
                            + g2 * readTap (buf[ch], writePos, d2);
            lastOut[ch] = wet;

            buffer.setSample (ch, n, (1.0f - mixState) * x + mixState * wet);
        }

        if (++writePos >= size) writePos = 0;
    }

    uiRatio.store (ratioState, std::memory_order_relaxed);   // pour FIG. 1 / FIG. 2
    uiPhase.store (tapPhase, std::memory_order_relaxed);
}

}
