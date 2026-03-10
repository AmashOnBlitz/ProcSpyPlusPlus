#pragma once
#include "../core/Types.h"
#include <string>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  Design tokens — Fluent 2 / WinUI 3, polished 2026 aesthetic
// ─────────────────────────────────────────────────────────────────────────────
namespace Token {

// ── Backgrounds ───────────────────────────────────────────────────────────────
inline const Color AppBackground     = Color::FromRGB(243, 243, 243);
inline const Color CardBackground    = Color::FromRGB(255, 255, 255);
inline const Color SubtleBackground  = Color::FromRGB(249, 249, 249);

// ── Control fills (fill-color system) ────────────────────────────────────────
inline const Color CtrlFillRest      = Color::FromRGB(255, 255, 255, 178); // 70%
inline const Color CtrlFillHover     = Color::FromRGB(249, 249, 249, 255);
inline const Color CtrlFillPress     = Color::FromRGB(249, 249, 249, 127); // 50%
inline const Color CtrlFillDisabled  = Color::FromRGB(249, 249, 249,  77); // 30%

// subtle gradient tint painted over the fill (additive)
inline const Color GradTintTop       = Color::FromRGB(255, 255, 255,  13); //  5%
inline const Color GradTintBot       = Color::FromRGB(  0,   0,   0,  10); //  4%

// ── Strokes ───────────────────────────────────────────────────────────────────
inline const Color CtrlStroke        = Color::FromRGB(  0,   0,   0,  15); //  6%
inline const Color CtrlStrokeBottom  = Color::FromRGB(  0,   0,   0,  41); // 16%
inline const Color CtrlStrokeDisabled= Color::FromRGB(  0,   0,   0,  21); //  8%
inline const Color StrokeCard        = Color::FromRGB(  0,   0,   0,  15);

// ── Accent ────────────────────────────────────────────────────────────────────
inline const Color AccentDefault     = Color::FromRGB(  0, 103, 192);
inline const Color AccentHover       = Color::FromRGB( 26, 117, 204);
inline const Color AccentPressed     = Color::FromRGB(  0,  84, 166);
inline const Color AccentGradTop     = Color::FromRGB(255, 255, 255,  20);
inline const Color AccentGradBot     = Color::FromRGB(  0,   0,   0,  20);

// ── Danger ────────────────────────────────────────────────────────────────────
inline const Color DangerDefault     = Color::FromRGB(196,  43,  28);
inline const Color DangerHover       = Color::FromRGB(163,  35,  22);
inline const Color DangerPressed     = Color::FromRGB(130,  28,  18);

// ── Text ──────────────────────────────────────────────────────────────────────
inline const Color TextPrimary       = Color::FromRGB(  0,   0,   0, 228); // 89%
inline const Color TextSecondary     = Color::FromRGB(  0,   0,   0, 154); // 60%
inline const Color TextTertiary      = Color::FromRGB(  0,   0,   0,  92); // 36%
inline const Color TextDisabled      = Color::FromRGB(  0,   0,   0,  92);
inline const Color TextOnAccent      = Color::FromRGB(255, 255, 255);
inline const Color TextPlaceholder   = Color::FromRGB(  0,   0,   0, 100);

// ── Focus ─────────────────────────────────────────────────────────────────────
inline const Color FocusOuter        = Color::FromRGB(  0,   0,   0, 228);
inline const Color FocusInner        = Color::FromRGB(255, 255, 255);

// ── Geometry ─────────────────────────────────────────────────────────────────
inline constexpr float RadiusSmall   =  4.f;
inline constexpr float RadiusControl =  6.f;
inline constexpr float RadiusLarge   =  8.f;
inline constexpr float RadiusCard    = 10.f;

// ── Typography ────────────────────────────────────────────────────────────────
// Win 11+ provides "Segoe UI Variable"; on Win 10 it gracefully falls back.
inline const std::wstring FontBody    = L"Segoe UI Variable Text";
inline const std::wstring FontDisplay = L"Segoe UI Variable Display";
inline constexpr float SizeCaption   = 12.f;
inline constexpr float SizeBody      = 14.f;
inline constexpr float SizeBodyLarge = 16.f;
inline constexpr float SizeSubtitle  = 20.f;
inline constexpr float SizeTitle     = 28.f;

} // namespace Token

