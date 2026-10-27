#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour bgTop    { 0xff10131c };
    const juce::Colour bgBottom { 0xff2b3550 };
    const juce::Colour accent   { 0xffff7a1a };

    constexpr int W = 580;
    constexpr int H = 690;

    // Escala de los medidores (igual que la referencia: no lineal)
    float dbToNorm (float db)
    {
        static const float d[] = { -96, -64, -48, -32, -24, -18, -12, -6, -3, -2, -1, 0 };
        static const float n[] = { 0.0f, 0.161f, 0.256f, 0.363f, 0.428f, 0.478f, 0.551f,
                                   0.641f, 0.753f, 0.888f, 0.951f, 1.0f };
        if (db <= d[0]) return 0.0f;
        if (db >= 0.0f) return 1.0f;
        for (int i = 1; i < 12; ++i)
            if (db < d[i])
            {
                const float t = (db - d[i - 1]) / (d[i] - d[i - 1]);
                return n[i - 1] + t * (n[i] - n[i - 1]);
            }
        return 1.0f;
    }

    juce::String dbText (float gain)
    {
        const float db = juce::Decibels::gainToDecibels (gain, -96.0f);
        return db <= -95.9f ? juce::String ("-inf") : juce::String (db, 1);
    }
}

//==============================================================================
StereoLookAndFeel::StereoLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha (0.25f));
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.5f));
    setColour (juce::Slider::trackColourId, accent);
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
    setColour (juce::Slider::backgroundColourId, juce::Colours::white.withAlpha (0.12f));
}

void StereoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float cur = startAngle + pos * (endAngle - startAngle);

    const int dots = 25;
    for (int i = 0; i < dots; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (dots - 1);
        const float px = c.x + r * std::sin (a);
        const float py = c.y - r * std::cos (a);
        g.setColour (a <= cur + 0.001f ? accent : juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (px - 2.0f, py - 2.0f, 4.0f, 4.0f);
    }

    // aguja
    const float ir = r * 0.62f;
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine (c.x + ir * 0.35f * std::sin (cur), c.y - ir * 0.35f * std::cos (cur),
                c.x + ir * std::sin (cur), c.y - ir * std::cos (cur), 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f, 1.0f);
}

void StereoLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn, bool, bool)
{
    auto b = btn.getLocalBounds().toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (16.0f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("Phase \xc3\x98")),
                b.removeFromLeft (b.getWidth() * 0.6f), juce::Justification::centredRight);

    auto track = juce::Rectangle<float> (36.0f, 16.0f).withCentre ({ b.getCentreX() + 2.0f, b.getCentreY() });
    const bool on = btn.getToggleState();
    g.setColour (on ? accent : juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (track, 8.0f);
    g.setColour (juce::Colours::white);
    const float kx = on ? track.getRight() - 9.0f : track.getX() + 9.0f;
    g.fillEllipse (kx - 6.0f, track.getCentreY() - 6.0f, 12.0f, 12.0f);
}

//==============================================================================
void StereoToolAudioProcessorEditor::setupKnob (juce::Slider& s, float)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setMouseDragSensitivity (600);   // arrastre lento = ajuste fino
    s.setLookAndFeel (&lnf);
    addAndMakeVisible (s);
}

StereoToolAudioProcessorEditor::StereoToolAudioProcessorEditor (StereoToolAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (W, H);

    setupKnob (gainL, 0.0f);
    setupKnob (gainR, 0.0f);
    setupKnob (panL, -1.0f);
    setupKnob (panR, 1.0f);

    for (auto* s : { &width, &pan })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 90, 22);
        s->setLookAndFeel (&lnf);
        addAndMakeVisible (*s);
    }

    for (auto* b : { &phaseL, &phaseR })
    {
        b->setLookAndFeel (&lnf);
        addAndMakeVisible (*b);
    }

    aGainL  = std::make_unique<SliderAtt> (proc.apvts, "gainL", gainL);
    aGainR  = std::make_unique<SliderAtt> (proc.apvts, "gainR", gainR);
    aPanL   = std::make_unique<SliderAtt> (proc.apvts, "panL", panL);
    aPanR   = std::make_unique<SliderAtt> (proc.apvts, "panR", panR);
    aWidth  = std::make_unique<SliderAtt> (proc.apvts, "width", width);
    aPan    = std::make_unique<SliderAtt> (proc.apvts, "pan", pan);
    aPhaseL = std::make_unique<ButtonAtt> (proc.apvts, "phaseL", phaseL);
    aPhaseR = std::make_unique<ButtonAtt> (proc.apvts, "phaseR", phaseR);

    scopeImg = juce::Image (juce::Image::RGB, 280, 280, true);
    startTimerHz (30);
}

