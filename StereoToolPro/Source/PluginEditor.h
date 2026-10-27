#pragma once
#include "PluginProcessor.h"

// Aspecto: azul oscuro + naranja "fuego" como en la referencia
class StereoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StereoLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class StereoToolAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit StereoToolAudioProcessorEditor (StereoToolAudioProcessor&);
    ~StereoToolAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void updateScope();
    void setupKnob (juce::Slider&, float defaultValue);

    StereoToolAudioProcessor& proc;
    StereoLookAndFeel lnf;

    juce::Slider gainL, gainR, panL, panR, width, pan;
    juce::ToggleButton phaseL, phaseR;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> aGainL, aGainR, aPanL, aPanR, aWidth, aPan;
    std::unique_ptr<ButtonAtt> aPhaseL, aPhaseR;

    juce::Image scopeImg;
    int lastScopePos = 0;

    juce::Rectangle<int> scopeBounds, meterBounds, corrBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoToolAudioProcessorEditor)
};