// ─────────────────────────────────────────────────────────────────────────────
//  Per-component style structs
// ─────────────────────────────────────────────────────────────────────────────

struct FontStyle {
    std::wstring name = Token::FontBody;
    float        size = Token::SizeBody;
    bool         bold = false;
};

// ────────────────────────────────────────────────────────── ButtonStyle ───────
struct ButtonStyle {
    Color background         = Token::CtrlFillRest;
    Color backgroundHover    = Token::CtrlFillHover;
    Color backgroundPressed  = Token::CtrlFillPress;
    Color backgroundDisabled = Token::CtrlFillDisabled;
    Color gradientTop        = Token::GradTintTop;
    Color gradientBottom     = Token::GradTintBot;
    Color border             = Token::CtrlStroke;
    Color borderBottom       = Token::CtrlStrokeBottom;
    Color borderFocused      = Token::AccentDefault;
    Color borderDisabled     = Token::CtrlStrokeDisabled;
    Color textColor          = Token::TextPrimary;
    Color textDisabled       = Token::TextDisabled;
    FontStyle font;
    float cornerRadius       = Token::RadiusControl;
    float borderWidth        = 1.f;
    bool  castShadow         = false;

    static ButtonStyle Accent() {
        ButtonStyle s;
        s.background         = Token::AccentDefault;
        s.backgroundHover    = Token::AccentHover;
        s.backgroundPressed  = Token::AccentPressed;
        s.backgroundDisabled = Token::CtrlFillDisabled;
        s.gradientTop        = Token::AccentGradTop;
        s.gradientBottom     = Token::AccentGradBot;
        s.border             = Token::AccentDefault;
        s.borderBottom       = Color::FromRGB(0, 0, 0, 41);
        s.textColor          = Token::TextOnAccent;
        s.textDisabled       = Token::TextDisabled;
        s.castShadow         = true;
        return s;
    }

    static ButtonStyle Danger() {
        ButtonStyle s;
        s.background         = Token::DangerDefault;
        s.backgroundHover    = Token::DangerHover;
        s.backgroundPressed  = Token::DangerPressed;
        s.backgroundDisabled = Token::CtrlFillDisabled;
        s.gradientTop        = Token::AccentGradTop;
        s.gradientBottom     = Token::AccentGradBot;
        s.border             = Token::DangerDefault;
        s.borderBottom       = Color::FromRGB(0, 0, 0, 41);
        s.textColor          = Token::TextOnAccent;
        s.textDisabled       = Token::TextDisabled;
        s.castShadow         = true;
        return s;
    }

    static ButtonStyle Subtle() {
        ButtonStyle s;
        s.background         = Color::Transparent;
        s.backgroundHover    = Color::FromRGB(0, 0, 0, 10);
        s.backgroundPressed  = Color::FromRGB(0, 0, 0, 18);
        s.gradientTop        = Color::Transparent;
        s.gradientBottom     = Color::Transparent;
        s.border             = Color::Transparent;
        s.borderBottom       = Color::Transparent;
        s.textColor          = Token::AccentDefault;
        return s;
    }
};

// ────────────────────────────────────────────────────────── LabelStyle ────────
struct LabelStyle {
    Color     textColor = Token::TextPrimary;
    FontStyle font;
    bool      wordWrap  = false;
};

// ────────────────────────────────────────────────────────── CheckboxStyle ─────
struct CheckboxStyle {
    Color boxRest        = Token::CtrlFillRest;
    Color boxHover       = Token::CtrlFillHover;
    Color boxChecked     = Token::AccentDefault;
    Color boxCheckedHov  = Token::AccentHover;
    Color boxBorder      = Token::CtrlStroke;
    Color boxBorderBot   = Token::CtrlStrokeBottom;
    Color checkMark      = Token::TextOnAccent;
    Color textColor      = Token::TextPrimary;
    FontStyle font;
    float size           = 18.f;
    float gap            = 8.f;
};

