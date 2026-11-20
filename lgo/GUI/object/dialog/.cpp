module lgo.GUI.object.dialog;
import std;
import external.GLFW;
import lgo.math.vec;
import lgo.GUI.primitive;
import lgo.GUI.object.state;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.misc;
import lgo.GUI.object.rect;
import lgo.GUI.object.rect_border;
import lgo.GUI.object.text;
import lgo.GUI.object.text_input;
import lgo.GUI.object.button;
import lgo.GUI.object.std;
namespace lgo
{
    modal_dialog_t::modal_dialog_t(engine_t* engine, depth_tracker_t* depth, std::string title,
                                   std::vector<std::string> lines,
                                   std::vector<std::pair<std::string, std::string>> fields,
                                   std::string accept,
                                   std::vector<std::pair<std::string, bool>> checkboxes)
        : m_state{engine, depth}
    {
        m_height = 152 + 28 * lines.size() + 68 * fields.size() + 36 * checkboxes.size();
        m_width = std::min<std::int64_t>(720, std::max<std::int64_t>(0, engine->get_window_size().x - 16));
        const auto inner{m_width - 48};
        m_backdrop = m_state.add_object_shared<rect_t>(
            depth_range_t{.12f, .13f}, pos_2D{}, engine->get_window_size(), color_t{.008f, .008f, .008f});
        m_panel = m_state.add_object_shared<rect_t>(depth_range_t{.10f, .11f}, pos_2D{},
                                                    size_2D{m_width, m_height}, color_t{.025f, .025f, .025f});
        auto label = [&](std::string text, std::int64_t y, color_t color)
        {
            auto object{m_state.add_object_shared<text_line_std_t>(depth_range_t{.04f, .05f}, pos_2D{},
                                                                   std::move(text), color, size_1D{inner})};
            m_children.emplace_back(object, pos_2D{24, y});
            return object;
        };
        label(std::move(title), 16, std_white);
        std::int64_t y{52};
        for (auto& line : lines)
        {
            label(std::move(line), y, std_light_gray);
            y += 28;
        }
        for (auto& [name, value] : fields)
        {
            label(std::move(name), y, std_white);
            y += 28;
            auto frame{m_state.add_object_shared<rect_border_t>(
                pos_2D{}, size_2D{inner, 32}, color_t{.008f, .008f, .008f}, size_1D{1},
                color_t{.14f, .14f, .14f}, depth_range_t{.06f, .07f})};
            m_children.emplace_back(frame, pos_2D{24, y});
            auto input{m_state.add_object_shared<text_line_std_input_t>(
                depth_range_t{.02f, .03f}, pos_2D{}, value, m_state.get_focus(), std_white, size_1D{inner})};
            m_inputs.push_back(input);
            m_children.emplace_back(input, pos_2D{24, y});
            y += 40;
        }
        for (const auto& [name, checked] : checkboxes)
        {
            const auto index = m_checkboxes.size();
            auto button = m_state.add_object_shared<button_text_t<text_line_std_t>>(
                depth_range_t{.02f,.03f}, pos_2D{}, [this,index]
                {
                    auto& checkbox = m_checkboxes[index];
                    checkbox.checked = !checkbox.checked;
                    checkbox.button->set_text(std::string{checkbox.checked ? "[x] " : "[ ] "} + checkbox.label);
                }, std::string{checked ? "[x] " : "[ ] "} + name, std_white, size_1D{inner});
            button->set_tooltip("切換選項："+name);
            m_checkboxes.push_back({name,checked,button});
            m_children.emplace_back(button,pos_2D{24,y});
            y += 36;
        }
        m_error = label("", y, std_red_light);
        auto cancel{m_state.add_object_shared<button_classic_std_t>(
            depth_range_t{.02f, .03f}, pos_2D{}, [this] { m_decision = false; }, "取消")};
        auto confirm{m_state.add_object_shared<button_classic_std_t>(
            depth_range_t{.02f, .03f}, pos_2D{}, [this] { m_decision = true; }, accept)};
        m_children.emplace_back(cancel, pos_2D{m_width - 240, m_height - 48});
        m_children.emplace_back(confirm, pos_2D{m_width - 120, m_height - 48});
        resize(engine->get_window_size());
        // Set a decision only; the owner validates and closes after input dispatch.
        // Ignore key repeats so holding Enter cannot accept a subsequent dialog.
        m_key=engine->callback_key_add([this](int key,int action,int)
        {
            if(action==GLFW_PRESS && (key==GLFW_KEY_ENTER || key==GLFW_KEY_KP_ENTER))
                m_decision=true;
        });
        if (m_width < 720)
            m_wheel = engine->callback_mouse_scroll_add([this](size_2D delta)
            {
                m_scroll -= 32 * delta.y;
                resize(m_window);
            });
    }
    auto modal_dialog_t::add_action(std::string label, std::function<void()> callback) -> void
    {
        auto button = m_state.add_object_shared<button_classic_std_t>(
            depth_range_t{.02f,.03f}, pos_2D{}, std::move(callback), std::move(label));
        m_children.emplace_back(button,pos_2D{24,m_height-48});
        resize(m_state.get_engine()->get_window_size());
    }
    auto modal_dialog_t::resize(size_2D size) -> void
    {
        m_backdrop->set_size(size);
        m_window = size;
        // Only a narrowed panel scrolls; desktop dialogs keep their placement.
        const auto overflow{m_width < 720 ? std::max<std::int64_t>(0, m_height + 16 - size.y) : 0};
        m_scroll = std::clamp<std::int64_t>(m_scroll, 0, overflow);
        const pos_2D origin{std::max<std::int64_t>(0, (size.x - m_width) / 2),
                            overflow ? 8 - m_scroll : std::max<std::int64_t>(0, (size.y - m_height) / 2)};
        m_panel->set_pos(origin);
        for (auto& [object, offset] : m_children)
            object->set_pos(pos_2D{origin.x + offset.x, origin.y + offset.y});
    }
    auto modal_dialog_t::values() const -> std::vector<std::string>
    {
        std::vector<std::string> result;
        for (const auto& input : m_inputs)
            result.push_back(input->get_text());
        for (const auto& checkbox : m_checkboxes)
            result.push_back(checkbox.checked ? "true" : "false");
        return result;
    }
} // namespace lgo
