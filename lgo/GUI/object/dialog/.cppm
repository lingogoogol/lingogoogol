export module lgo.GUI.object.dialog;
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

export namespace lgo
{
    // Owns focus and objects. Poll decision() and destroy on the next GUI turn,
    // never from a button callback that is still executing inside this dialog.
    class modal_dialog_t
    {
        state_t m_state;
        std::shared_ptr<rect_t> m_backdrop;
        std::shared_ptr<rect_t> m_panel;
        std::vector<std::pair<std::shared_ptr<GUI_object_t>, pos_2D>> m_children;
        std::vector<std::shared_ptr<text_line_std_input_t>> m_inputs;
        std::shared_ptr<text_line_std_t> m_error;
        std::optional<bool> m_decision;
        std::int64_t m_height{};

      public:
        modal_dialog_t(engine_t* engine, depth_tracker_t* depth, std::string title,
                       std::vector<std::string> lines,
                       std::vector<std::pair<std::string, std::string>> fields, std::string accept = "確定");
        ~modal_dialog_t()
        {
            m_state.clear_state();
        }
        auto resize(size_2D size) -> void;
        auto decision() -> std::optional<bool>
        {
            return std::exchange(m_decision, std::nullopt);
        }
        auto values() const -> std::vector<std::string>;
        auto error(std::string text) -> void
        {
            m_error->set_text(std::move(text));
        }
    };
} // namespace lgo
