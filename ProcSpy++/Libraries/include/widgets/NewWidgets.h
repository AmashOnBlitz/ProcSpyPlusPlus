#pragma once
#include "Widget.h"
#include "../styles/Styles.h"
#include <string>
#include <vector>
#include <functional>

// For IFileOpenDialog
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d2d1.h>
#include <wrl/client.h>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  ToggleSwitch
//  Modern sliding binary toggle (Fluent / iOS style).
//
//  Layout:  [====O]  Label text
//  The thumb slides from left (off) to right (on) with a smooth animation.
// ─────────────────────────────────────────────────────────────────────────────
class ToggleSwitch : public Widget {
public:
    explicit ToggleSwitch(const Rect& bounds,
                          std::wstring label     = L"",
                          bool         isOn      = false,
                          ToggleSwitchStyle style = {});

    void draw(Renderer& r) override;

    bool  isOn()    const { return on_; }
    void  setOn(bool v, bool notify = true);
    void  toggle(bool notify = true) { setOn(!on_, notify); }

    void  setLabel(std::wstring l)    { label_ = std::move(l); }
    const std::wstring& label() const { return label_; }
    void  setStyle(ToggleSwitchStyle s) { style_ = std::move(s); }

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseUp  (float x, float y, int button) override;
    bool onKeyDown  (const KeyEvent& e)             override;

    Signal<bool> onToggled;   // emits new state

private:
    Rect trackRect() const;
    Rect thumbRect() const;

    std::wstring      label_;
    bool              on_           = false;
    mutable float     toggleAnim_   = 0.f;   // 0=off, 1=on, animated
    mutable DWORD     lastAnimTick_ = 0;
    ToggleSwitchStyle style_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  ImageWidget
//  Displays a D2D1Bitmap (loaded from file or raw bytes).
//  Also shows a friendly placeholder when no image is loaded.
//
//  Usage:
//    auto img = win.addWidget<WUI::ImageWidget>(Rect{...});
//    img->loadFromFile(L"photo.png");     // shell path, uses WIC
//    img->setBitmap(myBitmap);            // supply your own D2D bitmap
//    img->setFitMode(ImageWidget::Fit::Cover);
// ─────────────────────────────────────────────────────────────────────────────
class ImageWidget : public Widget {
public:
    enum class Fit {
        Fill,    // stretch to fill, ignore aspect ratio
        Contain, // letterbox / pillarbox (default)
        Cover,   // zoom to fill, crop edges
        None,    // pixel-perfect at natural size, clipped
    };
    enum class Shape {
        Rectangle,
        RoundedRectangle,
        Circle,     // avatar
    };

    explicit ImageWidget(const Rect& bounds, ImageWidgetStyle style = {});
    ~ImageWidget();

    void draw(Renderer& r) override;

    // ── Image source ──────────────────────────────────────────────────────────
    // Loads via WIC (supports BMP, PNG, JPEG, GIF, TIFF, ICO…).
    // Requires the render target; call after the window is shown.
    bool loadFromFile(const std::wstring& path, ID2D1HwndRenderTarget* rt);

    // Supply a pre-created D2D1Bitmap directly.
    void setBitmap(ID2D1Bitmap* bmp);
    void clearBitmap();
    bool hasBitmap() const { return bitmap_ != nullptr; }

    // ── Appearance ────────────────────────────────────────────────────────────
    void setFitMode(Fit f)    { fit_   = f; }
    void setShape(Shape s)    { shape_ = s; }
    void setStyle(ImageWidgetStyle s) { style_ = std::move(s); }
    void setCornerRadius(float r)     { style_.cornerRadius = r; }
    void setBadgeText(std::wstring t) { badge_ = std::move(t); }
    void clearBadge()                 { badge_.clear(); }

private:
    D2D1_RECT_F computeDestRect() const;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap_;
    ImageWidgetStyle style_;
    Fit   fit_   = Fit::Contain;
    Shape shape_ = Shape::Rectangle;
    std::wstring badge_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  ColorPicker
//  Inline Fluent-style HSV color picker.
//
//  Layout (stacked vertically):
//    ┌─────────────────────────────────┐
//    │   Saturation / Value square     │
//    ├─────────────────────────────────┤
//    │   Hue strip                     │
//    ├─────────────────────────────────┤
//    │   Alpha strip  (optional)       │
//    ├─────────────────────────────────┤
//    │  # RRGGBB  [        ]  preview  │
//    └─────────────────────────────────┘
//
//  All sub-areas are D2D gradient renders — no external images.
// ─────────────────────────────────────────────────────────────────────────────
class ColorPicker : public Widget {
public:
    explicit ColorPicker(const Rect& bounds, Color initial = Color::Accent,
                         ColorPickerStyle style = {});

