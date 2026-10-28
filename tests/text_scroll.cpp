#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
using namespace std::chrono_literals;

auto check(bool value, const char* message) -> void
{
    if (!value) throw std::runtime_error{message};
}

int main()
{
    try
    {
        lgo::init_logfile("logfile/scroll.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf", 16};
        lgo::font_preference_t::default_get().assign(1, &face);
        lgo::engine_t engine{{800, 600}, "Text scroll regression"};
        lgo::depth_tracker_t depth;
        {
            const lgo::depth_range_t range{.2f, .7f};
            const auto& font{lgo::font_preference_t::default_get()};
            std::string text;
            for (int i{}; i < 30; ++i) text += "中文長文字 ABC 0123456789 ";
            lgo::text_line_scroll_t line{&engine, &depth, range, {100,80}, text, {200,40},
                lgo::std_red, lgo::size_1D{0}, font, lgo::std_red,
                lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top}, false};
            lgo::text_line_t plain{&engine, &depth, range, {100,180}, text, {200,24},
                {0}, font, lgo::std_red,
                {lgo::alignment_x::left, lgo::alignment_y::top}};
            line.show(); plain.show();
            auto pump = [&]
            {
                check(!engine.message_loop(), "Scroll test window closed.");
                lgo::text_line_t::pending_load();
            };
            unsigned step{};
            auto input = [&](nlohmann::json commands, nlohmann::json screenshot = false)
            {
                for (int i{}; i < 8; ++i) pump();
                const auto request = nlohmann::json{{"step", ++step}, {"commands", commands}, {"screenshot", screenshot}};
                { std::ofstream out{"input.json"}; out << request.dump(); }
                const auto deadline{std::chrono::steady_clock::now() + 10s};
                while (true)
                {
                    pump();
                    if (std::filesystem::exists("ack"))
                    {
                        std::ifstream in{"ack"}; unsigned value{}; in >> value;
                        if (value == step) break;
                    }
                    check(std::chrono::steady_clock::now() < deadline, "Mouse driver stalled.");
                }
                for (int i{}; i < 8; ++i) pump();
            };
            input(nlohmann::json::array(), true);
            check(line.get_clip_size().x == 200 && line.get_clip_size().y == 38 && line.scrollbar_size().y == 2,
                  "Single line reserves a vertical scrollbar or lacks a horizontal track.");
            const auto thumb{line.scrollbar_pos()};
            input({{"move", thumb.x + 4, thumb.y + line.scrollbar_size().y / 2}, {"down"}, {"move", 700, 350}, {"up"}}, true);
            check(line.scroll_offset() > 0 && line.get_text_offset().y == 0, "Mouse drag did not scroll horizontally.");
            check(line.get_pos().x == 100 && line.get_clip_pos().x == 100, "Dragging moved the viewport.");
            const auto offset{line.scroll_offset()};
            input({{"move", 100, 114}});
            check(line.scroll_offset() == offset, "Scrollbar still dragged after mouse release.");
            input(nlohmann::json::array({{"wheel", 4}}));
            check(line.scroll_offset() == offset, "Vertical wheel moved the horizontal scrollbar.");
            input(nlohmann::json::array({{"wheel", 6}}));
            check(line.scroll_offset() < offset, "Horizontal wheel did not scroll horizontally.");
            const auto before_shift{line.scroll_offset()};
            input(nlohmann::json::array({{"keydown", "Shift_L"}, {"wheel",4}, {"keyup","Shift_L"}}));
            check(line.scroll_offset() == before_shift-32, "Shift+vertical wheel was not converted to horizontal scrolling.");
            input(nlohmann::json::array({{"keydown", "Shift_R"}, {"wheel",5}, {"keyup","Shift_R"}}));
            check(line.scroll_offset() == before_shift, "Right Shift+wheel did not scroll horizontally.");
            input({{"move", 103, 119}, {"click"}});
            check(line.scroll_offset() < offset / 2, "Track click did not reposition the thumb.");
            line.set_scroll_offset(1000000);
            line.set_size({300,40});
            check(line.scroll_offset() <= line.get_text_size().x - 300, "Resize did not clamp scrolling.");
            line.set_text("短文字", false);
            for (int i{}; i < 8; ++i) pump();
            check(line.scroll_offset() == 0 && line.scrollbar_size().x == 300, "Deferred text shrink did not reset scrolling.");
            line.set_text(text, false);
            lgo::text_line_t::reload_fonts();
            for (int i{}; i < 8; ++i) pump();
            check(line.scrollbar_size().x < 300, "Font reload did not update scrollbar geometry.");
            line.set_pos({100,80}); line.set_size({200,40}); line.set_scroll_offset(100);
            input(nlohmann::json::array(), true);
            // Hidden controls must unregister input and drop any drag capture.
            line.hide();
            const auto hidden_offset{line.scroll_offset()};
            input({{"move", 110,119}, {"wheel", 5}, {"down"}, {"move", 700,350}, {"up"}});
            check(line.scroll_offset() == hidden_offset, "Hidden text still handles mouse input.");
            line.show(); line.hide(); plain.hide();

        }
        engine.flush();
        lgo::text_line_t::free();
        std::println("PASS: actual mouse drag, capture outside the track, release, track clicks, horizontal wheel, resize, deferred text/font reload and hidden callback cleanup");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
