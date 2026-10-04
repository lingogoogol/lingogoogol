export module lgo.GUI.object.state;

import std;

import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.focus;
import lgo.GUI.object.depth_tracker;

export namespace lgo
{
    class state_t
    {
    private:
        struct instance_t
        {
            std::map<std::uint64_t, std::shared_ptr<GUI_object_t>> m_object{};
            focus_t m_focus{};
        };

        engine_t* m_engine{};
        depth_tracker_t* m_depth_tracker{};
    protected:
        instance_t m_current{};
        std::uint64_t m_current_id_object{};
    private:
        std::uint64_t m_current_id_save{};
        std::map<std::uint64_t, instance_t> m_saved{};
    public:
        state_t() = default;
        state_t(engine_t* engine, depth_tracker_t* depth_tracker);
        state_t(const state_t&) = delete;
        auto init(engine_t* engine, depth_tracker_t* depth_tracker) -> void;
        auto operator=(const state_t&) = delete;

        auto get_engine() -> engine_t*;
        auto get_depth_tracker() -> depth_tracker_t*;
        auto get_focus() -> focus_t*;

        template<typename t_object, typename... t_arg>
        auto add_object(t_arg&&... arg) -> std::uint64_t;
        template<typename t_object, typename t_self, typename... t_arg>
        auto add_object_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object>;
        template<typename t_object, typename t_self, typename... t_arg>
        auto add_object_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object>;
        template<typename t_object>
        auto get_object_weak(std::uint64_t id) -> std::weak_ptr<t_object>;
        template<typename t_object>
        auto get_object_shared(std::uint64_t id) -> std::shared_ptr<t_object>;
        auto remove_object(std::uint64_t id) -> void;
        auto save_state() -> std::uint64_t;
        auto clear_state() -> void;
        auto restore_state(std::uint64_t id) -> void;
        auto remove_state(std::uint64_t id) -> void;

        auto log(std::uint32_t tab_count) const -> void;
    };

    template<typename t_object, typename... t_arg>
    auto state_t::add_object(t_arg&&... arg) -> std::uint64_t
    {
        m_current.m_object.emplace
        (
            m_current_id_object,
            std::make_shared<t_object>
            (
                m_engine,
                m_depth_tracker,
                std::forward<t_arg>(arg)...
            )
        );
        m_current.m_object[m_current_id_object]->show();
        return m_current_id_object++;
    }

    template<typename t_object, typename t_self, typename... t_arg>
    auto state_t::add_object_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object>
    {
        std::uint64_t id{ self.template add_object<t_object>(std::forward<t_arg>(arg)...) };
        return self.template get_object_weak<t_object>(id);
    }

    template<typename t_object, typename t_self, typename... t_arg>
    auto state_t::add_object_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object>
    {
        std::uint64_t id{ self.template add_object<t_object>(std::forward<t_arg>(arg)...) };
        return self.template get_object_shared<t_object>(id);
    }

    template<typename t_object>
    auto state_t::get_object_weak(std::uint64_t id) -> std::weak_ptr<t_object>
    {
        return std::dynamic_pointer_cast<t_object>(m_current.m_object[id]);
    }

    template<typename t_object>
    auto state_t::get_object_shared(std::uint64_t id) -> std::shared_ptr<t_object>
    {
        return std::dynamic_pointer_cast<t_object>(m_current.m_object[id]);
    }
}
