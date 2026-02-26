#ifndef COMPILERCPP_GUI_OBJECT_FOCUS
#define COMPILERCPP_GUI_OBJECT_FOCUS

#include <set>
#include <functional>

class focus_t {
private:
    std::set<std::function<void(void)>*> m_on{};
    std::set<std::function<void(void)>*> m_pending{};
    std::uint64_t m_depth{ 0 };
public:
    focus_t() = default;

    auto clear() -> void;
    auto exclusive(const std::function<void(void)>& lose) -> void;
    auto share(const std::function<void(void)>& lose) -> void;
};

auto focus_t::clear() -> void {
    ++m_depth;
    typename decltype(m_on)::key_type current{};
    for (auto i{ m_on.begin() }; i != m_on.end(); i = m_on.upper_bound(current)) {
        current = *i;
        m_on.erase(i);
        (*current)();
    }
    --m_depth;
    if (!m_depth) {
        m_on.insert(m_pending.begin(), m_pending.end());
        m_pending.clear();
    }
    return;
}

auto focus_t::exclusive(const std::function<void(void)>& lose) -> void {
    clear();
    share(lose);
    return;
}

auto focus_t::share(const std::function<void(void)>& lose) -> void {
    if (m_depth) {
        m_pending.emplace(new std::function<void(void)>{ lose });
    }
    else {
        m_on.emplace(new std::function<void(void)>{ lose });
    }
    return;
}

#endif