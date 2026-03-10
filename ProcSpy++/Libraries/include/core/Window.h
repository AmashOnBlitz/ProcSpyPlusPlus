#pragma once
#include "Types.h"
#include "Renderer.h"
#include "../widgets/Widget.h"
#include "../widgets/TabTooltipScrollbar.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace WinUI {

class Window;

// ─────────────────────────────────────────────────────────────────────────────
//  Application  — singleton; owns D2D/DWrite factories, runs the message loop
// ─────────────────────────────────────────────────────────────────────────────
class Application {
public:
    static Application& instance();
    bool init();
    int  run();
    void quit(int exitCode = 0);

    ID2D1Factory*   d2dFactory()    const { return d2dFactory_.Get(); }
    IDWriteFactory* dwriteFactory() const { return dwriteFactory_.Get(); }

    void registerWindow(Window* w);
    void unregisterWindow(Window* w);

private:
    Application() = default;
    Microsoft::WRL::ComPtr<ID2D1Factory>   d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    std::vector<Window*>                   windows_;
    int                                    exitCode_ = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  WindowConfig
// ─────────────────────────────────────────────────────────────────────────────
struct WindowConfig {
    std::wstring title      = L"MyWinUI App";
    int          x          = CW_USEDEFAULT;
    int          y          = CW_USEDEFAULT;
    int          width      = 800;
    int          height     = 600;
    bool         resizable  = true;
    Color        background = Color::FromRGB(243, 243, 243);  // Mica base
    HWND         parent     = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Window
// ─────────────────────────────────────────────────────────────────────────────
class Window {
public:
    explicit Window(const WindowConfig& cfg = {});
    ~Window();
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // ── Widget factory ────────────────────────────────────────────────────────
    template<typename T, typename... Args>
    std::shared_ptr<T> addWidget(Args&&... args) {
        auto w = std::make_shared<T>(std::forward<Args>(args)...);
        widgets_.push_back(w);
        return w;
    }
    void removeWidget(const std::shared_ptr<Widget>& w);

    // ── State ─────────────────────────────────────────────────────────────────
    HWND  hwnd()   const { return hwnd_; }
    bool  isOpen() const { return hwnd_ != nullptr; }
    void  close();
    void  show(int nCmdShow = SW_SHOW);
    void  hide();
    void  setTitle(const std::wstring& title);
    void  setBackground(const Color& c) { background_ = c; }
    Size  clientSize() const;

    // ── Focus ─────────────────────────────────────────────────────────────────
    void setFocus(const std::shared_ptr<Widget>& w);
    std::shared_ptr<Widget> focusedWidget() const { return focused_.lock(); }

    Tooltip&  tooltip()  { return *tooltip_; }
    Renderer& renderer() { return renderer_; }

    void bindRadioGroup(int groupId);

    // ── Signals ───────────────────────────────────────────────────────────────
    Signal<>         onClose;
    Signal<int, int> onResize;
    Signal<KeyEvent> onKeyDown;
    Signal<KeyEvent> onKeyUp;

private:
    static LRESULT CALLBACK s_WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(UINT msg, WPARAM wp, LPARAM lp);

    bool createD2DResources();
    void releaseD2DResources();

    void handlePaint();
    void handleResize(int w, int h);
    void handleMouseMove(int x, int y);
    void handleMouseDown(int x, int y, int button);
    void handleMouseUp  (int x, int y, int button);
    void handleMouseWheel(int x, int y, float delta);
    void handleKeyDown(WPARAM vk, bool shift, bool ctrl, bool alt);
    void handleKeyUp  (WPARAM vk);
    void handleChar(wchar_t c);

    HWND                                          hwnd_     = nullptr;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> rt_;
    Renderer                                      renderer_;
    Color                                         background_ = Color::FromRGB(243, 243, 243);

    std::vector<std::shared_ptr<Widget>> widgets_;
    std::weak_ptr<Widget>                focused_;
    std::weak_ptr<Widget>                hovered_;

    std::unique_ptr<Tooltip>             tooltip_;
    float                                mouseX_ = 0.f, mouseY_ = 0.f;
    DWORD                                hoverStartTime_ = 0;
    bool                                 tooltipShowing_ = false;

    static constexpr DWORD kTooltipDelayMs = 600;
};

} // namespace WinUI
