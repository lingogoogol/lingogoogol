#include <nlohmann/json.hpp>
import std;
import storage.google;
import storage.http;
import storage.kernel;
using namespace std::chrono_literals;
using nlohmann::json;

void check(bool ok,const char* message)
{ if (!ok) throw std::runtime_error{message}; }

int main(int argc,char** argv)
{
    try
    {
        check(argc==2,"Missing mode.");
        const std::string mode=argv[1];
        auto account=std::make_shared<google_account_t>("user1@storage-ai-project1.iam.gserviceaccount.com","synthetic-key");
        std::atomic_bool close{};
        auto queue=std::make_shared<blob_queue_t>("test-queue",account.get(),&close);
        auto wait=[&](auto done)
        {
            const auto deadline=std::chrono::steady_clock::now()+15s;
            while (!done())
            {
                queue->check_error();
                check(std::chrono::steady_clock::now()<deadline,"Reserve refill timed out.");
                std::this_thread::sleep_for(10ms);
            }
        };
        auto request=[&](std::string path)
        {
            google_easy_t easy;
            easy.path_set(path); easy.method_get(); easy.run();
            return json::parse(easy.payload());
        };
        if (mode=="full")
        {
            std::this_thread::sleep_for(400ms);
            check(queue->planned_creations()==0,"A full reserve started replenishing.");
        }
        else if (mode=="near-full")
        {
            wait([&] { return queue->planned_creations()>=100; });
            std::this_thread::sleep_for(400ms);
            check(queue->planned_creations()==100,"A one-ID gap scheduled more than one batch.");
            check(queue->count()==19999,"Fixture released its held response too early.");
            request("/fixture/release?count=1");
            wait([&] { return queue->count()==20099; });
            std::this_thread::sleep_for(300ms);
            check(queue->planned_creations()==100,"Refill overshot after its response.");

            storage_session_t session;
            const std::string first="user0@storage-ai-project1.iam.gserviceaccount.com";
            const std::string other="user0@storage-ai-project2.iam.gserviceaccount.com";
            session.accounts[first]=std::make_shared<google_account_t>(first,"synthetic-key");
            session.accounts[account->address()]=account;
            session.accounts[other]=std::make_shared<google_account_t>(other,"synthetic-key");
            session.queues[account->address()]=queue;
            // Project 1 already scheduled 100 background writes. Give project
            // 2 the next 100 allocations, then choose project 1's idle account.
            for (int i{};i<100;++i)
                check(session.choose(1)==other,"Background creation was omitted from project load.");
            check(session.choose(1)==first,"Background creation was omitted from account load.");
        }
        else if (mode=="partial-gap")
        {
            wait([&] { return request("/fixture/status").at("active")==64; });
            std::this_thread::sleep_for(400ms);
            check(queue->planned_creations()==6400,"Initial refill exceeded 64 batches.");
            request("/fixture/release?count=1");
            wait([&] { return queue->planned_creations()>=6500; });
            request("/fixture/release?count=1");
            wait([&] { return queue->planned_creations()>=6600; });
            std::this_thread::sleep_for(400ms);
            check(queue->planned_creations()==6600,"In-flight IDs were omitted from the reserve gap.");
            request("/fixture/release?count=64");
            wait([&] { return queue->count()==20050; });
            std::this_thread::sleep_for(300ms);
            check(queue->planned_creations()==6600,"Completed refill exceeded the rounded target.");
        }
        else if (mode=="empty-retry")
        {
            wait([&] { return queue->count()>=20000; });
            std::this_thread::sleep_for(300ms);
            check(queue->count()==20000,"Empty reserve overshot the target.");
            check(queue->planned_creations()==20000,"Transport retry double-counted a logical create.");
        }
        else throw std::runtime_error{"Unknown mode."};
        queue->stop();
        std::println("PASS: {}",mode);
    }
    catch (const std::exception& error) { std::println("FAIL: {}",error.what()); return 1; }
}
