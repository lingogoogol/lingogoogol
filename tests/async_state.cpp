#include <nlohmann/json.hpp>
import std;
import storage.state;
using namespace std::chrono_literals;
int main()
{
    try
    {
        auto check=[](bool value,const char* text){if(!value)throw std::runtime_error{text};};
        const auto previous=std::filesystem::current_path();
        const auto scratch=std::filesystem::temp_directory_path()/("storage-async-state-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(scratch);std::filesystem::current_path(scratch);
        struct cleanup_t{std::filesystem::path previous,scratch;~cleanup_t(){std::filesystem::current_path(previous);std::error_code error;std::filesystem::remove_all(scratch,error);}} cleanup{previous,scratch};
        storage_gui_thread_set();
        storage_write_queue_t queue;
        std::atomic_bool started{};
        queue.push([&]{started=true;std::this_thread::sleep_for(300ms);});
        while(!started)std::this_thread::yield();
        const auto start=std::chrono::steady_clock::now();
        auto journal=task_journal_t::create({{"kind","upload-file"},{"path","fixture"},{"routing",{{"owners",nlohmann::json::object()},{"kinds",nlohmann::json::object()}}}});
        journal->set("user_paused",true);
        journal->progress("task",{{"progress",123}});
        check(std::chrono::steady_clock::now()-start<150ms,"GUI state calls waited for disk worker");
        check(!journal->ready(),"Durability gate opened before queued state was written");
        check(journal->field("user_paused")==nlohmann::json(true),"Pending pause was not visible in memory");
        journal->wait();
        check(journal->progress("task")->at("progress")==123,"Progress was not persisted");
        for(int i=0;i<200;++i)journal->set("counter",i);
        journal->wait();
        const auto path=journal->path();journal.reset();
        auto restored=std::async(std::launch::async,[]{return task_journal_t::restore();}).get();
        check(restored.size()==1 && restored.front()->field("counter")==nlohmann::json(199),"Replay lost a committed update");
        std::atomic_int attempts{},after{};
        storage_write_queue_t retry;
        retry.push([&]{if(++attempts==1)throw std::runtime_error{"injected disk failure"};});
        retry.push([&]{++after;});
        bool rejected{};try{retry.wait();}catch(const std::exception&){rejected=true;}
        check(rejected && after==0,"Writes passed a failed durable operation");
        retry.retry();retry.wait();check(attempts==2 && after==1,"Retry lost or duplicated queued state");
        restored.front()->finish();restored.front()->wait();restored.clear();
        check(nlohmann::json::parse(storage_read("task")).empty(),"Completion was acknowledged before index removal");
        std::cout<<"PASS: nonblocking GUI state writes, durability gates, pause/progress persistence, ordered disk-error retry, crash-safe journal compaction and terminal index removal.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
