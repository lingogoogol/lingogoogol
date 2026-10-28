export module lgo.GUI.object.scroll;

import std;
import external.GLFW;
import lgo.math.vec;
import lgo.math.op;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect;

export namespace lgo
{
    enum class scroll_axes_t { x, y, xy };

    // One dispatcher per engine makes routing independent of callback pointer
    // order. Clip-chain depth identifies nested viewports and text scrollers.
    class scroll_input_t
    {
        struct route_t
        {
            std::set<scroll_input_t*> nodes;
            scroll_input_t* captured{};
            engine_t::callback_mouse_scroll_t* wheel{};
            engine_t::callback_mouse_move_t* move{};
            engine_t::callback_mouse_button_t* button{};
        };
        inline static std::map<engine_t*, route_t> routes;
        engine_t* m_engine{};
        GUI_object_t* m_owner{};
        float m_depth{};
        std::function<bool(pos_2D)> m_hit, m_track;
        std::function<bool(size_2D)> m_wheel;
        std::function<void(pos_2D)> m_begin, m_move;
        std::function<void()> m_end;

        static auto candidates(engine_t* engine, pos_2D pos) -> std::vector<scroll_input_t*>
        {
            std::vector<scroll_input_t*> result;
            for (auto node : routes.at(engine).nodes)
                if (node->m_owner->clip_contains(pos) && node->m_hit(pos)) result.push_back(node);
            std::ranges::sort(result, [](auto a, auto b)
            {
                const auto level = [](auto node) { return node->m_owner->parent_clip() ? node->m_owner->parent_clip()->depth() : 0; };
                if (level(a) != level(b)) return level(a) > level(b);
                if (a->m_depth != b->m_depth) return a->m_depth < b->m_depth;
                const auto sa{a->m_owner->get_size()}, sb{b->m_owner->get_size()};
                return sa.x * sa.y < sb.x * sb.y;
            });
            return result;
        }
      public:
        scroll_input_t() = default;
        scroll_input_t(const scroll_input_t&) = delete;
        auto operator=(const scroll_input_t&) -> scroll_input_t& = delete;
        ~scroll_input_t() { detach(); }
        auto attach(engine_t* engine, GUI_object_t* owner, float depth,
                    std::function<bool(pos_2D)> hit, std::function<bool(size_2D)> wheel,
                    std::function<bool(pos_2D)> track, std::function<void(pos_2D)> begin,
                    std::function<void(pos_2D)> move, std::function<void()> end) -> void
        {
            detach();
            m_engine = engine; m_owner = owner; m_depth = depth;
            m_hit = std::move(hit); m_wheel = std::move(wheel); m_track = std::move(track);
            m_begin = std::move(begin); m_move = std::move(move); m_end = std::move(end);
            auto& route{routes[engine]};
            if (route.nodes.empty())
            {
                route.wheel = engine->callback_mouse_scroll_add([engine](size_2D delta)
                {
                    for (auto axis : {size_2D{delta.x,0}, size_2D{0,delta.y}})
                    {
                        if (!axis.x && !axis.y) continue;
                        for (auto node : candidates(engine, engine->cursor_pos_get()))
                            if (node->m_wheel(axis)) break;
                    }
                });
                route.move = engine->callback_mouse_move_add([engine](pos_2D pos)
                { if (auto node{routes.at(engine).captured}) node->m_move(pos); });
                route.button = engine->callback_mouse_button_add([engine](int button, int action, int)
                {
                    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
                    auto& active{routes.at(engine).captured};
                    if (action == GLFW_RELEASE)
                    {
                        if (active) active->m_end();
                        active = nullptr;
                        return;
                    }
                    if (action != GLFW_PRESS || active) return;
                    const auto pos{engine->cursor_pos_get()};
                    for (auto node : candidates(engine,pos))
                        if (node->m_track(pos)) { active = node; node->m_begin(pos); break; }
                });
            }
            route.nodes.insert(this);
        }
        auto detach() -> void
        {
            if (!m_engine) return;
            auto& route{routes.at(m_engine)};
            if (route.captured == this) { m_end(); route.captured = nullptr; }
            route.nodes.erase(this);
            if (route.nodes.empty())
            {
                m_engine->callback_mouse_scroll_remove(route.wheel);
                m_engine->callback_mouse_move_remove(route.move);
                m_engine->callback_mouse_button_remove(route.button);
                routes.erase(m_engine);
            }
            m_engine = nullptr;
        }
    };

