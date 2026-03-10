#pragma once
#include "Widget.h"
#include "../styles/Styles.h"

namespace WinUI {

// ── Slider ────────────────────────────────────────────────────────────────────
class Slider : public Widget {
public:
    Slider(const Rect& bounds, float minVal = 0.f, float maxVal = 100.f,
           float value = 0.f, SliderStyle style = {});
    void draw(Renderer& r) override;

    float value()    const { return value_; }
    float minValue() const { return minVal_; }
    float maxValue() const { return maxVal_; }
    void  setValue(float v, bool notify = true);
    void  setRange(float minV, float maxV);
    void  setStep(float step)  { step_ = step; }
    void  setStyle(SliderStyle s) { style_ = std::move(s); }

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;
    bool onMouseUp  (float x, float y, int button) override;
    bool onKeyDown  (const KeyEvent& e)            override;

    Signal<float> onValueChanged;

private:
    float thumbCenterX() const;
    float xToValue(float x) const;

    float      minVal_, maxVal_, value_;
    float      step_    = 0.f;
    bool       dragging_= false;
    SliderStyle style_;
};

// ── ProgressBar ───────────────────────────────────────────────────────────────
class ProgressBar : public Widget {
public:
    ProgressBar(const Rect& bounds, float value = 0.f, ProgressBarStyle style = {});
    void draw(Renderer& r) override;

    float value()  const { return value_; }
    void  setValue(float v) { value_ = std::clamp(v, 0.f, 100.f); }
    void  setStyle(ProgressBarStyle s) { style_ = std::move(s); }
    void  setIndeterminate(bool b)     { indeterminate_ = b; }
    bool  isIndeterminate() const      { return indeterminate_; }
    void  tick(float deltaSeconds);

private:
    float            value_;
    bool             indeterminate_ = false;
    float            animPos_       = 0.f;
    ProgressBarStyle style_;
};

} // namespace WinUI
