#ifndef COMPILERCPP_GUI_OBJECT_OBJECT_IMPL
#define COMPILERCPP_GUI_OBJECT_OBJECT_IMPL

#include "object_def.hpp"
#include "div_def.hpp"

auto GUI_object_t::set_size(size_2D size) -> void {
    std::unique_lock lock{ m_mutex };
    if (m_size_callback) {
        m_size_callback(size);
    }
    return;
}

auto GUI_object_t::set_size_callback(std::function<void(size_2D)> callback) -> void {
    std::unique_lock lock{ m_mutex };
    m_size_callback = callback;
    return;
}

auto GUI_object_t::show() -> void {
    show_impl(true);
    return;
}

auto GUI_object_t::hide() -> void {
    hide_impl(true);
    return;
}

auto GUI_object_t::show_begin(bool base) -> bool {
    if (!base) {
        return true;
    }
    while (true) {
        std::unique_lock lock{ m_mutex };
        if (m_status == showed) {
            return false;
        }
        else if (m_status == hidden) {
            if (base) {
                m_status = showing;
            }
            return true;
        }
    }
}

auto GUI_object_t::show_end(bool base) -> void {
    if (base) {
        m_status = showed;
    }
    return;
}

auto GUI_object_t::hide_begin(bool base) -> bool {
    if (!base) {
        return true;
    }
    while (true) {
        std::unique_lock lock{ m_mutex };
        if (m_status == hidden) {
            return false;
        }
        else if (m_status == showed) {
            if (base) {
                m_status = hiding;
            }
            return true;
        }
    }
}

auto GUI_object_t::hide_end(bool base) -> void {
    if (base) {
        m_status = hidden;
    }
    return;
}

auto GUI_object_t::status() const -> status_t {
    return m_status;
}

#endif