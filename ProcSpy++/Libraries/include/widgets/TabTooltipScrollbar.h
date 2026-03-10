#pragma once
#include "Widget.h"
#include "../styles/Styles.h"
#include <vector>
#include <string>
#include <memory>

namespace WinUI {

class Renderer;

// ─────────────────────────────────────────────────────────────────────────────
//  TabControl
// ─────────────────────────────────────────────────────────────────────────────
class TabControl : public Widget {
public:
    explicit TabControl(const Rect& bounds, TabControlStyle style = {});
    void draw(Renderer& r) override;

    int  addTab(std::wstring label);
    void removeTab(int index);
    void setTabLabel(int index, std::wstring label);

    int  activeTab()  const { return activeTab_; }
    void setActiveTab(int index, bool notify = true);
    int  tabCount()   const { return (int)tabs_.size(); }

    Rect contentRect() const;
    void setStyle(TabControlStyle s) { style_ = std::move(s); }

    // ── Auto-hide / manual hide ────────────────────────────────────────────────
    bool tabBarVisible()   const { return tabsVisible_; }
    void showTabBar(bool notify = true);
    void hideTabBar(bool notify = true);
    void toggleTabBar()          { tabsVisible_ ? hideTabBar() : showTabBar(); }
    float effectiveHeaderHeight() const;

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;

    Signal<int>  onTabChanged;
    Signal<bool> onTabBarVisibilityChanged;

private:
    int tabAtX(float x) const;

    struct Tab { std::wstring label; float x, width; };
    std::vector<Tab> tabs_;
    int              activeTab_  = 0;
    int              hoveredTab_ = -1;
    bool             tabsVisible_ = true;
    TabControlStyle  style_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Tooltip
// ─────────────────────────────────────────────────────────────────────────────
class Tooltip : public Widget {
public:
    explicit Tooltip(TooltipStyle style = {});
    void draw(Renderer& r) override;

    void show(const std::wstring& text, float x, float y, Renderer& r);
    void hide();
    bool isShowing() const { return showing_; }
    void setStyle(TooltipStyle s) { style_ = std::move(s); }

private:
    std::wstring  text_;
    bool          showing_ = false;
    TooltipStyle  style_;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Scrollbar
//  Ultra-thin modern scrollbar: 6px at rest → 10px on hover (animated).
// ─────────────────────────────────────────────────────────────────────────────
class Scrollbar : public Widget {
public:
    enum class Orientation { Vertical, Horizontal };

    Scrollbar(const Rect& bounds, Orientation orient = Orientation::Vertical,
              ScrollbarStyle style = {});
    void draw(Renderer& r) override;

    void  setRange(float maxValue, float viewSize);
    float value()    const { return value_; }
    float maxValue() const { return maxValue_; }
    void  setValue(float v, bool notify = true);
    void  setStyle(ScrollbarStyle s) { style_ = std::move(s); }

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;
    bool onMouseUp  (float x, float y, int button) override;

    Signal<float> onScroll;

private:
    Rect   thumbRect()   const;
    float  posToValue(float pos) const;
    float  trackStart()  const;
    float  trackLength() const;

    Orientation    orient_;
    float          value_        = 0.f;
    float          maxValue_     = 100.f;
    float          viewSize_     = 10.f;
    bool           dragging_     = false;
    float          dragOffset_   = 0.f;
    mutable float  currentWidth_ = 6.f;   // animated: rest→expanded
    ScrollbarStyle style_;
};

} // namespace WinUI
