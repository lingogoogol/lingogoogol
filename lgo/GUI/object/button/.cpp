module lgo.GUI.object.button;

import std;

import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.font;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect_border;
import lgo.GUI.object.text;
import lgo.GUI.object.click_area;
import lgo.GUI.object.std;

namespace lgo
{
    struct button_tooltip_t::impl_t
    {
        engine_t* engine;
        std::function<std::string(std::string_view)> describe;
        // Rendering primitives normally participate in hit testing. A private
        // tracker makes the tooltip transparent to all application input.
        depth_tracker_t depth;
        const void* owner{};
        std::function<std::optional<button_tooltip_content_t>()> content;
        engine_t::callback_frame_t* frame{};
        std::unique_ptr<rect_border_t> panel;
        std::vector<std::unique_ptr<text_line_std_t>> lines;
        std::string text;
        size_2D window{}, extent{};
        font_face_t* font{};
        std::optional<button_tooltip_content_t> visible;

        auto hide() -> void
        {
            if (panel) panel->hide();
            for (auto& line : lines) line->hide();
            visible.reset();
        }
        auto layout(const button_tooltip_content_t& value) -> void
        {
            const auto size = engine->get_window_size();
            auto* face = font_preference_t::default_get().at(0);
            if (text!=value.text || size.x!=window.x || font!=face || !panel)
            {
                hide();
                lines.clear();
                text = value.text;
                window = size;
                font = face;
                const auto width = std::max<std::int64_t>(1,std::min<std::int64_t>(640,size.x-32));
                std::vector<std::string> wrapped;
                std::string line;
                // Matches the text line's rounded running pen position.
                double used{};
                // Decode whole UTF-8 code points; never split a Chinese glyph
                // or a supplementary character when wrapping long labels.
                for (std::size_t i{};i<text.size();)
                {
                    const auto start = i;
                    const auto lead = static_cast<unsigned char>(text[i++]);
                    if (lead=='\n') { wrapped.push_back(std::move(line)); line.clear(); used=0; continue; }
                    auto count = lead<0x80 ? 0 : lead<0xe0 ? 1 : lead<0xf0 ? 2 : 3;
                    char32_t code = lead & (count==0 ? 0x7f : count==1 ? 0x1f : count==2 ? 0x0f : 0x07);
                    for (int c{};c<count && i<text.size();++c)
                        code = (code<<6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
                    const auto advance = font_char_t{code}.advance_exact_get();
                    if (!line.empty() && used+advance>width)
                    { wrapped.push_back(std::move(line)); line.clear(); used=0; }
                    line += text.substr(start,i-start);
                    used += advance;
                }
                if (!line.empty()) wrapped.push_back(std::move(line));
                extent = {16,12};
                for (auto& label : wrapped)
                {
                    auto row = std::make_unique<text_line_std_t>(engine,&depth,depth_range_t{.002f,.003f},
                        pos_2D{},std::move(label),std_white,size_1D{width});
                    const auto measured = row->get_text_size();
                    extent.x = std::max(extent.x,measured.x+16);
                    extent.y += measured.y;
                    lines.push_back(std::move(row));
                }
                panel = std::make_unique<rect_border_t>(engine,&depth,pos_2D{},extent,
                    color_t{.025f,.025f,.025f},size_1D{1},std_dark_gray,depth_range_t{.004f,.006f});
            }
            const auto x = std::clamp<std::int64_t>(value.pos.x,8,std::max<std::int64_t>(8,size.x-extent.x-8));
            auto y = value.pos.y+value.size.y+6;
            if (y+extent.y>size.y-8) y=std::max<std::int64_t>(8,value.pos.y-extent.y-6);
            const pos_2D at{x,y};
            if (!visible || visible->pos.x!=x || visible->pos.y!=y)
            {
                panel->set_pos(at);
                std::int64_t line_y{y+6};
                for (auto& line : lines)
                {
                    line->set_pos({x+8,line_y});
                    line_y += line->get_text_size().y;
                }
            }
            panel->show();
            for (auto& line : lines) line->show();
            visible = button_tooltip_content_t{text,at,extent};
        }
        auto update() -> void
        {
            const auto value = content ? content() : std::nullopt;
            if (value && !value->text.empty()) layout(*value);
            else { owner=nullptr; content={}; hide(); }
        }
    };

