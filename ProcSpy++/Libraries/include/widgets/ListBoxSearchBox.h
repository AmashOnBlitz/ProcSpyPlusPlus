#pragma once
#include "Widget.h"
#include "../styles/Styles.h"
#include <vector>
#include <string>
#include <functional>

namespace WinUI {

// ─────────────────────────────────────────────────────────────────────────────
//  ListBox
// ─────────────────────────────────────────────────────────────────────────────
class ListBox : public Widget {
public:
    explicit ListBox(const Rect& bounds, ListBoxStyle style = {});
    void draw(Renderer& r) override;

    void addItem(std::wstring text, void* userData = nullptr);
    void insertItem(int index, std::wstring text, void* userData = nullptr);
    void removeItem(int index);
    void clearItems();

    int  itemCount() const               { return (int)items_.size(); }
    const std::wstring& itemText(int index) const;
    void* itemData(int index) const;
    void  setItemText(int index, std::wstring text);

    int  selectedIndex() const           { return selected_; }
    void setSelectedIndex(int idx, bool notify = true);
    void clearSelection()                { setSelectedIndex(-1, true); }
    bool hasSelection() const            { return selected_ >= 0; }

    void scrollToIndex(int index);
    int  topIndex() const                { return topIndex_; }
    void setStyle(ListBoxStyle s)        { style_ = std::move(s); }

    void setFilter(std::function<bool(const std::wstring&)> filter);
    void clearFilter()                   { setFilter(nullptr); }

    bool onMouseDown(float x, float y, int button) override;
    bool onMouseMove(float x, float y)             override;
    bool onMouseWheel(float delta)                  override;
    bool onKeyDown(const KeyEvent& e)               override;

    Signal<int> onSelectionChanged;
    Signal<int> onItemDoubleClicked;

    int  visibleItemCount() const;
    bool needsScrollbar()   const;

private:
    struct Item { std::wstring text; void* userData = nullptr; bool visible = true; };
    void rebuildVisible();

    std::vector<Item> items_;
    std::vector<int>  visibleIndices_;
    int           selected_      = -1;
    int           topIndex_      = 0;
    float         mouseY_        = 0.f;   // for per-row hover highlight
    ListBoxStyle  style_;

    std::function<bool(const std::wstring&)> filter_;

    DWORD lastClickTime_ = 0;
    int   lastClickIdx_  = -1;
};

// ─────────────────────────────────────────────────────────────────────────────
//  SearchBox
// ─────────────────────────────────────────────────────────────────────────────
class SearchBox : public Widget {
public:
    explicit SearchBox(const Rect& bounds, std::wstring placeholder = L"Search...",
                       SearchBoxStyle style = {});
    void draw(Renderer& r) override;

    const std::wstring& text()    const { return text_; }
    void  setText(std::wstring t, bool notify = true);
    void  clear(bool notify = true)     { setText(L"", notify); }

    const std::wstring& placeholder() const { return placeholder_; }
    void  setPlaceholder(std::wstring p)    { placeholder_ = std::move(p); }
    void  setStyle(SearchBoxStyle s)        { style_ = std::move(s); }
    void  setShowClearButton(bool b)        { showClear_ = b; }

    bool onMouseDown(float x, float y, int button) override;
    bool onChar(wchar_t c)                          override;
    bool onKeyDown(const KeyEvent& e)               override;
    void onFocusGained()                            override;
    void onFocusLost()                              override;

    Signal<const std::wstring&> onTextChanged;
    Signal<const std::wstring&> onSubmit;

private:
    void  insertChar(wchar_t c);
    void  deleteChar(bool forward);
    void  moveCaret(int delta, bool select);
    Rect  clearButtonRect() const;
    Rect  textRect()        const;
    float caretX()          const { return 0.f; }

    std::wstring   text_;
    std::wstring   placeholder_;
    SearchBoxStyle style_;
    bool           showClear_  = true;
    int            caretPos_   = 0;
    int            selStart_   = 0;
    float          scrollX_    = 0.f;
};

} // namespace WinUI