    void draw(Renderer& r) override;

    // ── Color access ──────────────────────────────────────────────────────────
    Color color()       const { return hsvaToColor(); }
    float hue()         const { return hue_; }       // 0..360
    float saturation()  const { return sat_; }       // 0..1
    float value()       const { return val_; }       // 0..1
    float alpha()       const { return alpha_; }     // 0..1

    void setColor(const Color& c);
    void setHue(float h)   { hue_   = std::clamp(h,    0.f, 360.f); }
    void setSat(float s)   { sat_   = std::clamp(s,    0.f,   1.f); }
    void setVal(float v)   { val_   = std::clamp(v,    0.f,   1.f); }
    void setAlpha(float a) { alpha_ = std::clamp(a,    0.f,   1.f); }
    void setStyle(ColorPickerStyle s) { style_ = std::move(s); }

    Signal<Color> onColorChanged;

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;
    bool onMouseUp  (float x, float y, int button) override;

private:
    // Sub-rects
    Rect svRect()    const;   // Saturation/Value square
    Rect hueRect()   const;   // hue strip below sv
    Rect alphaRect() const;   // alpha strip below hue
    Rect hexRect()   const;   // hex input row

    void drawCheckerboard(Renderer& r, const Rect& rc) const;
    void drawSVGradient  (Renderer& r, const Rect& rc) const;
    void drawHueStrip    (Renderer& r, const Rect& rc) const;
    void drawAlphaStrip  (Renderer& r, const Rect& rc) const;

    Color hsvaToColor() const;
    void  colorToHSVA(const Color& c);
    void  hitSV   (float x, float y);
    void  hitHue  (float x);
    void  hitAlpha(float x);

    enum class DragZone { None, SV, Hue, Alpha };

    float hue_   = 0.f;
    float sat_   = 1.f;
    float val_   = 1.f;
    float alpha_ = 1.f;
    ColorPickerStyle style_;
    DragZone dragging_ = DragZone::None;
};

// ─────────────────────────────────────────────────────────────────────────────
//  FileDialog  (Win32 IFileOpenDialog / IFileSaveDialog wrapper)
//
//  Fully static — no widget, just a blocking call that shows the native
//  Windows file picker.
//
//  Usage:
//    std::wstring path;
//    if (WUI::FileDialog::openFile(hwnd, path, {{L"Images", L"*.png;*.jpg"}}))
//        img->loadFromFile(path, rt);
// ─────────────────────────────────────────────────────────────────────────────
struct FileFilter {
    std::wstring name;    // e.g. L"Images"
    std::wstring spec;    // e.g. L"*.png;*.jpg;*.bmp"
};

class FileDialog {
public:
    // Returns true if user picked a file; path is filled.
    static bool openFile(HWND parent,
                         std::wstring&              outPath,
                         const std::vector<FileFilter>& filters  = {},
                         const std::wstring&            title    = L"Open File");

    // Multi-select variant
    static bool openFiles(HWND parent,
                          std::vector<std::wstring>& outPaths,
                          const std::vector<FileFilter>& filters  = {},
                          const std::wstring&            title    = L"Open Files");

    // Save dialog
    static bool saveFile(HWND parent,
                         std::wstring&              outPath,
                         const std::vector<FileFilter>& filters  = {},
                         const std::wstring&            title    = L"Save File",
                         const std::wstring&            defExt   = L"");

    // Open a folder
    static bool openFolder(HWND parent,
                            std::wstring& outPath,
                            const std::wstring& title = L"Select Folder");
};

} // namespace WinUI
