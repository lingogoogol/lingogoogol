#ifndef COMPILERCPP_GUI_OBJECT_VIRT
#define COMPILERCPP_GUI_OBJECT_VIRT

#include <memory>

#include "object_def.hpp"

#include "../primitive/engine_def.hpp"

template<typename t_type>
class virt_t: public GUI_object_t {
public:
    template<typename... u_arg>
    virt_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size
    , size_1D margin, size_2D displacement, u_arg&&... arg);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto obj_get() const -> const std::shared_ptr<t_type>;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
private:
    std::shared_ptr<t_type> m_obj{};
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_margin{};
    size_2D m_displacement{};
};

template<typename t_type>
template<typename... u_arg>
virt_t<t_type>::virt_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, size_2D size, size_1D margin, size_2D displacement, u_arg&&... arg)
: m_obj{ std::make_shared<t_type>(engine, depth_tracker, pos + displacement, std::forward<u_arg&&>(arg)...) }
, m_pos{ pos }, m_size{ size }, m_margin{ margin }, m_displacement{ displacement } {}

template<typename t_type>
auto virt_t<t_type>::get_pos() const -> pos_2D {
    return m_pos;
}

template<typename t_type>
auto virt_t<t_type>::set_pos(pos_2D pos) -> void {
    m_pos = pos;
    m_obj->set_pos(pos + m_displacement);
    return;
}

template<typename t_type>
auto virt_t<t_type>::get_size() const -> size_2D {
    return m_size;
}

template<typename t_type>
auto virt_t<t_type>::get_margin() const -> size_1D {
    return m_margin;
}

template<typename t_type>
auto virt_t<t_type>::obj_get() const -> const std::shared_ptr<t_type> {
    return m_obj;
}

template<typename t_type>
auto virt_t<t_type>::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_obj->show();
    show_end(base);
    return;
}

template<typename t_type>
auto virt_t<t_type>::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_obj->hide();
    hide_end(base);
    return;
}

#endif