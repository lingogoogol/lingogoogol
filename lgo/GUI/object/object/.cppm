export module lgo.GUI.object.object;

import std;

import lgo.math.vec;

export namespace lgo
{
    inline constexpr bool gui_object_names_enabled =
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        true;
        #else
        false;
        #endif

    inline constexpr bool gui_object_events_enabled =
        #ifdef DEBUG_GUI_OBJECT_LOG
        true;
        #else
        false;
        #endif

    struct clip_rect_t
    {
        pos_2D pos{};
        size_2D size{};
        auto intersect(clip_rect_t other) const -> clip_rect_t
        {
            const pos_2D begin{std::max(pos.x, other.pos.x), std::max(pos.y, other.pos.y)};
            return {begin, {std::max<std::int64_t>(0, std::min(pos.x + size.x, other.pos.x + other.size.x) - begin.x),
                            std::max<std::int64_t>(0, std::min(pos.y + size.y, other.pos.y + other.size.y) - begin.y)}};
        }
        auto contains(pos_2D point) const -> bool
        { return point.x >= pos.x && point.y >= pos.y && point.x < pos.x + size.x && point.y < pos.y + size.y; }
    };

    // Shared, world-coordinate clip chains keep nested viewport intersections
    // intact when children move, resize or create deferred glyphs.
    struct clip_region_t
    {
        clip_rect_t rect;
        std::shared_ptr<clip_region_t> parent;
        auto bounds() const -> clip_rect_t { return parent ? rect.intersect(parent->bounds()) : rect; }
        auto depth() const -> std::size_t { return parent ? parent->depth() + 1 : 1; }
    };

    class GUI_object_t
    {
    public:
        using status_t = std::uint8_t;
        using status_atomic_t = std::atomic_uint8_t;

        constexpr static status_t hidden{ 0 };
        constexpr static status_t shown{ 1 };
        constexpr static status_t hiding{ 2 };
        constexpr static status_t showing{ 3 };
    protected:
        mutable std::recursive_mutex m_mutex{};
        std::string m_name{};
    private:
        status_atomic_t m_status{};
        std::shared_ptr<clip_region_t> m_parent_clip;
        std::function<void(size_2D)> m_size_callback{};
    public:
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        GUI_object_t(std::source_location location = std::source_location::current());
        #else
        GUI_object_t() = default;
        #endif
        GUI_object_t(std::string name);
        GUI_object_t(const GUI_object_t& in);
        #ifdef DEBUG_GUI_OBJECT_LOG
        virtual ~GUI_object_t();
        #else
        virtual ~GUI_object_t() = default;
        #endif

        auto operator=(const GUI_object_t& in) -> GUI_object_t&;

        using child_visitor_t = std::function<void(GUI_object_t&)>;
        virtual auto visit_children(const child_visitor_t&) -> void {}
        auto parent_clip() const -> const std::shared_ptr<clip_region_t>& { return m_parent_clip; }
        auto clip_bounds(clip_rect_t rect) const -> clip_rect_t
        { return m_parent_clip ? rect.intersect(m_parent_clip->bounds()) : rect; }
        auto clip_contains(pos_2D point) const -> bool
        { return !m_parent_clip || m_parent_clip->bounds().contains(point); }
        auto set_parent_clip(std::shared_ptr<clip_region_t> clip) -> void
        {
            m_parent_clip = std::move(clip);
            clip_changed();
            propagate_clip();
        }
        auto refresh_clip() -> void
        {
            clip_changed();
            visit_children([](GUI_object_t& child) { child.refresh_clip(); });
        }
        auto name_get() -> std::string;
        auto name_set(std::string name) -> void;
        auto debug_attach(const void* parent, std::uint64_t id) const noexcept -> void;
        virtual auto get_pos() const -> pos_2D = 0;
        virtual auto set_pos(pos_2D pos) -> void = 0;
        virtual auto get_size() const -> size_2D = 0;
        virtual auto set_size(size_2D size) -> void;
        auto set_size_callback(std::function<void(size_2D)> callback) -> void;
        virtual auto get_margin() const -> size_1D = 0;
        virtual auto log(std::uint32_t tab_count) const -> void = 0;
    public:
        auto show() -> void;
        auto hide() -> void;
    protected:
        virtual auto clip_changed() -> void {}
        virtual auto propagate_clip() -> void
        { visit_children([this](GUI_object_t& child) { child.set_parent_clip(m_parent_clip); }); }
        virtual auto show_impl(bool base) -> void = 0;
        auto show_begin(bool base) -> bool;
        auto show_end(bool base) -> void;
        virtual auto hide_impl(bool base) -> void = 0;
        auto hide_begin(bool base) -> bool;
        auto hide_end(bool base) -> void;
        auto status() const -> status_t;
    private:
        auto debug_event(std::string_view event) const noexcept -> void;
        #if defined(DEBUG_GUI_OBJECT_NAME) || defined(DEBUG_GUI_OBJECT_LOG)
        std::uint64_t m_debug_id{};
        #endif
    };
}
