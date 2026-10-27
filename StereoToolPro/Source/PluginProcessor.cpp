#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    juce::String panToText (float v)
    {
        const float pct = std::abs (v) * 100.0f;
        if (pct < 0.05f) return "Center";
        return juce::String (v < 0 ? "L " : "R ") + juce::String (pct, 1) + "%";
    }

    float panFromText (const juce::String& t)
    {
        auto s = t.trim();
        if (s.startsWithIgnoreCase ("c")) return 0.0f;
        const float val = s.retainCharacters ("0123456789.").getFloatValue() / 100.0f;
        if (s.startsWithIgnoreCase ("l") || s.startsWith ("-")) return -val;
        return val;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout StereoToolAudioProcessor::createLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using Attr = juce::AudioParameterFloatAttributes;
    using PID = juce::ParameterID;

    auto gainText = [] (float v, int) {
        return v <= -95.99f ? juce::String ("-inf dB") : juce::String (v, 2) + " dB";
    };
    auto gainParse = [] (const juce::String& t) {
        auto s = t.trim();
        if (s.containsIgnoreCase ("inf")) return -96.0f;
        return s.getFloatValue();
    };
    auto panText = [] (float v, int) { return panToText (v); };
    auto widthText = [] (float v, int) { return juce::String (v * 100.0f, 1) + " %"; };
    auto widthParse = [] (const juce::String& t) { return t.getFloatValue() / 100.0f; };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APF> (PID { "gainL", 1 }, "Input Gain L",
        juce::NormalisableRange<float> (-96.0f, 24.0f, 0.01f), 0.0f,
        Attr().withStringFromValueFunction (gainText).withValueFromStringFunction (gainParse)));
    layout.add (std::make_unique<APF> (PID { "gainR", 1 }, "Input Gain R",
        juce::NormalisableRange<float> (-96.0f, 24.0f, 0.01f), 0.0f,
        Attr().withStringFromValueFunction (gainText).withValueFromStringFunction (gainParse)));

    // Pan individual: el canal izquierdo arranca todo a la izquierda (-1), el derecho todo a la derecha (+1)
    layout.add (std::make_unique<APF> (PID { "panL", 1 }, "Pan Left Channel",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), -1.0f,
        Attr().withStringFromValueFunction (panText).withValueFromStringFunction (panFromText)));
    layout.add (std::make_unique<APF> (PID { "panR", 1 }, "Pan Right Channel",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 1.0f,
        Attr().withStringFromValueFunction (panText).withValueFromStringFunction (panFromText)));

    layout.add (std::make_unique<APB> (PID { "phaseL", 1 }, "Phase Invert L", false));
    layout.add (std::make_unique<APB> (PID { "phaseR", 1 }, "Phase Invert R", false));

    layout.add (std::make_unique<APF> (PID { "width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f), 1.0f,
        Attr().withStringFromValueFunction (widthText).withValueFromStringFunction (widthParse)));
    layout.add (std::make_unique<APF> (PID { "pan", 1 }, "Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f,
        Attr().withStringFromValueFunction (panText).withValueFromStringFunction (panFromText)));

    return layout;
}

StereoToolAudioProcessor::StereoToolAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pGainL  = apvts.getRawParameterValue ("gainL");
    pGainR  = apvts.getRawParameterValue ("gainR");
    pPanL   = apvts.getRawParameterValue ("panL");
    pPanR   = apvts.getRawParameterValue ("panR");
    pPhaseL = apvts.getRawParameterValue ("phaseL");
    pPhaseR = apvts.getRawParameterValue ("phaseR");
    pWidth  = apvts.getRawParameterValue ("width");
    pPan    = apvts.getRawParameterValue ("pan");
}

