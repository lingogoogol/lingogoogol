#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import external.GLFW;
using namespace std::chrono_literals;
using nlohmann::json;

auto check(bool value, const char* message) -> void
{ if (!value) throw std::runtime_error{message}; }

auto check_motion() -> void
{
    const lgo::motion_limits_t limits{12000.,1600.};
    for (const auto dt : {.001, .008, .016, .033, .05})
        for (const auto distance : {1.,32.,1000.,5000.})
        {
            lgo::motion_axis_t motion;
            motion.target = distance;
            bool braking{};
            unsigned step{};
            while (motion.active() && step++ < 10000)
            {
                const auto position{motion.position}, velocity{motion.velocity};
                motion.advance(dt,limits);
                check(motion.position >= position-1e-6 && motion.position <= distance+1e-6,
                      "Motion overshot its target.");
                check(std::abs(motion.velocity) <= limits.speed_max+1e-6 &&
                      std::abs(motion.velocity-velocity) <= limits.acceleration_max*dt+1e-5,
                      "Motion exceeded speed or acceleration limits.");
                if (motion.velocity < velocity) braking = true;
            }
            check(!motion.active() && motion.position == distance && (braking || dt >= 2.*std::sqrt(distance/limits.acceleration_max)),
                  "Motion did not brake and stop exactly at its target.");
        }
    lgo::motion_axis_t reverse;
    reverse.target=1000.;
    for (int i{}; i<10; ++i) reverse.advance(.016,limits);
    reverse.target=-120.;
    for (int i{}; i<1000 && reverse.active(); ++i) reverse.advance(.016,limits);
    check(!reverse.active() && reverse.position==-120.,"Retargeting did not settle.");
}

