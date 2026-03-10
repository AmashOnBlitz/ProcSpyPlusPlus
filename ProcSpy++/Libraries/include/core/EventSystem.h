#pragma once
#include <functional>
#include <vector>

namespace WinUI {

template<typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;
    void connect(Slot slot)      { slots_.emplace_back(std::move(slot)); }
    void disconnect_all()        { slots_.clear(); }
    void emit(Args... args) const { for (const auto& s : slots_) s(args...); }
    void operator()(Args... args) const { emit(std::forward<Args>(args)...); }
    bool empty()  const { return slots_.empty(); }
    size_t count() const { return slots_.size(); }
private:
    std::vector<Slot> slots_;
};

struct MouseEvent { float x, y; int button; bool shift, ctrl, alt; };
struct KeyEvent   { int keyCode; wchar_t character; bool shift, ctrl, alt; };
struct ScrollEvent{ float delta; bool horizontal; };

} // namespace WinUI