StereoToolAudioProcessorEditor::~StereoToolAudioProcessorEditor()
{
    stopTimer();
    for (auto* s : { &gainL, &gainR, &panL, &panR, &width, &pan })
        s->setLookAndFeel (nullptr);
    phaseL.setLookAndFeel (nullptr);
    phaseR.setLookAndFeel (nullptr);
}

void StereoToolAudioProcessorEditor::resized()
{
    gainL.setBounds (20, 75, 110, 130);
    gainR.setBounds (W - 130, 75, 110, 130);
    phaseL.setBounds (20, 250, 110, 28);
    phaseR.setBounds (W - 130, 250, 110, 28);
    panL.setBounds (20, 300, 110, 130);
    panR.setBounds (W - 130, 300, 110, 130);

    scopeBounds = { (W - 280) / 2, 80, 280, 280 };

    width.setBounds (90, 385, W - 110, 24);
    pan.setBounds (90, 415, W - 110, 24);

    meterBounds = { 70, 485, 440, 4 * 18 };
    corrBounds  = { 70, 600, 440, 18 };
}

void StereoToolAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // clic sobre los medidores = reinicia los picos
    if (meterBounds.expanded (70, 10).contains (e.getPosition()))
        for (auto& p : proc.peakMax) p.store (0.0f);
}

void StereoToolAudioProcessorEditor::updateScope()
{
    const int pos = proc.scopeWritePos.load (std::memory_order_acquire);
    juce::Graphics g (scopeImg);

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillAll();

    if (pos != lastScopePos)
    {
        lastScopePos = pos;
        const float size  = (float) scopeImg.getWidth();
        const float cx    = size * 0.5f;
        const float scale = size * 0.5f * 0.95f;
        constexpr int N = 1024;

        for (int j = 0; j < N; ++j)
        {
            const int idx = (pos - N + j) & (StereoToolAudioProcessor::scopeSize - 1);
            const float l = proc.scopeL[idx], r = proc.scopeR[idx];
            if (! std::isfinite (l) || ! std::isfinite (r)) continue;

            const float x = cx + (r - l) * 0.7071f * scale;   // lado (S)
            const float y = cx - (l + r) * 0.7071f * scale;   // medio (M)
            const float mag = juce::jmin (1.0f, std::sqrt (l * l + r * r));

            g.setColour (juce::Colour::fromHSV (0.04f + 0.08f * mag, 1.0f, 0.65f + 0.35f * mag, 0.6f));
            g.fillRect (x - 0.8f, y - 0.8f, 1.6f, 1.6f);
        }
    }
}

void StereoToolAudioProcessorEditor::timerCallback()
{
    updateScope();
    repaint();
}

void StereoToolAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) H, false));
    g.fillAll();

    // titulo
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("Stereo Tool Pro", 0, 12, W, 40, juce::Justification::centred);

    // etiquetas de perillas
    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawText ("Input Gain L", 20, 205, 110, 22, juce::Justification::centred);
    g.drawText ("Input Gain R", W - 130, 205, 110, 22, juce::Justification::centred);
    g.drawText ("Left",  20, 432, 110, 22, juce::Justification::centred);
    g.drawText ("Right", W - 130, 432, 110, 22, juce::Justification::centred);
    g.drawText ("Width", 20, 385, 70, 24, juce::Justification::centredLeft);
    g.drawText ("Pan",   20, 415, 70, 24, juce::Justification::centredLeft);

    // ---- vectorscopio ----
    {
        juce::Path circle;
        circle.addEllipse (scopeBounds.toFloat());
        g.setColour (juce::Colours::black);
        g.fillPath (circle);

        g.saveState();
        g.reduceClipRegion (circle);
        g.drawImageAt (scopeImg, scopeBounds.getX(), scopeBounds.getY());
        g.restoreState();

        const auto c = scopeBounds.toFloat().getCentre();
        const float r = scopeBounds.getWidth() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (c.x - r, c.y, c.x + r, c.y);                         // eje S
        g.drawLine (c.x, c.y - r, c.x, c.y + r);                         // eje M
        g.drawLine (c.x - r * 0.7071f, c.y - r * 0.7071f, c.x + r * 0.7071f, c.y + r * 0.7071f);
        g.drawLine (c.x + r * 0.7071f, c.y - r * 0.7071f, c.x - r * 0.7071f, c.y + r * 0.7071f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (scopeBounds.toFloat(), 3.0f);

        g.setFont (juce::FontOptions (13.0f));
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawText ("L",  (int) (c.x - r * 0.7071f) - 4, (int) (c.y - r * 0.7071f) - 4, 16, 16, juce::Justification::centred);
        g.drawText ("R",  (int) (c.x + r * 0.7071f) - 12, (int) (c.y - r * 0.7071f) - 4, 16, 16, juce::Justification::centred);
        g.drawText ("M",  (int) c.x - 8, scopeBounds.getY() + 4, 16, 16, juce::Justification::centred);
        g.drawText ("-S", scopeBounds.getX() + 6, (int) c.y - 8, 24, 16, juce::Justification::centred);
        g.drawText ("+S", scopeBounds.getRight() - 30, (int) c.y - 8, 24, 16, juce::Justification::centred);
    }

    // ---- medidores PPM ----
    {
        static const float ticks[] = { -96, -64, -48, -32, -24, -18, -12, -6, -3, -2, -1, 0 };
        const char* names[] = { "L in", "R in", "L out", "R out" };
        const int bx = meterBounds.getX(), bw = meterBounds.getWidth();

        g.setFont (juce::FontOptions (12.0f));
        for (float t : ticks)
        {
            const int tx = bx + (int) (dbToNorm (t) * (float) bw);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawText (juce::String ((int) t), tx - 18, meterBounds.getY() - 20, 36, 14, juce::Justification::centred);
        }

        for (int k = 0; k < 4; ++k)
        {
            juce::Rectangle<int> row (bx, meterBounds.getY() + k * 18, bw, 14);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRect (row);

            const float db = juce::Decibels::gainToDecibels (proc.level[k].load(), -96.0f);
            const int fw = (int) (dbToNorm (db) * (float) bw);
            g.setColour (k < 2 ? juce::Colour (0xffb9c3dd) : accent);
            g.fillRect (row.withWidth (fw));
            if (db > -6.0f)
            {
                const int x6 = (int) (dbToNorm (-6.0f) * (float) bw);
                g.setColour (juce::Colour (0xffe03b3b));
                g.fillRect (row.withTrimmedLeft (x6).withWidth (juce::jmax (0, fw - x6)));
            }

            g.setColour (juce::Colours::white.withAlpha (0.25f));
            for (float t : ticks)
                g.drawVerticalLine (bx + (int) (dbToNorm (t) * (float) bw), (float) row.getY(), (float) row.getBottom());

            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.setFont (juce::FontOptions (14.0f));
            g.drawText (names[k], 8, row.getY() - 1, 58, 16, juce::Justification::centredRight);
            g.drawText (dbText (proc.peakMax[k].load()), bx + bw + 8, row.getY() - 1, 60, 16,
                        juce::Justification::centredLeft);
        }

        g.setFont (juce::FontOptions (11.0f));
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText ("clic = reiniciar picos (dB)", bx, meterBounds.getBottom() + 2, bw, 14, juce::Justification::centredRight);
    }

    // ---- correlacion de fase ----
    {
        const float corr = proc.correlation.load();
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Phase", 8, corrBounds.getY(), 58, 18, juce::Justification::centredRight);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRect (corrBounds);

        const int cx = corrBounds.getCentreX();
        const int mx = cx + (int) (corr * (float) corrBounds.getWidth() * 0.5f);
        g.setColour (corr >= 0.0f ? juce::Colour (0xff4fd16a) : juce::Colour (0xffe03b3b));
        g.fillRect (juce::Rectangle<int> (juce::jmin (cx, mx), corrBounds.getY(), std::abs (mx - cx) + 1, corrBounds.getHeight()));
        g.setColour (juce::Colours::white);
        g.fillRect (mx - 2, corrBounds.getY() - 2, 4, corrBounds.getHeight() + 4);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawVerticalLine (cx, (float) corrBounds.getY(), (float) corrBounds.getBottom());

        g.setFont (juce::FontOptions (12.0f));
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawText ("-1", corrBounds.getX() - 10, corrBounds.getBottom() + 3, 30, 14, juce::Justification::centred);
        g.drawText ("0", cx - 15, corrBounds.getBottom() + 3, 30, 14, juce::Justification::centred);
        g.drawText ("+1", corrBounds.getRight() - 20, corrBounds.getBottom() + 3, 30, 14, juce::Justification::centred);
        g.setFont (juce::FontOptions (14.0f));
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawText (juce::String (corr, 2), corrBounds.getRight() + 8, corrBounds.getY(), 60, 18, juce::Justification::centredLeft);
    }
}