int main()
{
    try
    {
        check_motion();
        lgo::init_logfile("logfile/scroll.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{{960,720},"General scroll regression"};
        lgo::depth_tracker_t depth;
        {
            const lgo::depth_range_t range{.2f,.7f}, background{.6f,.7f};
            const lgo::alignment_2D alignment{lgo::alignment_x::left,lgo::alignment_y::top};
            auto body = [&](lgo::size_2D size)
            { return std::make_shared<lgo::divy_soft_t>(&engine,&depth,range,lgo::pos_2D{},size,lgo::size_1D{0},alignment); };
            auto paint = [&](auto contents, lgo::size_2D size, lgo::color_t color)
            { return contents->template add_object_ghost_shared<lgo::rect_t>(background,size,color); };
            // A viewport participates in div layout just like any other child.
            // Its size notification must expose the old size to the parent.
            {
                auto parent{body({400,300})};
                auto scroll{parent->add_object_shared<lgo::scroll_t>(range,
                    lgo::size_2D{200,100}, lgo::scroll_axes_t::y, lgo::size_1D{0})};
                auto following{parent->add_object_shared<lgo::rect_t>(background,
                    lgo::size_2D{100,20}, lgo::std_blue)};
                const auto length{parent->len().x}, below{following->get_pos().y};
                scroll->set_size({220,160});
                check(parent->len().x==length+60 && following->get_pos().y==below+60,
                      "Resizing a viewport did not update its parent's occupied length and following child.");
                scroll->set_size({200,100});
                check(parent->len().x==length && following->get_pos().y==below,
                      "Restoring a viewport's size left layout drift.");
            }
            auto pump = [&]
            {
                check(!engine.message_loop(),"Window unexpectedly closed.");
                lgo::text_line_t::pending_load();
            };
            std::vector<lgo::scroll_t*> scrollers;
            std::vector<lgo::text_line_scroll_std_t*> text_scrollers;
            unsigned step{};
            auto input = [&](json commands, json clips = json::array())
            {
                for (int i{};i<3;++i) pump();
                { std::ofstream file{"input.json"}; file << json{{"step",++step},{"commands",commands},{"clips",clips}}; }
                const auto deadline{std::chrono::steady_clock::now()+10s};
                while (true)
                {
                    pump();
                    if (std::filesystem::exists("ack"))
                    { std::ifstream file{"ack"}; unsigned value{}; file>>value; if (value==step) break; }
                    check(std::chrono::steady_clock::now()<deadline,"Input driver stalled.");
                }
                while (std::ranges::any_of(scrollers, [](auto scroll) { return scroll->scrolling(); }) ||
                       std::ranges::any_of(text_scrollers, [](auto scroll) { return scroll->scrolling(); }))
                {
                    check(std::chrono::steady_clock::now()<deadline,"Scroll animation did not settle.");
                    pump();
                }
                for (int i{};i<3;++i) pump();
            };
            auto clip = [](int channel,lgo::pos_2D pos,lgo::size_2D size)
            { return json{{"channel",channel},{"box",{pos.x,pos.y,pos.x+size.x,pos.y+size.y}}}; };
            lgo::scroll_t viewport{&engine,&depth,range,{100,80},{240,160},lgo::scroll_axes_t::x};
            scrollers.push_back(&viewport);
            auto contents{body({600,500})};
            paint(contents,{600,500},lgo::std_red_light);
            viewport.set_content(contents,{10,15}); viewport.show();
            input(json::array(),{clip(0,{100,80},viewport.view_size())});
            check(contents->get_pos().x==110 && contents->get_pos().y==95,"Content origin is not relative to the viewport.");
            viewport.motion_limits_set({400.,100.});
            bool intermediate{};
            auto frame{engine.callback_frame_add([&](double)
                { if (viewport.scroll_offset().x>0 && viewport.scroll_offset().x<32) intermediate=true; })};
            input({{"move",150,120},{"wheel",7}});
            engine.callback_frame_remove(frame);
            check(intermediate,"Wheel scrolling jumped straight to its destination.");
            const auto arrival{std::chrono::steady_clock::now()+5s};
            auto previous{viewport.scroll_offset().x};
            while (viewport.scrolling())
            {
                check(std::chrono::steady_clock::now()<arrival,"Wheel animation never settled.");
                pump();
                check(viewport.scroll_offset().x>=previous && viewport.scroll_offset().x<=32,
                      "Wheel animation reversed or overshot.");
                previous=viewport.scroll_offset().x;
            }
            check(viewport.scroll_offset().x==32,"Wheel animation stopped at the wrong pixel.");
            viewport.motion_limits_set({});
            viewport.set_scroll_offset({80,50});
            check(viewport.scroll_offset().x==80 && viewport.scroll_offset().y==0,"X-only mode moved vertically.");
            input({{"move",150,120},{"wheel",5}});
            check(viewport.scroll_offset().x==80,"Vertical wheel moved an X-only viewport.");
            input(json::array({{"wheel",7}}));
            check(viewport.scroll_offset().x==112,"Native horizontal wheel did not move X.");
            input(nlohmann::json::array({{"keydown","Shift_L"},{"wheel",4},{"keyup","Shift_L"}}));
            check(viewport.scroll_offset().x==80,"Shift+wheel did not map to X exactly once.");
            const auto x_thumb{viewport.scrollbar_pos(true)};
            input({{"move",x_thumb.x+2,x_thumb.y},{"down"},{"move",900,600},{"up"}});
            check(viewport.scroll_offset().x==viewport.max_scroll().x,"X thumb did not capture an outside drag.");
            const auto end_x{viewport.scroll_offset().x};
            input({{"move",110,90}});
            check(viewport.scroll_offset().x==end_x,"X drag did not stop on release.");
            viewport.set_size({300,190});
            check(viewport.scroll_offset().x==viewport.max_scroll().x && viewport.max_scroll().x==310,"Resize did not clamp X.");
            input(json::array(),{clip(0,viewport.get_pos(),viewport.view_size())});
            viewport.hide();

            viewport.set_axes(lgo::scroll_axes_t::y); viewport.set_size({240,160});
            viewport.set_content_pos({}); viewport.set_scroll_offset({});
            int clicks{};
            auto button{contents->add_object_ghost_shared<lgo::button_classic_std_t>(range,[&]{++clicks;},"裁切測試")};
            button->set_pos(contents->get_pos()+lgo::size_2D{10,145});
            viewport.show();
            input({{"move",120,230},{"click"}});
            check(clicks==1,"Visible portion of a clipped button is not clickable.");
            input({{"move",120,250},{"click"},{"wheel",5}});
            check(clicks==1 && viewport.scroll_offset().y==0,"Clipped button or viewport received outside input.");
            input({{"move",120,120},{"wheel",5}});
            check(viewport.scroll_offset().y==32 && viewport.scroll_offset().x==0,"Y-only wheel routing failed.");
            input(json::array({{"wheel",7}}));
            check(viewport.scroll_offset().y==32,"Horizontal wheel moved a Y-only viewport.");
            lgo::rect_t modal{&engine,&depth,{.01f,.05f},{90,70},{300,220},lgo::std_black};
            modal.show(); input(json::array({{"wheel",5}}));
            check(viewport.scroll_offset().y==32,"A modal did not block scrolling behind it.");
            modal.hide(); viewport.hide();

            // A nested XY viewport lives inside an ordinary fixed-size div.
            lgo::scroll_t outer{&engine,&depth,range,{80,70},{320,260},lgo::scroll_axes_t::xy};
            scrollers.push_back(&outer);
            auto outer_body{body({700,650})}; paint(outer_body,{700,650},lgo::std_red_light);
            auto inner{outer_body->add_object_ghost_shared<lgo::scroll_t>(lgo::depth_range_t{.15f,.6f},
                lgo::size_2D{160,120},lgo::scroll_axes_t::xy)};
            scrollers.push_back(inner.get());
            inner->set_pos({40,40});
            auto inner_body{body({400,300})};
            inner_body->add_object_ghost<lgo::rect_t>(lgo::depth_range_t{.4f,.5f},lgo::size_2D{400,300},lgo::std_blue);
            inner->set_content(inner_body);
            outer.set_content(outer_body); outer.show();
            bool rejected{};
            try { inner->set_content(outer_body); }
            catch (const std::invalid_argument&) { rejected = true; }
            check(rejected && inner->content()==inner_body,"Cyclic nested content was accepted or changed the old contents.");
            check(inner->get_pos().x==120 && inner->get_pos().y==110,"Nested viewport did not move with its div.");
            input(json::array(),{clip(0,outer.get_pos(),outer.view_size()),clip(2,inner->get_pos(),inner->view_size())});
            input({{"move",140,130},{"wheel",5}});
            check(inner->scroll_offset().y==32 && outer.scroll_offset().y==0,"Nested Y wheel also moved the parent.");
            input(json::array({{"wheel",7}}));
            check(inner->scroll_offset().x==32 && outer.scroll_offset().x==0,"Nested X wheel also moved the parent.");
            input(nlohmann::json::array({{"keydown","Shift_R"},{"wheel",5},{"keyup","Shift_R"}}));
            check(inner->scroll_offset().x==64 && inner->scroll_offset().y==32 && outer.scroll_offset().x==0,
                  "Nested Shift+wheel affected the wrong axis or parent.");
            const auto y_thumb{inner->scrollbar_pos(false)};
            input({{"move",y_thumb.x,y_thumb.y+2},{"down"},{"move",900,650},{"up"}});
            check(inner->scroll_offset().y==inner->max_scroll().y && outer.scroll_offset().y==0,
                  "Dragging a child thumb also dragged the parent.");
            input({{"move",140,130},{"wheel",5}});
            check(outer.scroll_offset().y==32 && inner->scroll_offset().y==inner->max_scroll().y,
                  "Wheel at the inner edge did not bubble once to the parent.");
            inner->set_scroll_offset({}); outer.set_scroll_offset({0,90});
            input(json::array(),{clip(0,outer.get_pos(),outer.view_size()),
                clip(2,{120,70},{152,62})});
            const auto saved{inner->scroll_offset()};
            input({{"move",279,35},{"down"},{"move",279,110},{"up"}});
            check(inner->scroll_offset().x==saved.x && inner->scroll_offset().y==saved.y,
                  "An ancestor-clipped child scrollbar captured a drag.");
            // Deferred glyphs added after both viewport clips already exist.
            std::string long_text;
            for (int i{};i<20;++i) long_text+="巢狀裁切 ABC ";
            auto text{inner_body->add_object_ghost_shared<lgo::text_line_scroll_std_t>(range,long_text,lgo::std_green,lgo::size_1D{400},false)};
            text_scrollers.push_back(text.get());
            text->set_pos(inner_body->get_pos()+lgo::size_2D{0,60});
            input(json::array(),{clip(1,{120,70},{152,62}),clip(2,{120,70},{152,62}),clip(0,outer.get_pos(),outer.view_size())});
            input({{"move",150,90},{"wheel",7}});
            check(text->scroll_offset()>0 && inner->scroll_offset().x==0 && outer.scroll_offset().x==0,
                  "Nested horizontal text and parent scrolled together.");
            outer.set_size({140,105});
            input(json::array(),{clip(0,outer.get_pos(),outer.view_size()),clip(2,{120,70},{92,62}),clip(1,{120,70},{92,62})});
            inner_body->set_size({100,90});
            inner->set_scroll_offset({900,900});
            check(inner->scroll_offset().x==0 && inner->scroll_offset().y==0,"Content shrink did not clamp both axes.");
            const auto parent_offset{outer.scroll_offset()};
            outer.hide();
            input({{"move",140,90},{"wheel",5},{"wheel",7}});
            check(outer.scroll_offset().x==parent_offset.x && outer.scroll_offset().y==parent_offset.y,
                  "Hidden nested viewports kept input registrations.");
            outer.show(); outer.hide();
            auto replacement{body({10,10})};
            viewport.set_content(replacement);
            check(!contents->parent_clip(),"Detached contents retained the old viewport's clip chain.");
            contents->set_size({999,999});
            check(viewport.max_scroll().x==0 && viewport.max_scroll().y==0,"Detached content retained a size callback.");
            {
                lgo::divy_soft_t animated{&engine,&depth,range,{400,80},{300,350},{0},
                    {lgo::alignment_x::left,lgo::alignment_y::bottom},true};
                const auto first{animated.add_object<lgo::rect_t>(background,lgo::size_2D{100,40},lgo::std_blue)};
                auto second{animated.add_object_shared<lgo::rect_t>(background,lgo::size_2D{100,40},lgo::std_blue)};
                animated.add_object<lgo::rect_t>(background,lgo::size_2D{100,40},lgo::std_blue);
                animated.show();
                check(!animated.smooth_children(),"Div animation is not opt-in.");
                animated.smooth_children_set(true,{400.,100.});
                const auto initial{second->get_pos()};
                const auto length{animated.len()};
                animated.remove_object(first);
                const auto shift{length.x-animated.len().x};
                animated.set_size({300,390});
                check(animated.children_moving() && second->get_pos().y==initial.y,
                      "Consecutive layout changes jumped instead of preserving the current position.");
                animated.set_pos({420,100});
                check(second->get_pos().y==initial.y+20,"Ancestor translation was eased a second time.");
                animated.smooth_children_set(false);
                check(!animated.children_moving() && second->get_pos().y==initial.y+shift+60,
                      "Disabling smoothing did not finish the accumulated layout target.");
                animated.smooth_children_set(true);
                const auto settled{second->get_pos().y};
                animated.set_size({300,430});
                animated.hide();
                animated.set_size({300,450});
                animated.show();
                pump();
                check(!animated.children_moving() && second->get_pos().y==settled+60,
                      "Hidden layout changes resumed an obsolete animation.");
                animated.hide();
            }
        }
        engine.flush(); lgo::text_line_t::free();
        std::println("PASS: X/Y/XY viewports, content origins, clipping, partial button hits, native/Shift wheels, nested routing/edge bubbling, drag capture, resize, deferred glyphs and cleanup");
        return 0;
    }
    catch (const std::exception& error) { std::println("FAIL: {}",error.what()); return 1; }
}