    button_tooltip_t::button_tooltip_t(engine_t* engine) : m_impl{std::make_unique<impl_t>()}
    {
        m_impl->engine = engine;
        m_impl->frame = engine->callback_frame_add([this](double){m_impl->update();});
    }
    button_tooltip_t::~button_tooltip_t()
    {
        m_impl->engine->callback_frame_remove(m_impl->frame);
        m_impl->hide();
    }
    auto button_tooltip_t::shared(engine_t* engine) -> std::shared_ptr<button_tooltip_t>
    {
        static std::map<engine_t*,std::weak_ptr<button_tooltip_t>> overlays;
        auto& slot = overlays[engine];
        auto result = slot.lock();
        if (!result) { result=std::shared_ptr<button_tooltip_t>{new button_tooltip_t{engine}}; slot=result; }
        return result;
    }
    auto button_tooltip_t::activate(const void* owner,
        std::function<std::optional<button_tooltip_content_t>()> content) -> void
    {
        m_impl->owner = owner;
        m_impl->content = std::move(content);
    }
    auto button_tooltip_t::dismiss(const void* owner) -> void
    {
        if (m_impl->owner!=owner) return;
        m_impl->owner = nullptr;
        m_impl->content = {};
        m_impl->hide();
    }
    auto button_tooltip_t::set_descriptions(std::function<std::string(std::string_view)> describe) -> void
    { m_impl->describe=std::move(describe); }
    auto button_tooltip_t::describe(std::string_view label) const -> std::string
    { return m_impl->describe ? m_impl->describe(label) : std::string{label}; }
    auto button_tooltip_t::snapshot() const -> std::optional<button_tooltip_content_t>
    { return m_impl->visible; }

    button_classic_impl_t::button_classic_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string display_text,
        size_2D size,
        color_t content_color,
        size_1D border_size,
        color_t border_color
    )
    :
        m_engine{ engine },
        m_rect
        {
            engine,
            depth_tracker,
            pos,
            size,
            content_color,
            border_size,
            border_color,
            depth_range_t{ (depth_range.near + depth_range.far) / 2, depth_range.far }
        },
        m_text
        {
            m_engine,
            depth_tracker,
            depth_range_t{ depth_range.near, (depth_range.near + depth_range.far) / 2 },
            m_rect.content_get_pos(),
            display_text,
            m_rect.content_get_size(),
            size_1D{ 0x10 },
            font_preference_t::default_get(),
            std_white,
            alignment_2D{ alignment_x::center, alignment_y::center }
        }
    {}

    auto button_classic_impl_t::get_pos() const -> pos_2D
    {
        return m_rect.get_pos();
    }

    auto button_classic_impl_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_rect.set_pos(pos);
        m_text.set_pos(m_rect.content_get_pos());
        return;
    }

    auto button_classic_impl_t::get_size() const -> size_2D
    {
        return m_rect.get_size();
    }

    auto button_classic_impl_t::get_margin() const -> size_1D
    {
        return size_1D{ 0 };
    }

    auto button_classic_impl_t::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: button_classic_impl_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "rect:\n");
        m_rect.log(tab_count + 1);
        log_file(tab(tab_count + 1) + "text:\n");
        m_text.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto button_classic_impl_t::depth_range_get() const -> depth_range_t
    {
        return m_rect.depth_range_get();
    }

    auto button_classic_impl_t::button_enabled(bool enabled) -> void
    {
        m_rect.content_set_color(std_black);
        m_rect.border_set_color(enabled ? std_white : std_dark_gray);
        m_text.set_color(enabled ? std_white : std_dark_gray);
    }

    auto button_classic_impl_t::button_move(pos_2D) -> void
    {
        m_rect.border_set_color(std_yellow);
        m_text.set_color(std_yellow);
        return;
    }

    auto button_classic_impl_t::button_leave() -> void
    {
        m_rect.border_set_color(std_white);
        m_rect.content_set_color(std_black);
        m_text.set_color(std_white);
        return;
    }

    auto button_classic_impl_t::button_left_click(pos_2D) -> void
    {
        m_rect.border_set_color(std_white);
        m_rect.content_set_color(std_white);
        m_text.set_color(std_black);
        return;
    }

    auto button_classic_impl_t::button_left_release(pos_2D) -> void
    {
        m_rect.border_set_color(std_yellow);
        m_rect.content_set_color(std_black);
        m_text.set_color(std_yellow);
        return;
    }

    auto button_classic_impl_t::show_impl(bool base) -> void
    {
        if (!show_begin(base)) { return; }
        std::unique_lock lock{ m_mutex };
        m_rect.show();
        m_text.show();
        show_end(base);
        return;
    }

    auto button_classic_impl_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_text.hide();
        m_rect.hide();
        hide_end(base);
        return;
    }

    size_2D button_classic_std_impl_t::size{ std_sideways.x, std_forward.x };
    color_t button_classic_std_impl_t::content_color{ std_black };
    size_1D button_classic_std_impl_t::border_size{ std_line_size };
    color_t button_classic_std_impl_t::border_color{ std_white };
    size_1D button_classic_std_impl_t::margin{ std_margin };

    button_classic_std_impl_t::button_classic_std_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string display_text
    )
    :
        button_classic_impl_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            display_text,
            size,
            content_color,
            size_1D{ 0x2 },
            border_color
        }
    {}

    auto button_classic_std_impl_t::get_margin() const -> size_1D
    {
        return margin;
    }
}