bool StereoToolAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void StereoToolAudioProcessor::prepareToPlay (double sr, int)
{
    auto init = [sr] (juce::SmoothedValue<float>& s, float v, double secs) {
        s.reset (sr, secs);
        s.setCurrentAndTargetValue (v);
    };
    init (sGainL, dbToGain (pGainL->load()), 0.02);
    init (sGainR, dbToGain (pGainR->load()), 0.02);
    init (sPanL, pPanL->load(), 0.02);
    init (sPanR, pPanR->load(), 0.02);
    init (sPolL, pPhaseL->load() > 0.5f ? -1.0f : 1.0f, 0.005);
    init (sPolR, pPhaseR->load() > 0.5f ? -1.0f : 1.0f, 0.005);
    init (sWidth, pWidth->load(), 0.02);
    init (sPan, pPan->load(), 0.02);

    // Balistica PPM: ataque ~ 1.7 ms, caida 20 dB en 1.7 s
    attackCoeff  = 1.0f - std::exp (-1.0f / (0.0017f * (float) sr));
    releaseCoeff = std::pow (0.1f, 1.0f / (1.7f * (float) sr));
    corrCoeff    = 1.0 - std::exp (-1.0 / (0.3 * sr)); // ventana ~300 ms

    for (auto& e : env) e = 0.0f;
    sLL = sRR = sLR = 0.0;
}

void StereoToolAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    sGainL.setTargetValue (dbToGain (pGainL->load()));
    sGainR.setTargetValue (dbToGain (pGainR->load()));
    sPanL.setTargetValue (pPanL->load());
    sPanR.setTargetValue (pPanR->load());
    sPolL.setTargetValue (pPhaseL->load() > 0.5f ? -1.0f : 1.0f);
    sPolR.setTargetValue (pPhaseR->load() > 0.5f ? -1.0f : 1.0f);
    sWidth.setTargetValue (pWidth->load());
    sPan.setTargetValue (pPan->load());

    float blockPeak[4] { 0, 0, 0, 0 };
    int wp = scopeWritePos.load (std::memory_order_relaxed);
    const float halfPi = juce::MathConstants<float>::halfPi;

    auto ppm = [&] (int k, float x)
    {
        x = std::abs (x);
        if (x > env[k]) env[k] += (x - env[k]) * attackCoeff;
        else            env[k] *= releaseCoeff;
        if (env[k] > blockPeak[k]) blockPeak[k] = env[k];
    };

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i], inR = R[i];
        ppm (0, inL);
        ppm (1, inR);

        // 1) ganancia de entrada + polaridad por canal
        const float a = inL * sGainL.getNextValue() * sPolL.getNextValue();
        const float b = inR * sGainR.getNextValue() * sPolR.getNextValue();

        // 2) pan individual de cada canal (ley de potencia constante)
        const float thL = (sPanL.getNextValue() + 1.0f) * 0.5f * halfPi;
        const float thR = (sPanR.getNextValue() + 1.0f) * 0.5f * halfPi;
        float oL = a * std::cos (thL) + b * std::cos (thR);
        float oR = a * std::sin (thL) + b * std::sin (thR);

        // 3) ancho (M/S)
        const float w = sWidth.getNextValue();
        const float m = (oL + oR) * 0.5f;
        const float s = (oL - oR) * 0.5f * w;
        oL = m + s;
        oR = m - s;

        // 4) pan / balance general
        const float p = sPan.getNextValue();
        oL *= (p > 0.0f ? 1.0f - p : 1.0f);
        oR *= (p < 0.0f ? 1.0f + p : 1.0f);

        L[i] = oL;
        R[i] = oR;

        ppm (2, oL);
        ppm (3, oR);

        // correlacion de fase (ventana exponencial)
        sLL += corrCoeff * ((double) oL * oL - sLL);
        sRR += corrCoeff * ((double) oR * oR - sRR);
        sLR += corrCoeff * ((double) oL * oR - sLR);

        // vectorscopio
        scopeL[wp] = oL;
        scopeR[wp] = oR;
        wp = (wp + 1) & (scopeSize - 1);
    }

    scopeWritePos.store (wp, std::memory_order_release);

    for (int k = 0; k < 4; ++k)
    {
        level[k].store (env[k]);
        if (blockPeak[k] > peakMax[k].load()) peakMax[k].store (blockPeak[k]);
    }

    const double den = sLL * sRR;
    if (den > 1.0e-12)
        correlation.store ((float) juce::jlimit (-1.0, 1.0, sLR / std::sqrt (den)));
}

juce::AudioProcessorEditor* StereoToolAudioProcessor::createEditor()
{
    return new StereoToolAudioProcessorEditor (*this);
}

void StereoToolAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void StereoToolAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoToolAudioProcessor();
}
