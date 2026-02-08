#include "PPGLookAndFeel.h"

PPGLookAndFeel::PPGLookAndFeel()
{
    // Window / general background
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(kBackground));

    // Labels
    setColour(juce::Label::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));

    // Sliders
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(kTextPrimary));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x00000000));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(kAccent));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(kSliderTrack));
    setColour(juce::Slider::thumbColourId, juce::Colour(kSurface));

    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::ComboBox::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(kSurfaceBorder));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(kTextSecondary));

    // PopupMenu
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::PopupMenu::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(kAccent));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    // TextButton
    setColour(juce::TextButton::buttonColourId, juce::Colour(kAccent));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    // TabbedComponent
    setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0x00000000));
    setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colour(kTextSecondary));
    setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colour(kAccent));
}

//==============================================================================
void PPGLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    auto trackWidth = 3.0f;

    // Background track (full arc)
    {
        juce::Path track;
        track.addCentredArc(centreX, centreY, radius - trackWidth * 0.5f, radius - trackWidth * 0.5f,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(kSliderTrack));
        g.strokePath(track, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Filled arc (value)
    if (sliderPos > 0.0f)
    {
        juce::Path filled;
        filled.addCentredArc(centreX, centreY, radius - trackWidth * 0.5f, radius - trackWidth * 0.5f,
                             0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(kAccent));
        g.strokePath(filled, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Thumb (white circle with subtle shadow)
    auto thumbRadius = 7.0f;
    auto thumbX = centreX + (radius - trackWidth * 0.5f) * std::cos(angle - juce::MathConstants<float>::halfPi);
    auto thumbY = centreY + (radius - trackWidth * 0.5f) * std::sin(angle - juce::MathConstants<float>::halfPi);

    // Shadow
    g.setColour(juce::Colour(0x18000000));
    g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius + 1.0f, thumbRadius * 2.0f, thumbRadius * 2.0f);

    // Thumb fill
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);

    // Thumb outline
    g.setColour(juce::Colour(kSurfaceBorder));
    g.drawEllipse(thumbX - thumbRadius, thumbY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f, 0.5f);
}

//==============================================================================
void PPGLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto cornerRadius = 8.0f;

    auto baseColour = backgroundColour;

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.08f);

    if (!button.isEnabled())
        baseColour = baseColour.withAlpha(0.4f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

//==============================================================================
void PPGLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool /*shouldDrawButtonAsHighlighted*/,
                                      bool /*shouldDrawButtonAsDown*/)
{
    auto font = getTextButtonFont(button, button.getHeight());
    g.setFont(font);

    auto textColour = button.findColour(juce::TextButton::textColourOffId);
    if (!button.isEnabled())
        textColour = textColour.withAlpha(0.4f);

    g.setColour(textColour);
    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(4, 2),
               juce::Justification::centred, true);
}

//==============================================================================
void PPGLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    auto cornerRadius = 8.0f;

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // Chevron
    auto arrowArea = juce::Rectangle<float>((float)width - 24.0f, 0.0f, 20.0f, (float)height);
    juce::Path chevron;
    auto cx = arrowArea.getCentreX();
    auto cy = arrowArea.getCentreY();
    chevron.addTriangle(cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(chevron);
}

//==============================================================================
void PPGLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(findColour(juce::PopupMenu::backgroundColourId));
    g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 8.0f);

    g.setColour(juce::Colour(kSurfaceBorder));
    g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, 8.0f, 0.5f);
}

//==============================================================================
void PPGLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool /*hasSubMenu*/,
                                         const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                         const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        auto r = area.reduced(8, 0);
        g.setColour(juce::Colour(kSurfaceBorder));
        g.fillRect(r.getX(), r.getCentreY(), r.getWidth(), 1);
        return;
    }

    auto r = area.reduced(4, 1);

    if (isHighlighted && isActive)
    {
        g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRoundedRectangle(r.toFloat(), 6.0f);
        g.setColour(findColour(juce::PopupMenu::highlightedTextColourId));
    }
    else
    {
        g.setColour(isActive ? findColour(juce::PopupMenu::textColourId)
                             : juce::Colour(kTextSecondary));
    }

    auto textArea = r.reduced(8, 0);
    g.setFont(getPopupMenuFont());
    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);

    if (isTicked)
    {
        auto tickArea = juce::Rectangle<float>((float)r.getRight() - 20.0f,
                                                (float)r.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.fillEllipse(tickArea);
    }
}

//==============================================================================
int PPGLookAndFeel::getTabButtonBestWidth(juce::TabBarButton& button, int /*tabDepth*/)
{
    auto font = juce::Font(juce::FontOptions().withHeight(14.0f));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, button.getButtonText(), 0.0f, 0.0f);
    return (int)std::ceil(glyphs.getBoundingBox(0, -1, false).getWidth()) + 48;
}

void PPGLookAndFeel::drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                                     bool /*isMouseOver*/, bool /*isMouseDown*/)
{
    auto area = button.getActiveArea();
    bool isFront = button.isFrontTab();

    // Background — white surface behind the whole tab bar
    g.setColour(juce::Colour(kSurface));
    g.fillRect(area);

    // Active bottom border
    if (isFront)
    {
        g.setColour(juce::Colour(kAccent));
        g.fillRect(area.getX(), area.getBottom() - 3, area.getWidth(), 3);
    }

    // Text
    auto font = juce::Font(juce::FontOptions().withHeight(14.0f));
    g.setFont(font);
    g.setColour(isFront ? juce::Colour(kAccent) : juce::Colour(kTextSecondary));
    g.drawText(button.getButtonText(), area, juce::Justification::centred, false);
}

void PPGLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar& bar, juce::Graphics& g)
{
    g.setColour(juce::Colour(kSurface));
    g.fillAll();

    // Bottom separator
    g.setColour(juce::Colour(kSurfaceBorder));
    g.fillRect(0, bar.getHeight() - 1, bar.getWidth(), 1);
}

void PPGLookAndFeel::drawTabAreaBehindFrontButton(juce::TabbedButtonBar& bar, juce::Graphics& g,
                                                    int w, int h)
{
    // This component sits BETWEEN inactive tabs and the front tab in z-order,
    // covering the full tab bar area. We must re-draw inactive tab text here
    // since a solid fill would hide it.

    // Bottom separator only — the bar background already painted white
    g.setColour(juce::Colour(kSurfaceBorder));
    g.fillRect(0, h - 1, w, 1);

    // Re-draw inactive tab labels on top
    for (int i = 0; i < bar.getNumTabs(); ++i)
    {
        if (auto* tb = bar.getTabButton(i))
        {
            if (tb->isFrontTab())
                continue;

            auto area = tb->getBounds();
            auto font = juce::Font(juce::FontOptions().withHeight(14.0f));
            g.setFont(font);
            g.setColour(juce::Colour(kTextSecondary));
            g.drawText(tb->getButtonText(), area, juce::Justification::centred, false);
        }
    }
}

int PPGLookAndFeel::getTabButtonOverlap(int /*tabDepth*/)
{
    return 0;
}

//==============================================================================
juce::Font PPGLookAndFeel::getTextButtonFont(juce::TextButton&, int /*buttonHeight*/)
{
    return juce::Font(juce::FontOptions().withHeight(13.0f));
}

juce::Font PPGLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions().withHeight(12.0f));
}

juce::Font PPGLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions().withHeight(12.0f));
}

juce::Font PPGLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions().withHeight(13.0f));
}
