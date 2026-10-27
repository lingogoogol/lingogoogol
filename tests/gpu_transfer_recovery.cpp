#include <nlohmann/json.hpp>
#include <vulkan/vk_platform.h>
import external.Vulkan;
import std;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;
import storage.kernel;
using namespace std::chrono_literals;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void inject_loss(lgo::engine_t& engine, bool buffer=false) {
    engine.flush();
    auto* dispatch=const_cast<std::remove_const_t<std::remove_pointer_t<decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
    if(buffer) {
        auto original=dispatch->vkCreateBuffer;
        dispatch->vkCreateBuffer=[](auto... args) { using result_t=decltype(original(args...));return static_cast<result_t>(-4); };
    } else {
        auto original=dispatch->vkWaitForFences;
        dispatch->vkWaitForFences=[](auto... args) { using result_t=decltype(original(args...));return static_cast<result_t>(-4); };
        engine.draw();
    }
}
int main() {
std::set_terminate([] {std::_Exit(42);});

try {
    lgo::init_logfile("logfile/test.log");
    lgo::FreeType_t ft{};
    lgo::font_face_t::freetype_set(&ft);
    lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf", static_cast<std::uint64_t>(lgo::std_font_size.x)};
    lgo::font_preference_t::default_get().push_back(&face);
    lgo::engine_t engine{lgo::size_2D{1536,1024}, "upload-regression"};
    struct font_cleanup_t {
        lgo::engine_t& engine;
        ~font_cleanup_t() {
            try { engine.flush(); } catch (...) {}
            lgo::text_line_t::free();
        }
    } cleanup{engine};
    lgo::depth_tracker_t depth{};
    lgo::state_t gui{&engine, &depth};
    load_state_t loads{};
    std::atomic_bool close{true}; // The fixture supplies IDs; never create cloud blobs.
    blob_queue_t queue{"blob_queue", &google_account_t::default_get(), &close};
    std::string pos{"root"};
    std::shared_ptr<lgo::divy_soft_t> column{};
    GUI_persistent_t persistent{};
    curl_multi_worker_t multi{};
    home(&engine,&gui,&persistent,&column,&loads,&multi,&queue,&pos);
    auto main=gui.get_object_shared<lgo::divy_hard_t>(0);
    auto work=main->get_object_shared<lgo::divx_soft_t>(3);
    auto viewer=work->get_object_shared<lgo::divy_soft_t>(3);
    check(viewer && viewer->len().x==0,"initial folder is not empty");
    int notifications=0;
    auto refresh=loads.m_on_upload_linked;
    loads.m_on_upload_linked=[&](const std::string& dest) { ++notifications; if(notifications==4) inject_loss(engine,true); refresh(dest); };
    auto add=[&](std::string path, bool folder) {
        auto t=std::make_shared<upload_task_t>();
        t->m_state=&loads; t->m_type="upload"; t->m_name=path; t->m_pos.m_main_id=pos;
        t->m_progress_div=column->add_object<lgo::text_line_scroll_std_t>(lgo::depth_range_t{0,0.5},path,lgo::std_blue_light,lgo::size_1D{256},false);
        t->m_progress_text=column->get_object_shared<lgo::text_line_scroll_std_t>(t->m_progress_div);
        if(folder) folder_upload(&queue,t.get(),path,&t->m_pos);
        else file_upload(&queue,t.get(),path,&t->m_pos);
        t->m_begin_progress=t->m_entity_arr.begin(); t->m_end_progress=t->m_entity_arr.begin();
        std::promise<void> ready; ready.set_value(); t->m_prepared=ready.get_future();
        t->m_self_i=loads.m_task_list.emplace(loads.m_task_list.end(),t);
        check(!t->m_linked.valid(),"published during preparation");
        return t;
    };
    auto empty=add("input/empty.bin",false);
    auto tree=add("input/tree",true);
    auto other=add("input/another.bin",false);
    // Finish an unrelated task while browsing another folder: refresh must keep this view.
    pos="other";
    auto deadline=std::chrono::steady_clock::now()+40s;
    bool upload_loss=false;
    while(!loads.m_task_list.empty()) {
        if(!upload_loss && !loads.m_end.empty()) {
            auto* identity=tree->m_progress_text.get();
            inject_loss(engine);
            check(!engine.gpu_available(),"upload loss was not injected");
            check(identity==tree->m_progress_text.get(),"upload progress object changed");
            upload_loss=true;
        }
        check(std::chrono::steady_clock::now()<deadline,"upload timed out");
        check(!message_loop(&engine,&persistent,&multi,&loads,&queue),"window closed unexpectedly");
        lgo::text_line_t::pending_load();
        std::this_thread::sleep_for(1ms);
    }
    check(notifications==3,"missing upload notification");
    check(pos=="other","refresh changed current folder");
    check(viewer->len().x==0,"upload refreshed an unrelated view");
    pos="root";
    auto download=std::make_shared<download_task_t>();
    download->m_state=&loads;download->m_type="download";download->m_name="downloaded.bin";
    download->m_progress_div=column->add_object<lgo::text_line_scroll_std_t>(lgo::depth_range_t{0,0.5},"download",lgo::std_white,lgo::size_1D{256},false);
    download->m_progress_text=column->get_object_shared<lgo::text_line_scroll_std_t>(download->m_progress_div);
    std::string file_id;
    for(auto& entity:tree->m_entity_arr) if(auto file=dynamic_cast<upload_file_t*>(entity.get());file && file->m_size) file_id=file->m_main_id;
    std::filesystem::remove("downloaded.bin");
    file_download(&google_easy_t::get(),download.get(),file_id,"downloaded.bin");
    download->m_idle_first=download->m_file_arr.begin();
    std::promise<void> ready_download;ready_download.set_value();download->m_prepared=ready_download.get_future();
    download->m_self_i=loads.m_task_list.emplace(loads.m_task_list.end(),download);
    bool download_loss=false;
    auto visible=add("input/visible.bin",false);
    while(!loads.m_task_list.empty()) {
        if(!download_loss && !loads.m_end.empty()) {
            inject_loss(engine);download_loss=true;
        }

        check(std::chrono::steady_clock::now()<deadline,"refresh timed out");
        message_loop(&engine,&persistent,&multi,&loads,&queue);
        lgo::text_line_t::pending_load();
        std::this_thread::sleep_for(1ms);
    }
    // Listing refresh now completes asynchronously after upload publication.
    while(!persistent.m_listings.empty() || !engine.gpu_available()) {
        check(std::chrono::steady_clock::now()<deadline,"asynchronous refresh timed out");
        message_loop(&engine,&persistent,&multi,&loads,&queue);
        lgo::text_line_t::pending_load();
    }
    check(upload_loss && download_loss,"active transfer loss was not exercised");
    check(download->m_progress==download->m_size && download->m_size!=0,"download progress did not finish");
    check(lgo::get_file_str("downloaded.bin")==lgo::get_file_str("input/tree/sub/payload.bin"),"recovered download content mismatch");
    check(loads.m_end.empty(),"GPU error interrupted completion bookkeeping");
    check(notifications==4,"visible upload did not refresh");
    check(pos=="root","visible refresh navigated away");
    auto label=viewer->get_object_shared<lgo::button_text_t<lgo::text_line_scroll_std_t>>(9);
    check(label && label->get_text().find("visible.bin")!=std::string::npos,"uploaded file missing from GUI list");
    engine.flush(); lgo::text_line_t::free(); gui.clear_state();
    std::println("PASS: active upload/download GPU recovery, unchanged tasks, exact downloaded bytes, publication and refresh");
    return 0;
} catch(const std::exception& e) { std::println("FAIL: {}",e.what()); return 1; }
}
