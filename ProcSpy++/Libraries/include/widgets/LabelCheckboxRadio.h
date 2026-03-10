#pragma once
#include "Widget.h"
#include "../styles/Styles.h"
#include <dwrite.h>

namespace WinUI {

// ── Label ─────────────────────────────────────────────────────────────────────
class Label : public Widget {
public:
    Label(const Rect& bounds, std::wstring text = L"", LabelStyle style = {});
    void draw(Renderer& r) override;

    void               setText(std::wstring t) { text_ = std::move(t); }
    const std::wstring& getText() const        { return text_; }
    void               setStyle(LabelStyle s)  { style_ = std::move(s); }
    void setHAlign(DWRITE_TEXT_ALIGNMENT a)       { hAlign_ = a; }
    void setVAlign(DWRITE_PARAGRAPH_ALIGNMENT a)   { vAlign_ = a; }

private:
    std::wstring               text_;
    LabelStyle                 style_;
    DWRITE_TEXT_ALIGNMENT      hAlign_ = DWRITE_TEXT_ALIGNMENT_LEADING;
    DWRITE_PARAGRAPH_ALIGNMENT vAlign_ = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
};

// ── Checkbox ─────────────────────────────────────────────────────────────────
class Checkbox : public Widget {
public:
    Checkbox(const Rect& bounds, std::wstring text = L"",
             bool checked = false, CheckboxStyle style = {});
    void draw(Renderer& r) override;

    bool isChecked() const  { return checked_; }
    void setChecked(bool c) { bool prev = checked_; checked_ = c; if (prev != c) onCheckedChanged.emit(c); }
    void toggle()           { setChecked(!checked_); }
    void setStyle(CheckboxStyle s) { style_ = std::move(s); }
    void setText(std::wstring t)   { text_  = std::move(t); }

    bool onMouseUp(float x, float y, int button) override;
    Signal<bool> onCheckedChanged;

private:
    std::wstring  text_;
    bool          checked_;
    CheckboxStyle style_;
};

// ── RadioButton ───────────────────────────────────────────────────────────────
class RadioButton : public Widget {
public:
    RadioButton(const Rect& bounds, std::wstring text = L"",
                int groupId = 0, int id = 0, RadioButtonStyle style = {});
    void draw(Renderer& r) override;

    bool isSelected() const  { return selected_; }
    void setSelected(bool s) { selected_ = s; }
    int  groupId() const     { return groupId_; }
    int  id()      const     { return id_; }
    void setStyle(RadioButtonStyle s) { style_ = std::move(s); }
    void setText(std::wstring t)      { text_  = std::move(t); }

    bool onMouseUp(float x, float y, int button) override;
    Signal<int> onSelected;

private:
    std::wstring     text_;
    int              groupId_, id_;
    bool             selected_ = false;
    RadioButtonStyle style_;
};

} // namespace WinUI