// ────────────────────────────────────────────────────────── RadioButtonStyle ──
struct RadioButtonStyle {
    Color circleRest     = Token::CtrlFillRest;
    Color circleHover    = Token::CtrlFillHover;
    Color circleSelected = Token::AccentDefault;
    Color circleSelHov   = Token::AccentHover;
    Color circleBorder   = Token::CtrlStroke;
    Color circleBorderB  = Token::CtrlStrokeBottom;
    Color dot            = Token::TextOnAccent;
    Color textColor      = Token::TextPrimary;
    FontStyle font;
    float size           = 18.f;
    float gap            = 8.f;
};

// ────────────────────────────────────────────────────────── SliderStyle ───────
struct SliderStyle {
    Color track          = Color::FromRGB(200, 200, 200);
    Color trackFilled    = Token::AccentDefault;
    Color thumb          = Color::White;
    Color thumbBorder    = Token::AccentDefault;
    Color thumbHover     = Color::FromRGB(232, 242, 255);
    Color thumbPressed   = Color::FromRGB(210, 230, 255);
    float trackHeight    = 4.f;
    float thumbRadius    = 10.f;
};

// ────────────────────────────────────────────────────────── ProgressBarStyle ──
struct ProgressBarStyle {
    Color track          = Color::FromRGB(0, 0, 0, 18);
    Color fill           = Token::AccentDefault;
    Color fillGradTop    = Color::FromRGB(255, 255, 255, 35);
    Color text           = Token::TextOnAccent;
    float cornerRadius   = 3.f;
    bool  showPercent    = false;
    bool  useGradient    = true;
    FontStyle font;
};

// ────────────────────────────────────────────────────────── ListBoxStyle ──────
struct ListBoxStyle {
    Color background       = Token::CardBackground;
    Color itemHover        = Color::FromRGB(0, 0, 0,  8);
    Color itemSelected     = Color::FromRGB(0, 103, 192, 22);
    Color itemSelectedHov  = Color::FromRGB(0, 103, 192, 35);
    Color itemSelectedText = Token::TextPrimary;
    Color itemAccentBar    = Token::AccentDefault;
    Color textColor        = Token::TextPrimary;
    Color border           = Token::StrokeCard;
    Color scrollThumb      = Color::FromRGB(0, 0, 0, 80);
    Color scrollThumbHov   = Color::FromRGB(0, 0, 0, 120);
    FontStyle font;
    float itemHeight       = 32.f;
    float padding          = 10.f;
    float cornerRadius     = Token::RadiusCard;
};

// ────────────────────────────────────────────────────────── SearchBoxStyle ────
struct SearchBoxStyle {
    Color background        = Token::CtrlFillRest;
    Color backgroundFocused = Token::CardBackground;
    Color border            = Token::CtrlStroke;
    Color borderBottom      = Token::CtrlStrokeBottom;
    Color borderFocused     = Token::AccentDefault;
    Color textColor         = Token::TextPrimary;
    Color placeholderColor  = Token::TextPlaceholder;
    Color iconColor         = Token::TextSecondary;
    FontStyle font;
    float cornerRadius      = Token::RadiusControl;
    float paddingH          = 10.f;
};

// ────────────────────────────────────────────────────────── TextBoxStyle ──────
struct TextBoxStyle {
    Color background        = Token::CtrlFillRest;
    Color backgroundFocused = Token::CardBackground;
    Color border            = Token::CtrlStroke;
    Color borderBottom      = Token::CtrlStrokeBottom;
    Color borderFocused     = Token::AccentDefault;
    Color textColor         = Token::TextPrimary;
    Color placeholderColor  = Token::TextPlaceholder;
    Color selectionColor    = Color::FromRGB(0, 103, 192, 60);
    FontStyle font;
    float cornerRadius      = Token::RadiusControl;
    float paddingH          = 10.f;
    float paddingV          =  6.f;
};

