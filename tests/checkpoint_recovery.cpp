#include <winsock2.h>
#include <windows.h>
#include <nlohmann/json.hpp>
import std;
import storage.state;
import storage.google;
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
            const auto initial=storage_read("dirty/state");
            reader=CreateFileW(L"dirty/00000000000000000000.record.tmp",GENERIC_READ,FILE_SHARE_READ,
                               nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(reader!=INVALID_HANDLE_VALUE,"Cannot lock next record.");
            bool failed{};
            try { journal.set("value",1); }
            catch (const std::runtime_error&) { failed=true; }
            CloseHandle(reader);
            check(failed && storage_read("dirty/state")==initial,
                  "Record failure changed the initial snapshot.");
            journal.set("value",1);
            task_journal_t restored{"dirty",json::parse(initial)};
            check(restored.field("value")==json(1),"Retry discarded an unchanged dirty field.");
            reader=CreateFileW(L"dirty/00000000000000000001.record.tmp",GENERIC_READ,FILE_SHARE_READ,
                               nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(reader!=INVALID_HANDLE_VALUE,"Cannot lock entity record.");
            int reservations{};
            auto reserve=[&](std::size_t count)
            {
                ++reservations;
                check(count==1,"Preparation reserved more than one entity.");
                return std::vector<std::string>{"id-a"};
            };
            const auto fields=json{{"source:a",{{"size",1},{"mtime",2}}},{"owner:file:a","fixture"}};
            failed=false;
            try { journal.allocate("file:a",1,reserve,fields,"fixture","file"); }
            catch (const std::runtime_error&) { failed=true; }
            CloseHandle(reader);
            check(failed,"Locked entity record did not report failure.");
            check(journal.allocate("file:a",1,reserve,fields,"fixture","file")==std::vector<std::string>{"id-a"} && reservations==1,
                  "Entity flush retry allocated a replacement ID.");
            task_journal_t replay{"dirty",json::parse(storage_read("dirty/state"))};
            const auto entity=replay.data();
            check(entity.at("allocations").at("file:a").at(0)=="id-a" && entity.at("source:a").at("size")==1 &&
                  entity.at("routing").at("owners").at("id-a")=="fixture","Entity replay lost metadata or ownership.");
            check(replay.allocate("file:a",1,reserve).front()=="id-a" && reservations==1,"Replay consumed another ID.");
            check(storage_read("dirty/state")==initial,"An entity update rewrote the initial snapshot.");
        }
        else if (phase == "queue-crash")
        {
            storage_atomic_write("tail-queue",json::array({"quoted-\"-\\", "中文", "last"}).dump());
            std::atomic_bool close{true};
            google_account_t account;
            blob_queue_t queue{"tail-queue",&account,&close};
            check(queue.get(1).front()=="last","Queue did not reserve from its tail.");
            queue.stop();
            const auto before=storage_read("tail-queue");
            // Simulate termination after journaling an append and writing only
            // its comma. Recovery must repair the malformed live JSON first.
            const auto offset=before.size()-1;
            storage_atomic_write("tail-queue.tail",json{{"offset",offset},{"tail",",\"recovered\"]"}}.dump());
            storage_rewrite_tail("tail-queue",offset,",");
            std::_Exit(0);
        }
        else if (phase == "queue-resume")
        {
            std::atomic_bool close{true};
            google_account_t account;
            {
                blob_queue_t queue{"tail-queue",&account,&close};
                check(queue.get(1).front()=="recovered","Interrupted queue append did not recover.");
                // A journal commit followed by a locked live file must not
                // hand out an ID whose removal has not been flushed.
                auto reader=CreateFileW(L"tail-queue",GENERIC_READ,FILE_SHARE_READ,
                                        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                check(reader!=INVALID_HANDLE_VALUE,"Cannot lock live queue.");
                bool failed{};
                try { queue.get(1); } catch (const std::runtime_error&) { failed=true; }
                CloseHandle(reader);
                check(failed,"Queue handed out an ID after a failed live-file write.");
                check(queue.get(1).front()=="quoted-\"-\\","Queue retried an ID removed by its durable edit.");
                check(json::parse(storage_read("tail-queue")).empty(),"Queue did not truncate to an empty array.");
                queue.stop();
            }
            {
                blob_queue_t queue{"tail-queue",&account,&close};
                check(queue.count()==0,"Empty queue recovery restored consumed IDs.");
                queue.stop();
            }
        }
        else if (phase == "fresh" || phase.starts_with("scale-"))
        {
            const std::size_t files=phase=="fresh" ? 1945 : std::stoul(phase.substr(6));
            constexpr std::size_t reserve_count=30000;
            const std::string address="fixture@storage-ai-fixture.iam.gserviceaccount.com";
            std::filesystem::remove_all("fresh-input");
            std::filesystem::remove_all("fresh-journal");
            std::filesystem::remove("fresh-queue.tail");
            std::filesystem::create_directories("fresh-input/empty-folder");
            for (std::size_t i{};i<files;++i)
                std::ofstream{std::filesystem::path{"fresh-input"}/std::format("file-{:04}.txt",i)} << "x";
            auto saved=json{{"routing",{{"owners",json::object()},{"kinds",json::object()}}}};
            // A preceding upload may have populated the shared session's
            // routing table before the new task's initial snapshot is taken.
            for (int i{};i<60000;++i)
            {
                const auto id=std::format("previous-content-{:026}",i);
                saved["routing"]["owners"][id]=address;
                saved["routing"]["kinds"][id]="data";
            }
            auto initial=json::array();
            for (std::size_t i{};i<reserve_count;++i) initial.push_back("main-"+std::to_string(i));
            storage_atomic_write("fresh-queue",initial.dump());
            std::atomic_bool close{};
            auto account=std::make_shared<google_account_t>();
            auto queue=std::make_shared<blob_queue_t>("fresh-queue",account.get(),&close);
            auto session=std::make_shared<storage_session_t>();
            session->accounts[address]=account;
            session->queues[address]=queue;
            session->restore_routing(saved.at("routing"));
            storage_atomic_write("fresh-journal/state",saved.dump());
            auto journal=std::make_shared<task_journal_t>("fresh-journal",std::move(saved));
            // Simulate another transfer consuming the same reserve while the
            // new folder prepares. All IDs must be distinct and durable.
            auto previous=std::async(std::launch::async,[&]
            {
                std::vector<std::string> ids;
                for (int i{};i<32;++i) ids.push_back(queue->get(1).front());
                return ids;
            });
            upload_task_t task;
            task.m_session=session; task.m_journal=journal;
            const auto start=std::chrono::steady_clock::now();
            folder_upload(queue.get(),&task,"fresh-input",nullptr);
            const auto elapsed=std::chrono::steady_clock::now()-start;
            const auto other=previous.get();
            queue->stop();
            check(task.m_entity_arr.size()==files+2 && task.m_size==files,"Fresh preparation lost entries.");
            const auto initial_state=storage_read("fresh-journal/state");
            task_journal_t reloaded{"fresh-journal",json::parse(initial_state)};
            const auto durable=reloaded.data();
            std::uint64_t record_bytes{};
            std::size_t record_count{};
            for (const auto& entry : std::filesystem::directory_iterator{"fresh-journal"})
                if (entry.path().extension()==".record")
                { ++record_count; record_bytes+=entry.file_size(); }
            check(record_count==files+2,"Preparation did not save one record per entity.");
            check(record_bytes<files*2048,"New entities rewrote the inherited routing table.");
            std::set<std::string> allocated(other.begin(),other.end());
            for (const auto& entity : task.m_entity_arr)
            {
                const auto key=entity->m_type+":"+entity->m_path_local;
                const auto id=durable.at("allocations").at(key).at(0).get<std::string>();
                check(allocated.insert(id).second,"Concurrent jobs reused an ID.");
                check(durable.at("owner:"+key)==address && durable.at("routing").at("owners").at(id)==address,
                      "Preparation did not durably record account ownership.");
            }
            const auto remaining=json::parse(storage_read("fresh-queue"));
            check(remaining.size()+allocated.size()==reserve_count,"Queue consumption was not durable.");
            for (const auto& id : remaining) check(!allocated.contains(id.get<std::string>()),"Reserved ID remained in queue.");
            check(elapsed<60s,"Fresh entity preparation is too slow.");
            // Reload from disk, with no usable queue: IDs and source stamps
            // must replay without changing the checkpoint or touching network.
            upload_task_t resumed;
            resumed.m_session=session;
            resumed.m_journal=std::make_shared<task_journal_t>("fresh-journal",json::parse(initial_state));
            const auto stamp=std::filesystem::last_write_time("fresh-journal/state");
            folder_upload(nullptr,&resumed,"fresh-input",nullptr);
            check(std::filesystem::last_write_time("fresh-journal/state")==stamp,"Fresh task replay rewrote checkpoint.");
            check(resumed.m_entity_arr.size()==task.m_entity_arr.size(),"Fresh replay lost entries.");
            check(storage_read("fresh-journal/state")==initial_state,"Preparation rewrote its initial snapshot.");
            std::cout << "Files: " << files << "; entity records: " << record_count << "; total bytes: " << record_bytes << '\n';
            std::cout << "Fresh preparation milliseconds: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << '\n';
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
