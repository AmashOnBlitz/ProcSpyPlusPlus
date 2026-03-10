#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <cstdint>
#include <algorithm>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  Color
// ─────────────────────────────────────────────────────────────────────────────
struct Color {
    float r, g, b, a;

    constexpr Color(float r = 0.f, float g = 0.f, float b = 0.f, float a = 1.f)
        : r(r), g(g), b(b), a(a) {}

    static constexpr Color FromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
    }

    D2D1_COLOR_F toD2D() const { return D2D1::ColorF(r, g, b, a); }

    Color lerp(const Color& other, float t) const {
        t = std::clamp(t, 0.f, 1.f);
        return { r + (other.r - r)*t, g + (other.g - g)*t,
                 b + (other.b - b)*t, a + (other.a - a)*t };
    }
    Color withAlpha(float newAlpha) const { return { r, g, b, newAlpha }; }
    Color darken(float amount) const {
        return { std::max(0.f, r-amount), std::max(0.f, g-amount),
                 std::max(0.f, b-amount), a };
    }
    Color lighten(float amount) const {
        return { std::min(1.f, r+amount), std::min(1.f, g+amount),
                 std::min(1.f, b+amount), a };
    }

    static const Color White;
    static const Color Black;
    static const Color Transparent;
    static const Color Accent;
    static const Color AccentDark;
    static const Color LightGray;
    static const Color DarkGray;
    static const Color Red;
    static const Color Green;
};

inline const Color Color::White       = { 1.f, 1.f, 1.f, 1.f };
inline const Color Color::Black       = { 0.f, 0.f, 0.f, 1.f };
inline const Color Color::Transparent = { 0.f, 0.f, 0.f, 0.f };
inline const Color Color::Accent      = Color::FromRGB(  0, 103, 192);
inline const Color Color::AccentDark  = Color::FromRGB(  0,  84, 166);
inline const Color Color::LightGray   = Color::FromRGB(240, 240, 240);
inline const Color Color::DarkGray    = Color::FromRGB(102, 102, 102);
inline const Color Color::Red         = Color::FromRGB(196,  43,  28);
inline const Color Color::Green       = Color::FromRGB( 16, 124,  16);

// ─────────────────────────────────────────────────────────────────────────────
//  Point / Size / Rect
// ─────────────────────────────────────────────────────────────────────────────
struct Point {
    float x, y;
    constexpr Point(float x = 0.f, float y = 0.f) : x(x), y(y) {}
    D2D1_POINT_2F toD2D() const { return D2D1::Point2F(x, y); }
    Point operator+(const Point& o) const { return { x+o.x, y+o.y }; }
    Point operator-(const Point& o) const { return { x-o.x, y-o.y }; }
};

struct Size {
    float width, height;
    constexpr Size(float w = 0.f, float h = 0.f) : width(w), height(h) {}
};

struct Rect {
    float x, y, width, height;
    constexpr Rect(float x = 0.f, float y = 0.f, float w = 0.f, float h = 0.f)
        : x(x), y(y), width(w), height(h) {}

    float right()  const { return x + width;  }
    float bottom() const { return y + height; }

    bool contains(float px, float py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }
    bool contains(const Point& p) const { return contains(p.x, p.y); }

    D2D1_RECT_F toD2D() const { return D2D1::RectF(x, y, right(), bottom()); }

    Rect deflate(float dx, float dy) const {
        return { x+dx, y+dy, std::max(0.f, width-2*dx), std::max(0.f, height-2*dy) };
    }
    Rect deflate(float d) const { return deflate(d, d); }
    Rect translate(float dx, float dy) const { return { x+dx, y+dy, width, height }; }

    bool intersects(const Rect& o) const {
        return !(right() <= o.x || o.right() <= x || bottom() <= o.y || o.bottom() <= y);
    }
    Point center() const { return { x + width*0.5f, y + height*0.5f }; }
};

} // namespace WinUI
