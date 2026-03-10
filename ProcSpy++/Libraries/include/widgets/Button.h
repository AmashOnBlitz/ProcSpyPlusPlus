#pragma once
#include "Widget.h"
#include "../styles/Styles.h"

namespace WinUI {

class Button : public Widget {
public:
    Button(const Rect& bounds, std::wstring text = L"Button", ButtonStyle style = {});
    void draw(Renderer& r) override;

    void              setText(std::wstring t)  { text_ = std::move(t); }
    const std::wstring& getText() const        { return text_; }
    void              setStyle(ButtonStyle s)  { style_ = std::move(s); }
    const ButtonStyle& getStyle() const        { return style_; }

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseUp  (float x, float y, int button) override;

    Signal<> onClick;

private:
    std::wstring text_;
    ButtonStyle  style_;
};

} // namespace WinUI