// ────────────────────────────────────────────────────────── ToggleSwitchStyle ─
struct ToggleSwitchStyle {
    Color trackOff          = Color::FromRGB(160, 160, 160);
    Color trackOffHover     = Color::FromRGB(140, 140, 140);
    Color trackOn           = Token::AccentDefault;
    Color trackOnHover      = Token::AccentHover;
    Color trackOnPressed    = Token::AccentPressed;
    Color trackBorder       = Token::CtrlStrokeBottom;
    Color thumb             = Color::White;
    Color thumbShadow       = Color::FromRGB(0, 0, 0, 40);
    Color textColor         = Token::TextPrimary;
    Color textDisabled      = Token::TextDisabled;
    FontStyle font;
    float trackWidth        = 40.f;
    float trackHeight       = 20.f;
    float thumbRadius       =  8.f;
    float gap               =  8.f;
};

// ────────────────────────────────────────────────────────── ImageWidgetStyle ──
struct ImageWidgetStyle {
    Color   background      = Color::Transparent;
    Color   border          = Color::Transparent;
    Color   placeholder     = Color::FromRGB(220, 220, 220);
    Color   placeholderIcon = Color::FromRGB(160, 160, 160);
    float   cornerRadius    = 0.f;
    float   borderWidth     = 0.f;
    bool    keepAspect      = true;
    bool    castShadow      = false;
};

// ────────────────────────────────────────────────────────── ColorPickerStyle ──
struct ColorPickerStyle {
    Color border            = Token::CtrlStroke;
    Color borderBottom      = Token::CtrlStrokeBottom;
    Color background        = Token::CardBackground;
    Color checkerA          = Color::FromRGB(200, 200, 200);
    Color checkerB          = Color::FromRGB(255, 255, 255);
    Color thumbStroke       = Color::White;
    Color thumbStrokeDark   = Color::FromRGB( 80,  80,  80);
    Color textColor         = Token::TextPrimary;
    Color inputBackground   = Token::CtrlFillRest;
    FontStyle font;
    float cornerRadius      = Token::RadiusCard;
    float pickerSize        = 200.f;
    float hueBarHeight      = 14.f;
    float alphaBarHeight    = 14.f;
    float thumbRadius       =  7.f;
    float gapBetweenParts   =  8.f;
    bool  showAlpha         = true;
    bool  showHexInput      = true;
};

// ────────────────────────────────────────────────────────── TabControlStyle ───
struct TabControlStyle {
    Color tabBackground    = Color::Transparent;
    Color tabHover         = Color::FromRGB(0, 0, 0,  8);
    Color tabActive        = Token::CardBackground;
    Color tabBorder        = Color::FromRGB(0, 0, 0, 15);
    Color tabActivePill    = Token::AccentDefault;
    Color textNormal       = Token::TextSecondary;
    Color textActive       = Token::TextPrimary;
    Color panelBackground  = Token::CardBackground;
    Color panelBorder      = Color::FromRGB(0, 0, 0, 15);
    Color headerBackground = Color::FromRGB(243, 243, 243);
    FontStyle font;
    float tabHeight        = 40.f;
    float tabPaddingH      = 16.f;
    float pillHeight       =  3.f;
    float pillRadius       =  1.5f;
    float tabRadius        =  6.f;
    // Auto-hide: collapse the tab bar when tabsVisible_ is false or when
    // only one tab is present. The panel fills the full bounds.
    bool  autoHide         = false;
};

// ────────────────────────────────────────────────────────── TooltipStyle ──────
struct TooltipStyle {
    Color background   = Color::FromRGB( 32,  32,  32);
    Color text         = Color::FromRGB(255, 255, 255);
    Color border       = Color::FromRGB( 64,  64,  64);
    FontStyle font;
    float paddingH     = 10.f;
    float paddingV     =  6.f;
    float cornerRadius =  6.f;
};

// ────────────────────────────────────────────────────────── ScrollbarStyle ────
struct ScrollbarStyle {
    Color track         = Color::Transparent;
    Color thumb         = Color::FromRGB(0, 0, 0,  80);
    Color thumbHover    = Color::FromRGB(0, 0, 0, 120);
    Color thumbPressed  = Color::FromRGB(0, 0, 0, 160);
    float width         =  6.f;     // narrow resting width
    float widthExpanded = 10.f;     // expanded on hover
    float minThumbSize  = 32.f;
    float cornerRadius  =  3.f;
    float margin        =  2.f;
};

} // namespace WinUI
