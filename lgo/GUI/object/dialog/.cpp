module lgo.GUI.object.dialog;
import std;
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
                                   std::string accept)
        : m_state{engine, depth}
    {
        m_height = 152 + 28 * lines.size() + 68 * fields.size();
        m_backdrop = m_state.add_object_shared<rect_t>(
            depth_range_t{.12f, .13f}, pos_2D{}, engine->get_window_size(), color_t{.008f, .008f, .008f});
        m_panel = m_state.add_object_shared<rect_t>(depth_range_t{.10f, .11f}, pos_2D{},
                                                    size_2D{720, m_height}, color_t{.025f, .025f, .025f});
        auto label = [&](std::string text, std::int64_t y, color_t color)
        {
            auto object{m_state.add_object_shared<text_line_std_t>(depth_range_t{.04f, .05f}, pos_2D{},
                                                                   std::move(text), color, size_1D{672})};
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
                pos_2D{}, size_2D{672, 32}, color_t{.008f, .008f, .008f}, size_1D{1},
                color_t{.14f, .14f, .14f}, depth_range_t{.06f, .07f})};
            m_children.emplace_back(frame, pos_2D{24, y});
            auto input{m_state.add_object_shared<text_line_std_input_t>(
                depth_range_t{.02f, .03f}, pos_2D{}, value, m_state.get_focus(), std_white, size_1D{672})};
            m_inputs.push_back(input);
            m_children.emplace_back(input, pos_2D{24, y});
            y += 40;
        }
        m_error = label("", y, std_red_light);
        auto cancel{m_state.add_object_shared<button_classic_std_t>(
            depth_range_t{.02f, .03f}, pos_2D{}, [this] { m_decision = false; }, "取消")};
        auto confirm{m_state.add_object_shared<button_classic_std_t>(
            depth_range_t{.02f, .03f}, pos_2D{}, [this] { m_decision = true; }, accept)};
        m_children.emplace_back(cancel, pos_2D{480, m_height - 48});
        m_children.emplace_back(confirm, pos_2D{600, m_height - 48});
        resize(engine->get_window_size());
    }
    auto modal_dialog_t::resize(size_2D size) -> void
    {
        m_backdrop->set_size(size);
        const pos_2D origin{std::max<std::int64_t>(0, (size.x - 720) / 2),
                            std::max<std::int64_t>(0, (size.y - m_height) / 2)};
        m_panel->set_pos(origin);
        for (auto& [object, offset] : m_children)
            object->set_pos(pos_2D{origin.x + offset.x, origin.y + offset.y});
    }
    auto modal_dialog_t::values() const -> std::vector<std::string>
    {
        std::vector<std::string> result;
        for (const auto& input : m_inputs)
            result.push_back(input->get_text());
        return result;
    }
} // namespace lgo
