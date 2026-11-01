#include <winsock2.h>
#include <windows.h>
#include <nlohmann/json.hpp>
import std;
import storage.state;
import storage.kernel;
using nlohmann::json;
using namespace std::chrono_literals;

void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error{message};
}

int main(int argc, char** argv)
{
    try
    {
        check(argc == 2, "Expected test phase.");
        const std::string phase{argv[1]};
        if (phase == "sharing")
        {
            storage_atomic_write("checkpoint", "old");
            // Ordinary external readers often omit FILE_SHARE_DELETE. Windows
            // then refuses atomic replacement until that reader closes.
            auto reader=CreateFileW(L"checkpoint",GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(reader!=INVALID_HANDLE_VALUE,"Cannot open competing reader.");
            std::jthread release{[reader] { std::this_thread::sleep_for(150ms); CloseHandle(reader); }};
            storage_atomic_write("checkpoint", "new");
            release.join();
            check(storage_read("checkpoint")=="new","Replacement did not survive a short-lived reader.");
            storage_atomic_write("dirty/state",R"({"value":0})");
            task_journal_t journal{"dirty",{{"value",0}}};
            reader=CreateFileW(L"dirty/state",GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(reader!=INVALID_HANDLE_VALUE,"Cannot lock original checkpoint.");
            bool failed{};
            try { journal.set("value",1); }
            catch (const std::runtime_error& e) { failed=std::string{e.what()}.find("Windows error")!=std::string::npos; }
            CloseHandle(reader);
            check(failed && json::parse(storage_read("dirty/state")).at("value")==0,
                  "Permanent lock did not preserve the previous checkpoint and error code.");
            journal.set("value",1);
            check(json::parse(storage_read("dirty/state")).at("value")==1,
                  "Retry discarded a dirty value because its in-memory copy was unchanged.");
        }
        else if (phase == "resume")
        {
            std::filesystem::create_directory("input");
            constexpr int files=576;
            const std::string address="fixture@storage-ai-fixture.iam.gserviceaccount.com";
            auto saved=json{{"routing",{{"owners",json::object()},{"kinds",json::object()}}},
                            {"allocations",json::object()}};
            std::vector<std::string> names;
            for (int i{};i<60000;++i)
            {
                auto id=std::format("content-{:026}",i);
                saved["routing"]["owners"][id]=address;
                saved["routing"]["kinds"][id]="data";
            }
            auto reserve=[&](const std::string& key,const std::string& id,const std::string& type)
            {
                saved["allocations"][key]=json::array({id});
                saved["owner:"+key]=address;
                saved["routing"]["owners"][id]=address;
                saved["routing"]["kinds"][id]=type;
            };
            reserve("folder:input","folder-main","folder");
            for (int i{};i<files;++i)
            {
                const auto name=std::format("file-{:03}.txt",i);
                const auto path=std::filesystem::path{"input"}/name;
                storage_atomic_write(path,"x");
                names.push_back(name);
                reserve("file:"+path.string(),"file-main-"+std::to_string(i),"file");
                saved["source:"+path.string()]={{"size",1},
                    {"mtime",std::filesystem::last_write_time(path).time_since_epoch().count()}};
            }
            saved["children:input"]=names;
            storage_atomic_write("journal/state",saved.dump());
            const auto before=std::filesystem::last_write_time("journal/state");
            auto session=std::make_shared<storage_session_t>();
            session->restore_routing(saved.at("routing"));
            upload_task_t task;
            task.m_session=session;
            task.m_journal=std::make_shared<task_journal_t>("journal",std::move(saved));
            const auto start=std::chrono::steady_clock::now();
            // No accounts, queues or network: every reservation must replay.
            folder_upload(nullptr,&task,"input",nullptr);
            const auto elapsed=std::chrono::steady_clock::now()-start;
            check(task.m_entity_arr.size()==files+1 && task.m_size==files,"Preparation lost source files.");
            check(std::filesystem::last_write_time("journal/state")==before,
                  "Unchanged resume preparation rewrote its entire checkpoint.");
            check(elapsed<10s,"Large resume preparation is still too slow.");
            const auto changed=std::filesystem::path{"input"}/"file-000.txt";
            storage_atomic_write(changed,"changed");
            bool rejected{};
            try { file_upload(nullptr,&task,changed,nullptr); }
            catch (const std::runtime_error& e) { rejected=std::string{e.what()}.starts_with("Upload source changed"); }
            check(rejected,"Source change was silently accepted.");
            std::cout << "Preparation milliseconds: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << '\n';
        }
        else throw std::runtime_error{"Unknown test phase."};
        std::cout << "PASS: " << phase << '\n';
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
