#ifndef COMPILERCPP_GUI_OBJECT_OBJECT_DEF
#define COMPILERCPP_GUI_OBJECT_OBJECT_DEF

#include <functional>
#include <mutex>
#include <atomic>
#include <string>

#include "div_decl.hpp"

#include "../../lib/stu.hpp"

class GUI_object_t {
public:
    using status_t = std::uint8_t;
    using status_atomic_t = std::atomic_uint8_t;

    constexpr static status_t hidden{ 0 };
    constexpr static status_t showed{ 1 };
    constexpr static status_t hiding{ 2 };
    constexpr static status_t showing{ 3 };
protected:
    mutable std::recursive_mutex m_mutex{};
    std::string m_name{};
private:
    status_atomic_t m_status{};
    std::function<void(size_2D)> m_size_callback{};
public:
    GUI_object_t() = default;
    GUI_object_t(std::string name);
    GUI_object_t(const GUI_object_t& in);
    virtual ~GUI_object_t() = default;

    auto operator=(const GUI_object_t& in) -> GUI_object_t&;

    virtual auto get_pos() const -> pos_2D = 0;
    virtual auto set_pos(pos_2D pos) -> void = 0;
    virtual auto get_size() const -> size_2D = 0;
    virtual auto set_size(size_2D size) -> void;
    auto set_size_callback(std::function<void(size_2D)> callback) -> void;
    virtual auto get_margin() const -> size_1D = 0;
public:
    auto show() -> void;
    auto hide() -> void;
protected:
    virtual auto show_impl(bool base) -> void = 0;
    auto show_begin(bool base) -> bool;
    auto show_end(bool base) -> void;
    virtual auto hide_impl(bool base) -> void = 0;
    auto hide_begin(bool base) -> bool;
    auto hide_end(bool base) -> void;
    auto status() const -> status_t;
};

#endif