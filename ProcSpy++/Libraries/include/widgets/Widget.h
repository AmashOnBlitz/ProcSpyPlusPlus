#pragma once
#include "../core/Types.h"
#include "../core/EventSystem.h"
#include "../core/Renderer.h"
#include <string>
#include <memory>
#include <algorithm>
#include <windows.h>  // GetTickCount

namespace WinUI {

class Window;

// ─────────────────────────────────────────────────────────────────────────────
//  Widget
//  Abstract base for every UI element.
//
//  Animation helpers (2026 edition):
//    tickAnim()        — call at the top of draw(); advances hoverAnim_/pressAnim_
//    hoverAnim()       — 0..1 smooth hover progress
//    pressAnim()       — 0..1 smooth press progress
//    lerpColor(a,b,t)  — static convenience
// ─────────────────────────────────────────────────────────────────────────────
class Widget {
public:
    explicit Widget(const Rect& bounds);
    virtual ~Widget() = default;

    Widget(const Widget&)            = delete;
    Widget& operator=(const Widget&) = delete;

    // ── Layout ────────────────────────────────────────────────────────────────
    const Rect& bounds()   const { return bounds_; }
    void  setBounds(const Rect& r)     { bounds_ = r; onBoundsChanged(); }
    void  setPosition(float x, float y){ bounds_.x = x; bounds_.y = y; onBoundsChanged(); }
    void  setSize(float w, float h)    { bounds_.width = w; bounds_.height = h; onBoundsChanged(); }
    float x()      const { return bounds_.x; }
    float y()      const { return bounds_.y; }
    float width()  const { return bounds_.width; }
    float height() const { return bounds_.height; }

    // ── Visibility / enabled ──────────────────────────────────────────────────
    bool visible()  const { return visible_;  }
    bool enabled()  const { return enabled_;  }
    void setVisible(bool v) { visible_ = v; }
    void setEnabled(bool e) { enabled_ = e; }

    // ── State ─────────────────────────────────────────────────────────────────
    bool hovered() const { return hovered_; }
    bool pressed() const { return pressed_; }
    bool focused() const { return focused_; }

    // ── Tooltip ───────────────────────────────────────────────────────────────
    const std::wstring& tooltipText() const { return tooltip_; }
    void setTooltip(const std::wstring& t)  { tooltip_ = t; }

    // ── Rendering ─────────────────────────────────────────────────────────────
    virtual void draw(Renderer& r) = 0;

    // ── Input (called by Window) ───────────────────────────────────────────────
    virtual bool onMouseMove(float x, float y);
    virtual bool onMouseDown(float x, float y, int button);
    virtual bool onMouseUp  (float x, float y, int button);
    virtual bool onMouseLeave();
    virtual bool onMouseWheel(float delta);
    virtual bool onKeyDown(const KeyEvent& e);
    virtual bool onKeyUp  (const KeyEvent& e);
    virtual bool onChar(wchar_t c);
    virtual void onFocusGained();
    virtual void onFocusLost();

    // ── Signals ───────────────────────────────────────────────────────────────
    Signal<>            onHoverEnter;
    Signal<>            onHoverLeave;
    Signal<float,float> onMouseMoved;

    // ── Animation helpers ─────────────────────────────────────────────────────
    // Call tickAnim() at the start of draw(); then use hoverAnim()/pressAnim()
    // to interpolate colors for smooth transitions.
    static Color lerpColor(const Color& a, const Color& b, float t) {
        return a.lerp(b, t);
    }

protected:
    virtual void onBoundsChanged() {}

    // Advances hoverAnim_ and pressAnim_ based on elapsed wall-clock time.
    // Safe to call every frame; first call initialises lastAnimTick_.
    void tickAnim() const {
        DWORD now = GetTickCount();
        if (lastAnimTick_ == 0) lastAnimTick_ = now;
        float dt = std::min((float)(now - lastAnimTick_) * 0.001f, 0.05f);
        lastAnimTick_ = now;

        bool  wantHover  = hovered_ && enabled_;
        bool  wantPress  = pressed_ && hovered_ && enabled_;

        const float kHoverSpeed = 1.f / 0.12f;   // 120 ms
        const float kPressSpeed = 1.f / 0.06f;   // 60 ms

        float hTarget = wantHover ? 1.f : 0.f;
        float pTarget = wantPress ? 1.f : 0.f;

        hoverAnim_ = lerp1(hoverAnim_, hTarget, dt * kHoverSpeed);
        pressAnim_ = lerp1(pressAnim_, pTarget, dt * kPressSpeed);
    }

    float hoverAnim() const { return hoverAnim_; }
    float pressAnim() const { return pressAnim_; }

    Rect         bounds_;
    bool         visible_  = true;
    bool         enabled_  = true;
    bool         hovered_  = false;
    bool         pressed_  = false;
    bool         focused_  = false;
    std::wstring tooltip_;

private:
    static float lerp1(float a, float b, float speed) {
        if (b > a) return std::min(a + speed, b);
        return std::max(a - speed, b);
    }

    mutable float hoverAnim_    = 0.f;
    mutable float pressAnim_    = 0.f;
    mutable DWORD lastAnimTick_ = 0;
};

} // namespace WinUI
