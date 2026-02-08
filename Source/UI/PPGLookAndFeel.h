#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PPGLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PPGLookAndFeel();

    // Color palette
    static constexpr uint32_t kBackground     = 0xFFF5F5F7;
    static constexpr uint32_t kSurface        = 0xFFFFFFFF;
    static constexpr uint32_t kSurfaceBorder  = 0xFFE5E5EA;
    static constexpr uint32_t kTextPrimary    = 0xFF1D1D1F;
    static constexpr uint32_t kTextSecondary  = 0xFF86868B;
    static constexpr uint32_t kAccent         = 0xFF007AFF;
    static constexpr uint32_t kPositive       = 0xFF34C759;
    static constexpr uint32_t kNegative       = 0xFFFF3B30;
    static constexpr uint32_t kNeutral        = 0xFF8E8E93;
    static constexpr uint32_t kSliderTrack    = 0xFFD1D1D6;
    static constexpr uint32_t kGroupHeader    = 0xFF6E6E73;

    // Draw overrides
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    int getTabButtonBestWidth(juce::TabBarButton&, int tabDepth) override;
    void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;
    void drawTabbedButtonBarBackground(juce::TabbedButtonBar&, juce::Graphics&) override;
    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar&, juce::Graphics&, int w, int h) override;
    int getTabButtonOverlap(int tabDepth) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getPopupMenuFont() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PPGLookAndFeel)
};
