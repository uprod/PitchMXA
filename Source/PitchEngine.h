#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <vector>

namespace pitchmxa
{

// Coeur DSP du pitch shifter, methode "epissure de bande" : le son est ecrit
// dans un tampon circulaire, et deux tetes de lecture le relisent a une autre
// vitesse — plus vite pour monter, moins vite pour descendre. Chaque tete
// balaie la fenetre (GRAIN) en dents de scie ; les deux sont decalees d'une
// demi-fenetre et fondues en sin/cos (puissance constante) pour masquer les
// epissures. Le feedback renvoie le signal transpose a l'entree : chaque
// repetition se transpose a nouveau — la spirale des harmonizers d'epoque.
class PitchEngine
{
public:
    static constexpr int kMaxCh = 2;

    PitchEngine();

    void prepare (double sampleRate, int blockSize, int numChannels);
    void reset();

    void setPitchSemis (float semis);      // -12..+12
    void setFineCents (float cents);       // -100..+100
    void setGrainMs (float ms);            // 20..120
    void setSnap (int mode);               // 0 = libre, 1 = demi-ton, 2 = octave
    void setFeedback (float amount01);     // 0..0.7
    void setMix (float amount01);

    // Traite le buffer en place (dry/wet compris).
    void process (juce::AudioBuffer<float>& buffer);

    // --- Verites partagees avec l'UI (FIG. 1 / FIG. 2) ----------------------
    // Quantification du potard PITCH par le commutateur SNAP.
    static float quantizedSemis (float pitchSemis, int snapMode) noexcept
    {
        if (snapMode == 1) return std::round (pitchSemis);
        if (snapMode == 2) return std::round (pitchSemis / 12.0f) * 12.0f;
        return pitchSemis;
    }

    static float ratioFor (float semis) noexcept
    {
        return std::pow (2.0f, semis / 12.0f);
    }

    static float semisFor (float ratio) noexcept
    {
        return 12.0f * std::log2 (juce::jmax (1.0e-6f, ratio));
    }

    // Gain de fondu d'une tete a la position normalisee p (0..1) : sin(pi p).
    // La paire decalee d'une demi-fenetre somme a puissance constante.
    static float fadeGainFor (float p01) noexcept
    {
        const float p = p01 - std::floor (p01);
        return std::sin (p * juce::MathConstants<float>::pi);
    }

    // Rapport reel (lisse) et phase de la tete A, publies pour l'UI.
    float getRatioLive() const noexcept { return uiRatio.load (std::memory_order_relaxed); }
    float getTapPhase01() const noexcept { return uiPhase.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    int    numCh = 2;

    float pitchSemis = 0.0f;
    float fineCents  = 0.0f;
    float grainMs    = 60.0f;
    int   snap       = 1;
    float feedback   = 0.0f;
    float mix        = 1.0f;

    float ratioState = 1.0f;   // rapport lisse (anti-clic sur PITCH)
    float grainState = 60.0f;  // fenetre lisse (ms)
    float mixState   = 1.0f;
    float tapPhase   = 0.0f;   // position normalisee de la tete A, 0..1
    float kSlew = 0.001f;      // lissage (~30 ms), fixe dans prepare()
    float kSlow = 0.005f;      // idem (~10 ms) pour le mix

    std::vector<float> buf[kMaxCh];
    int   writePos = 0;
    float lastOut[kMaxCh] { 0.0f, 0.0f };   // pour le feedback

    std::atomic<float> uiRatio { 1.0f };
    std::atomic<float> uiPhase { 0.0f };
};

}
