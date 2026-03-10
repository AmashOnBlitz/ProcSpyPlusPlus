#pragma once
#include "Widget.h"
#include "../styles/Styles.h"
#include <vector>
#include <string>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  TextBox
//  General-purpose text input.
//
//  Single-line:  standard one-liner, horizontally scrolling.
//  Multi-line:   set multiline(true); VK_RETURN inserts '\n'; text wraps
//                inside the widget bounds; vertical scrolling via wheel.
//
//  Features
//  ─────────
//  • Fluent 2 styling (same fill/border/focus tokens as SearchBox).
//  • Blinking caret, selection highlight.
//  • Ctrl+A  — select all.
//  • Ctrl+C / Ctrl+V / Ctrl+X — clipboard.
//  • Shift+Arrow — extend selection.
//  • Optional character limit (setMaxLength).
//  • Optional read-only mode.
//  • Optional password masking (setPasswordChar).
// ─────────────────────────────────────────────────────────────────────────────
class TextBox : public Widget {
public:
    explicit TextBox(const Rect& bounds,
                     std::wstring placeholder = L"",
                     TextBoxStyle style = {});

    void draw(Renderer& r) override;

    // ── Content ───────────────────────────────────────────────────────────────
    const std::wstring& text()    const { return text_; }
    void  setText(std::wstring t, bool notify = true);
    void  clear(bool notify = true)     { setText(L"", notify); }
    void  appendText(const std::wstring& t);

    // ── Options ───────────────────────────────────────────────────────────────
    bool multiline()        const { return multiline_; }
    void setMultiline(bool b)     { multiline_ = b; scrollY_ = 0.f; }

    const std::wstring& placeholder() const { return placeholder_; }
    void setPlaceholder(std::wstring p)     { placeholder_ = std::move(p); }

    void setMaxLength(int n)    { maxLength_ = n; }
    int  maxLength()    const   { return maxLength_; }

    void setReadOnly(bool b)    { readOnly_ = b; }
    bool readOnly()     const   { return readOnly_; }

    void setPasswordChar(wchar_t c) { passwordChar_ = c; }
    wchar_t passwordChar() const    { return passwordChar_; }

    void setStyle(TextBoxStyle s)   { style_ = std::move(s); }

    // ── Signals ───────────────────────────────────────────────────────────────
    Signal<const std::wstring&> onTextChanged;
    Signal<const std::wstring&> onSubmit;      // Enter in single-line mode

    // ── Input ─────────────────────────────────────────────────────────────────
    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;
    bool onMouseUp  (float x, float y, int button) override;
    bool onChar(wchar_t c)                          override;
    bool onKeyDown(const KeyEvent& e)               override;
    bool onMouseWheel(float delta)                  override;
    void onFocusGained()                            override;
    void onFocusLost()                              override;

private:
    // ── Internal text helpers ─────────────────────────────────────────────────
    void insertChar(wchar_t c);
    void insertString(const std::wstring& s);
    void deleteSelection();
    void deleteChar(bool forward);
    void moveCaret(int delta, bool select, bool byWord = false);
    void moveCaretVertical(int lines, bool select);
    void selectAll();
    bool hasSelection() const { return selStart_ != caretPos_; }
    std::wstring selectedText() const;
    void copyToClipboard()  const;
    void cutToClipboard();
    void pasteFromClipboard();

    // ── Layout helpers ────────────────────────────────────────────────────────
    Rect  textArea()                    const; // inner rect minus padding
    float lineHeight()                  const;
    int   lineCount()                   const;
    int   lineOf(int charIndex)         const;
    int   lineStart(int lineIdx)        const;
    int   lineEnd(int lineIdx)          const;
    float charX(int charIndex)          const; // X pixel within its line
    int   hitTest(float lx, float ly)   const; // pixel → char index (relative to textArea)
    std::wstring displayText()          const; // masked if passwordChar_ set
    // Scroll so caret is visible
    void  ensureCaretVisible();

    std::wstring   text_;
    std::wstring   placeholder_;
    TextBoxStyle   style_;
    bool           multiline_    = false;
    int            maxLength_    = -1;
    bool           readOnly_     = false;
    wchar_t        passwordChar_ = L'\0';

    int   caretPos_   = 0;
    int   selStart_   = 0;
    float scrollX_    = 0.f;   // single-line horizontal scroll
    float scrollY_    = 0.f;   // multi-line vertical scroll (pixels)

    bool  mouseDown_  = false; // tracking drag-selection
};

} // namespace WinUI
