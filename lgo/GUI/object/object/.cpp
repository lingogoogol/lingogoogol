module lgo.GUI.object.object;

import std;

import lgo.io.file;
import lgo.math.vec;

namespace lgo
{
    #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
    namespace
    {
        std::atomic_uint64_t next_gui_id{};
    }
    #endif

    #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
    GUI_object_t::GUI_object_t(std::source_location location)
    {
        m_debug_id = ++next_gui_id;
        // source_location records the calling constructor, including template
        // arguments, without compiler-specific demangling or RTTI dependencies.
        m_name = std::string{ location.function_name() } + "#" + std::to_string(m_debug_id);
        if constexpr (gui_object_events_enabled) debug_event("create");
    }
    #endif

    GUI_object_t::GUI_object_t(const GUI_object_t& in)
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        : GUI_object_t(std::source_location::current())
        #endif
    {
        std::unique_lock lock{ in.m_mutex };
        m_status = in.m_status.load();
        m_size_callback = in.m_size_callback;
        m_parent_clip = in.m_parent_clip;
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        m_name = in.m_name + "/copy#" + std::to_string(m_debug_id);
        if constexpr (gui_object_events_enabled) debug_event("copy");
        #else
        m_name = in.m_name;
        #endif
    }

    GUI_object_t::GUI_object_t(std::string name)
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        : GUI_object_t(std::source_location::current())
        #endif
    {
        name_set(std::move(name));
    }

    #ifdef DEBUG_GUI_OBJECT_LOG
    GUI_object_t::~GUI_object_t()
    {
        debug_event("destroy");
    }
    #endif

    auto GUI_object_t::debug_event([[maybe_unused]] std::string_view event) const noexcept -> void
    {
        #ifdef DEBUG_GUI_OBJECT_LOG
        // Diagnostics must not throw from destructors or native callback paths.
        try
        {
            std::unique_lock lock{ m_mutex };
            log_file(std::format("GUI {} id={} object={} name={}\n", event, m_debug_id,
                static_cast<const void*>(this), m_name));
        }
        catch (...) {}
        #endif
    }

    auto GUI_object_t::debug_attach([[maybe_unused]] const void* parent,
        [[maybe_unused]] std::uint64_t id) const noexcept -> void
    {
        #ifdef DEBUG_GUI_OBJECT_LOG
        try
        {
            std::unique_lock lock{ m_mutex };
            log_file(std::format("GUI attach id={} parent={} slot={} name={}\n", m_debug_id, parent, id, m_name));
        }
        catch (...) {}
        #endif
    }

    auto GUI_object_t::operator=(const GUI_object_t& in) -> GUI_object_t&
    {
        m_status = in.m_status.load();
        m_size_callback = in.m_size_callback;
        m_parent_clip = in.m_parent_clip;
        return *this;
    }

    auto GUI_object_t::name_get() -> std::string
    {
        std::unique_lock lock{ m_mutex };
        return m_name;
    }

    auto GUI_object_t::name_set(std::string name) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_name = std::move(name);
        if constexpr (gui_object_events_enabled) debug_event("rename");
        return;
    }

    auto GUI_object_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        if (m_size_callback)
        {
            m_size_callback(size);
        }
        return;
    }

    auto GUI_object_t::set_size_callback(std::function<void(size_2D)> callback) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_size_callback = callback;
        return;
    }

    auto GUI_object_t::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        std::unique_lock lock{ m_mutex };
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: GUI_object_t\n");
        log_file(tab(tab_count + 1) + "name: " + m_name + "\n");
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        log_file(tab(tab_count + 1) + "id: " + std::to_string(m_debug_id) + "\n");
        #endif
        std::string status_str{};
        switch (m_status)
        {
            case hidden:
            {
                status_str = "hidden";
                break;
            }
            case shown:
            {
                status_str = "shown";
                break;
            }
            case hiding:
            {
                status_str = "hiding";
                break;
            }
            case showing:
            {
                status_str = "showing";
                break;
            }
        }
        log_file(tab(tab_count + 1) + "status: " + status_str + "\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto GUI_object_t::show() -> void
    {
        show_impl(true);
        return;
    }

    auto GUI_object_t::hide() -> void
    {
        hide_impl(true);
        return;
    }

    auto GUI_object_t::show_begin(bool base) -> bool
    {
        if (!base)
        {
            return true;
        }
        while (true)
        {
            std::unique_lock lock{ m_mutex };
            if (m_status == shown)
            {
                return false;
            }
            else if (m_status == hidden)
            {
                if (base)
                {
                    m_status = showing;
                }
                return true;
            }
        }
    }

    auto GUI_object_t::show_end(bool base) -> void
    {
        if (base)
        {
            m_status = shown;
            if constexpr (gui_object_events_enabled) debug_event("show");
        }
        return;
    }

    auto GUI_object_t::hide_begin(bool base) -> bool
    {
        if (!base)
        {
            return true;
        }
        while (true)
        {
            std::unique_lock lock{ m_mutex };
            if (m_status == hidden)
            {
                return false;
            }
            else if (m_status == shown)
            {
                if (base)
                {
                    m_status = hiding;
                }
                return true;
            }
        }
    }

    auto GUI_object_t::hide_end(bool base) -> void
    {
        if (base)
        {
            m_status = hidden;
            if constexpr (gui_object_events_enabled) debug_event("hide");
        }
        return;
    }

    auto GUI_object_t::status() const -> status_t
    {
        return m_status;
    }
}
