// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  MyWinUI.h  —  single include for the entire MyWinUI library (2026 ed.)  ║
// ╚═══════════════════════════════════════════════════════════════════════════╝
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

// ── Core ──────────────────────────────────────────────────────────────────────
#include "core/Types.h"
#include "core/EventSystem.h"
#include "core/Renderer.h"
#include "core/Window.h"

// ── Styles ────────────────────────────────────────────────────────────────────
#include "styles/Styles.h"

// ── Widgets ───────────────────────────────────────────────────────────────────
#include "widgets/Widget.h"
#include "widgets/Button.h"
#include "widgets/LabelCheckboxRadio.h"
#include "widgets/SliderProgress.h"
#include "widgets/ListBoxSearchBox.h"
#include "widgets/TabTooltipScrollbar.h"
// ── New widgets (2026 edition) ────────────────────────────────────────────────
#include "widgets/TextBox.h"
#include "widgets/NewWidgets.h"

// ── Link libraries ────────────────────────────────────────────────────────────
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shell32.lib")

// ── Namespace alias ───────────────────────────────────────────────────────────
namespace WUI = WinUI;

// ── Entry-point helper macro ──────────────────────────────────────────────────
#define WINUI_MAIN(AppClass)                                                \
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int nCmdShow) {          \
    if (!WinUI::Application::instance().init()) return 1;                  \
    AppClass app;                                                            \
    app.run(nCmdShow);                                                       \
    return WinUI::Application::instance().run();                            \
}
