#pragma once
#include "Types.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  Renderer
//  Wraps D2D1HwndRenderTarget + DirectWrite.
//  Anti-aliasing: per-primitive (shapes crisp, not aliased).
//  New in 2026 edition:
//    fillGradientRect / fillRoundedRectGradient  – vertical linear gradients
//    drawShadow                                  – soft layered shadow
//    drawDoubleBorder                            – top/bottom asymmetric stroke
// ─────────────────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool    init(ID2D1HwndRenderTarget* rt, IDWriteFactory* dwrite);
    void    beginDraw();
    HRESULT endDraw();
    void    clear(const Color& color);
    void    resize(UINT width, UINT height);

    // ── Primitives ────────────────────────────────────────────────────────────
    void fillRect(const Rect& rect, const Color& color);
    void drawRect(const Rect& rect, const Color& color, float strokeWidth = 1.f);

    void fillRoundedRect(const Rect& rect, float rx, float ry, const Color& color);
    void drawRoundedRect(const Rect& rect, float rx, float ry,
                         const Color& color, float strokeWidth = 1.f);

    void fillCircle(float cx, float cy, float radius, const Color& color);
    void drawCircle(float cx, float cy, float radius,
                    const Color& color, float strokeWidth = 1.f);

    void drawLine(float x1, float y1, float x2, float y2,
                  const Color& color, float strokeWidth = 1.f);

    void fillArrow(float cx, float cy, float size, float angleDeg, const Color& color);

    // ── Gradient fills ────────────────────────────────────────────────────────
    // Vertical linear gradient from topColor → bottomColor
    void fillGradientRect(const Rect& rect, const Color& topColor, const Color& bottomColor);
    void fillRoundedRectGradient(const Rect& rect, float rx, float ry,
                                  const Color& topColor, const Color& bottomColor);

    // ── Soft shadow (no D2D effects required) ────────────────────────────────
    // Draws a multi-layer semi-transparent shadow *before* the control rect.
    // blur: spread in pixels.  opacity: max layer alpha multiplier (0..1).
    void drawShadow(const Rect& rect, float cornerRadius,
                    float blur = 8.f, float opacity = 1.f,
                    float offsetY = 2.f);

    // ── Asymmetric border (WinUI 3 "thicker bottom" stroke) ──────────────────
    // Draws top/side stroke at thin width, bottom stroke slightly darker/wider.
    void drawDoubleBorder(const Rect& rect, float rx,
                          const Color& colorTop, const Color& colorBottom,
                          float strokeWidth = 1.f);

    // ── Text ─────────────────────────────────────────────────────────────────
    void drawText(const std::wstring& text,
                  const Rect&         rect,
                  const Color&        color,
                  const std::wstring& fontName  = L"Segoe UI Variable Text",
                  float               fontSize  = 14.f,
                  DWRITE_TEXT_ALIGNMENT    hAlign = DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                  bool                bold      = false,
                  bool                clip      = true);

    Size measureText(const std::wstring& text,
                     const std::wstring& fontName = L"Segoe UI Variable Text",
                     float               fontSize = 14.f,
                     bool                bold     = false);

    // ── Clipping ─────────────────────────────────────────────────────────────
    void pushClip(const Rect& rect);
    void popClip();

    // ── Raw access ───────────────────────────────────────────────────────────
    ID2D1HwndRenderTarget* getRenderTarget()  const { return rt_; }
    IDWriteFactory*        getDWriteFactory() const { return dwrite_; }

private:
    ID2D1SolidColorBrush* getBrush(const Color& color);

    struct TextFormatKey {
        std::wstring           fontName;
        float                  fontSize;
        DWRITE_FONT_WEIGHT     weight;
        DWRITE_TEXT_ALIGNMENT  hAlign;
        DWRITE_PARAGRAPH_ALIGNMENT vAlign;
    };
    IDWriteTextFormat* getTextFormat(const TextFormatKey& key);

    ID2D1HwndRenderTarget* rt_     = nullptr;
    IDWriteFactory*        dwrite_ = nullptr;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;

    struct TFKeyHasher {
        size_t operator()(const TextFormatKey& k) const noexcept;
    };
    struct TFKeyEqual {
        bool operator()(const TextFormatKey& a, const TextFormatKey& b) const noexcept;
    };
    std::unordered_map<TextFormatKey,
                       Microsoft::WRL::ComPtr<IDWriteTextFormat>,
                       TFKeyHasher, TFKeyEqual> textFormats_;

    int clipDepth_ = 0;
};

} // namespace WinUI