    class scroll_t: public GUI_object_t
    {
        engine_t* m_engine;
        depth_tracker_t* m_tracker;
        depth_range_t m_depth;
        pos_2D m_pos;
        size_2D m_size;
        size_1D m_margin;
        scroll_axes_t m_axes;
        std::shared_ptr<GUI_object_t> m_content;
        std::shared_ptr<clip_region_t> m_view{std::make_shared<clip_region_t>()};
        pos_2D m_origin{};
        size_2D m_offset{}, m_content_size{};
        rect_t m_x_track, m_x_thumb, m_y_track, m_y_thumb;
        scroll_input_t m_input;
        std::uint64_t m_depth_id{};
        int m_drag_axis{-1};
        std::int64_t m_grab{};
        bool m_layout{};
        static constexpr std::int64_t bar_width{2}, bar_space{8}, wheel_step{32};
        auto x_enabled() const -> bool { return m_axes != scroll_axes_t::y; }
        auto y_enabled() const -> bool { return m_axes != scroll_axes_t::x; }
        auto hit(pos_2D pos) const -> bool
        { return clip_contains(pos) && inside(m_pos,m_size,pos) && m_tracker->top(pos,m_depth_id); }
        auto update_bars() -> void
        {
            const auto view{view_size()}, maximum{max_scroll()};
            auto bar = [&](bool x, rect_t& track, rect_t& thumb, bool enabled, std::int64_t limit,
                           std::int64_t extent, std::int64_t offset)
            {
                const auto length{x ? view.x : view.y};
                const auto thickness{std::min(bar_width, x ? m_size.y : m_size.x)};
                const auto thumb_length{limit ? std::clamp(length * length / std::max<std::int64_t>(1,extent),
                    std::min<std::int64_t>(24,length),length) : length};
                const auto travel{length - thumb_length};
                const auto at{limit ? offset * travel / limit : 0};
                track.set_pos(x ? pos_2D{m_pos.x,m_pos.y + m_size.y - thickness}
                                : pos_2D{m_pos.x + m_size.x - thickness,m_pos.y});
                track.set_size(x ? size_2D{length,thickness} : size_2D{thickness,length});
                thumb.set_pos(track.get_pos() + (x ? size_2D{at,0} : size_2D{0,at}));
                thumb.set_size(x ? size_2D{thumb_length,thickness} : size_2D{thickness,thumb_length});
                if (status() == shown && enabled && limit) { track.show(); thumb.show(); }
                else { thumb.hide(); track.hide(); }
            };
            bar(true,m_x_track,m_x_thumb,x_enabled(),maximum.x,m_origin.x + m_content_size.x,m_offset.x);
            bar(false,m_y_track,m_y_thumb,y_enabled(),maximum.y,m_origin.y + m_content_size.y,m_offset.y);
        }
        auto layout() -> void
        {
            if (m_layout) return;
            m_layout = true;
            struct reset_t { bool& active; ~reset_t() { active = false; } } reset{m_layout};
            const auto maximum{max_scroll()};
            m_offset = {std::clamp(m_offset.x,std::int64_t{},maximum.x),
                        std::clamp(m_offset.y,std::int64_t{},maximum.y)};
            m_view->rect = {m_pos,view_size()};
            m_view->parent = parent_clip();
            if (m_content)
            {
                m_content->set_pos(m_pos + size_2D{m_origin.x - m_offset.x,m_origin.y - m_offset.y});
                m_content->refresh_clip();
            }
            update_bars();
            if (status() == shown)
            {
                const auto bounds{clip_bounds({m_pos,m_size})};
                m_tracker->update(m_depth_id,bounds.pos,bounds.size);
            }
        }
        auto drag(pos_2D pos) -> void
        {
            if (m_drag_axis < 0) return;
            const bool x{m_drag_axis == 0};
            const auto& track{x ? m_x_track : m_y_track};
            const auto& thumb{x ? m_x_thumb : m_y_thumb};
            const auto travel{x ? track.get_size().x - thumb.get_size().x : track.get_size().y - thumb.get_size().y};
            if (travel <= 0) return;
            const auto at{std::clamp((x ? pos.x - track.get_pos().x : pos.y - track.get_pos().y) - m_grab,
                std::int64_t{},travel)};
            auto offset{m_offset};
            if (x) offset.x = at * max_scroll().x / travel;
            else offset.y = at * max_scroll().y / travel;
            set_scroll_offset(offset);
        }
      protected:
        auto clip_changed() -> void override { layout(); }
        auto propagate_clip() -> void override
        {
            m_view->parent = parent_clip();
            if (m_content) m_content->set_parent_clip(m_view);
            for (auto bar : {&m_x_track,&m_x_thumb,&m_y_track,&m_y_thumb}) bar->set_parent_clip(parent_clip());
        }
      public:
        scroll_t(engine_t* engine, depth_tracker_t* tracker, depth_range_t depth, pos_2D pos,
                 size_2D size, scroll_axes_t axes = scroll_axes_t::xy, size_1D margin = {8})
            : m_engine{engine},m_tracker{tracker},m_depth{depth},m_pos{pos},m_size{std::max<std::int64_t>(0,size.x),std::max<std::int64_t>(0,size.y)},m_margin{margin},m_axes{axes},
              m_x_track{engine,tracker,{(depth.near+depth.far)/2,depth.far},pos,{},color_t{.15f,.15f,.15f}},
              m_x_thumb{engine,tracker,depth,pos,{},color_t{.65f,.65f,.65f}},
              m_y_track{engine,tracker,{(depth.near+depth.far)/2,depth.far},pos,{},color_t{.15f,.15f,.15f}},
              m_y_thumb{engine,tracker,depth,pos,{},color_t{.65f,.65f,.65f}} { layout(); }
        ~scroll_t() override { hide(); set_content({}); }
        auto get_pos() const -> pos_2D override { return m_pos; }
        auto get_size() const -> size_2D override { return m_size; }
        auto get_margin() const -> size_1D override { return m_margin; }
        auto set_pos(pos_2D pos) -> void override { m_pos = pos; layout(); }
        auto set_size(size_2D size) -> void override
        { m_size = {std::max<std::int64_t>(0,size.x),std::max<std::int64_t>(0,size.y)}; layout(); GUI_object_t::set_size(m_size); }
        auto view_size() const -> size_2D
        { return {std::max<std::int64_t>(0,m_size.x - (y_enabled() ? bar_space : 0)),
                  std::max<std::int64_t>(0,m_size.y - (x_enabled() ? bar_space : 0))}; }
        auto max_scroll() const -> size_2D
        { const auto view{view_size()}; return {x_enabled() ? std::max<std::int64_t>(0,m_origin.x+m_content_size.x-view.x) : 0,
                                              y_enabled() ? std::max<std::int64_t>(0,m_origin.y+m_content_size.y-view.y) : 0}; }
        auto set_axes(scroll_axes_t axes) -> void { m_axes = axes; layout(); }
        auto axes() const -> scroll_axes_t { return m_axes; }
        auto set_content(std::shared_ptr<GUI_object_t> content, pos_2D origin = {}) -> void
        {
            if (content)
            {
                bool cycle{};
                std::function<void(GUI_object_t&)> inspect = [&](GUI_object_t& child)
                {
                    if (&child == this) { cycle = true; return; }
                    child.visit_children(inspect);
                };
                inspect(*content);
                if (cycle) throw std::invalid_argument{"Scroll content cannot contain its own viewport."};
            }
            if (m_content)
            { m_content->hide(); m_content->set_size_callback({}); m_content->set_parent_clip({}); }
            m_content = std::move(content); m_origin = origin;
            m_content_size = m_content ? m_content->get_size() : size_2D{};
            if (m_content)
            {
                m_content->set_parent_clip(m_view);
                m_content->set_size_callback([this](size_2D size) { m_content_size = size; layout(); });
            }
            layout();
            if (m_content && status() == shown) m_content->show();
        }
        auto content() const -> const std::shared_ptr<GUI_object_t>& { return m_content; }
        // Unscrolled origin relative to the viewport; independent of scroll offset.
        auto set_content_pos(pos_2D origin) -> void { m_origin = origin; layout(); }
        auto content_pos() const -> pos_2D { return {m_origin.x-m_offset.x,m_origin.y-m_offset.y}; }
        auto scroll_offset() const -> size_2D { return m_offset; }
        auto set_scroll_offset(size_2D offset) -> void
        {
            const auto maximum{max_scroll()};
            offset = {std::clamp(offset.x,std::int64_t{},maximum.x),std::clamp(offset.y,std::int64_t{},maximum.y)};
            if (offset.x == m_offset.x && offset.y == m_offset.y) return;
            m_offset = offset; layout();
        }
        auto scrollbar_pos(bool x) const -> pos_2D { return (x ? m_x_thumb : m_y_thumb).get_pos(); }
        auto scrollbar_size(bool x) const -> size_2D { return (x ? m_x_thumb : m_y_thumb).get_size(); }
        auto visit_children(const child_visitor_t& visit) -> void override
        { if (m_content) visit(*m_content); visit(m_x_track); visit(m_x_thumb); visit(m_y_track); visit(m_y_thumb); }
        auto log(std::uint32_t tab) const -> void override
        { GUI_object_t::log(tab); if (m_content) m_content->log(tab+1); }
        auto show_impl(bool base) -> void override
        {
            if (!show_begin(base)) return;
            const auto bounds{clip_bounds({m_pos,m_size})};
            m_depth_id = m_tracker->add(bounds.pos,bounds.size,m_depth.near,false);
            m_input.attach(m_engine,this,m_depth.near,[this](pos_2D pos) { return hit(pos); },
                [this](size_2D delta)
                {
                    const auto old{m_offset};
                    set_scroll_offset({m_offset.x-wheel_step*delta.x,m_offset.y-wheel_step*delta.y});
                    return old.x != m_offset.x || old.y != m_offset.y;
                }, [this](pos_2D pos)
                {
                    return (max_scroll().x && inside(m_x_track.get_pos(),m_x_track.get_size(),pos)) ||
                           (max_scroll().y && inside(m_y_track.get_pos(),m_y_track.get_size(),pos));
                }, [this](pos_2D pos)
                {
                    const bool x{max_scroll().x && inside(m_x_track.get_pos(),m_x_track.get_size(),pos)};
                    m_drag_axis = x ? 0 : 1;
                    const auto& thumb{x ? m_x_thumb : m_y_thumb};
                    m_grab = inside(thumb.get_pos(),thumb.get_size(),pos)
                        ? (x ? pos.x-thumb.get_pos().x : pos.y-thumb.get_pos().y)
                        : (x ? thumb.get_size().x : thumb.get_size().y)/2;
                    drag(pos);
                },[this](pos_2D pos) { drag(pos); },[this] { m_drag_axis = -1; });
            show_end(base); layout();
            if (m_content) m_content->show();
        }
        auto hide_impl(bool base) -> void override
        {
            if (!hide_begin(base)) return;
            m_input.detach();
            if (m_content) m_content->hide();
            m_x_thumb.hide();m_x_track.hide();m_y_thumb.hide();m_y_track.hide();
            m_tracker->remove(m_depth_id);
            hide_end(base);
        }
    };
}
