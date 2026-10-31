#include <nlohmann/json.hpp>

import std;
import storage.google;
import storage.state;
import storage.kernel;

auto check(bool value,const char* message) -> void
{ if (!value) throw std::runtime_error{message}; }
int main(int argc,char** argv)
{
    try
    {
        check(argc==2,"Missing test mode.");
        const std::string mode{argv[1]};
        auto control=std::make_shared<request_control_t>();
        if (mode=="cloud-denied")
        {
            const auto previous = google_oauth_user_t::restore("client.json","oauth@example.invalid",oauth_purpose_t::cloud);
            bool denied{};
            try { google_oauth_user_t::login("client.json",control,oauth_purpose_t::cloud); }
            catch (const std::exception&) { denied = true; }
            check(denied && google_oauth_user_t::restore("client.json","oauth@example.invalid",oauth_purpose_t::cloud)->credential_id()==previous->credential_id(),
                "Missing consent replaced a valid Cloud login.");
            std::println("PASS: incomplete Cloud consent preserves the previous login.");
            return 0;
        }
        if (mode=="seed-cloud")
        {
            auto user = google_oauth_user_t::login("client.json",control,oauth_purpose_t::cloud);
            check(bool(user) && !user->download_picture(control).empty(),"Cloud login or portrait failed.");
            std::println("PASS: cloud user login and portrait saved separately from Drive credentials.");
            return 0;
        }
        if (mode=="cloud-project-interrupt" || mode=="cloud-lookup-error")
        {
            auto user = google_oauth_user_t::restore("client.json","oauth@example.invalid",oauth_purpose_t::cloud);
            bool failed{};
            try { google_cloud_prepare(user,control); }
            catch (const std::exception& error)
            {
                const auto message = std::string{error.what()};
                failed = mode=="cloud-project-interrupt" ? message.find("HTTP 503")!=std::string::npos :
                    message.find("HTTP 403")!=std::string::npos || message.find("HTTP 404")!=std::string::npos;
            }
            check(failed,"Failed project lookup/operation did not stop provisioning.");
            std::println("PASS: project lookup/operation failure preserves the remembered project.");
            return 0;
        }
        if (mode=="cloud-resume")
        {
            auto user = google_oauth_user_t::restore("client.json","oauth@example.invalid",oauth_purpose_t::cloud);
            check(google_cloud_prepare(user,control).starts_with("storage-ai-"),"Saved project did not survive restart.");
            std::println("PASS: project reuse across processes and delayed search indexing.");
            return 0;
        }
        if (mode=="cloud-provision")
        {
            auto user = google_oauth_user_t::restore("client.json","oauth@example.invalid",oauth_purpose_t::cloud);
            check(google_oauth_user_t::saved_accounts("client.json",oauth_purpose_t::cloud).size()==1,
                "Cloud identity was not restored.");
            const auto project = google_cloud_prepare(user,control);
            check(project.starts_with("storage-ai-") && google_cloud_prepare(user,control)==project,
                "Existing project was not reused.");
            const auto path = google_cloud_create_account(user,control,project,"test-service");
            const auto key = nlohmann::json::parse(storage_read(path));
            check(key.at("client_email")=="test-service@"+project+".iam.gserviceaccount.com",
                "Wrong private key was saved.");
            auto fails = [](auto work)
            {
                try { work(); } catch (const std::exception&) { return; }
                throw std::runtime_error{"Invalid provisioning operation succeeded."};
            };
            for (const auto name : {"bad","UPPER-case","-invalid","invalid-","invalid_name"})
                fails([&] { google_cloud_create_account(user,control,project,name); });
            const auto original = storage_read(path);
            fails([&] { google_cloud_create_account(user,control,project,"test-service"); });
            check(storage_read(path)==original,"Existing key was overwritten.");
            bool disabled{};
            try { google_cloud_create_account(user,control,project,"fail-key"); }
            catch (const std::exception& error)
            {
                disabled = std::string{error.what()}.find("projects/999999")!=std::string::npos &&
                    std::string{error.what()}.find("iam.googleapis.com")!=std::string::npos;
            }
            check(disabled,"SERVICE_DISABLED did not identify the actual API consumer project.");
            check(!std::filesystem::exists("key/"+project+"-fail-key.json"),"Denied key creation wrote a credential.");
            fails([&] { google_cloud_create_account(user,control,project,"race-save"); });
            check(storage_read("key/"+project+"-race-save.json")=="keep-existing","A concurrent key file was overwritten.");
            std::filesystem::remove("key/"+project+"-race-save.json");
            control->cancel();
            fails([&] { google_cloud_prepare(user,control); });
            check(google_oauth_user_t::restore("client.json")->address()=="oauth@example.invalid",
                "Cloud provisioning changed the legacy Drive selection.");
            auto updated = nlohmann::json::parse(storage_read("client.json"));
            updated["installed"]["client_id"] = "updated-client.apps.googleusercontent.com";
            storage_atomic_write("updated-client.json",updated.dump());
            check(google_oauth_user_t::saved_accounts("updated-client.json",oauth_purpose_t::cloud).empty(),
                "A new OAuth client inherited the old client's authorizations.");
            fails([&] { google_oauth_user_t::restore("updated-client.json",user->address(),oauth_purpose_t::cloud); });
            std::filesystem::remove("updated-client.json");
            std::filesystem::remove(path);
            std::filesystem::remove("key");
            std::println("PASS: new/existing Cloud projects, paginated search, API operations, service account/key creation, validation, cancellation, permission denial and atomic key-save rollback.");
            return 0;
        }
        if (mode=="seed-login")
        {
            check(bool(google_oauth_user_t::login("client.json",control)),"Fixture login did not succeed.");
            std::println("PASS: additional OAuth fixture account saved.");
            return 0;
        }
        if (mode=="cancel")
        {
            auto job=std::async(std::launch::async,[&]{return google_oauth_user_t::login("client.json",control);});
            while (!std::filesystem::exists("browser-url")) std::this_thread::sleep_for(std::chrono::milliseconds{10});
            control->cancel();
            try { (void)job.get(); throw std::logic_error{"Cancelled login succeeded."}; }
            catch (const std::runtime_error&) { }
            std::println("PASS: OAuth loopback cancellation."); return 0;
        }
        if (mode=="reject")
        {
            try { google_oauth_user_t::restore("client.json"); }
            catch (const std::exception&) { std::println("PASS: Corrupt OAuth vault was rejected."); return 0; }
            throw std::logic_error{"Invalid OAuth vault was accepted."};
        }
        if (mode=="cng-failure")
        {
            try { google_oauth_user_t::login("client.json",control); }
            catch (const std::runtime_error& error)
            {
                check(std::string{error.what()}.find("CNG")!=std::string::npos,"Failure did not originate in CNG.");
                std::println("PASS: Missing CNG support fails closed."); return 0;
            }
            throw std::logic_error{"Unexpected CNG capability."};
        }
        if (mode=="second-login")
        {
            auto second = google_oauth_user_t::login("client.json",control);
            check(second->address()=="second@example.invalid","Wrong second account.");
            const auto saved = google_oauth_user_t::saved_accounts("client.json");
            check(saved.size()==2 && saved[0].address=="oauth@example.invalid" && !saved[0].portrait.empty(),
                "History did not retain both accounts and the cached portrait.");
            google_oauth_user_t::sign_out("client.json");
            check(!google_oauth_user_t::restore("client.json"),"Sign-out selected an account.");
            check(google_oauth_user_t::saved_accounts("client.json").size()==2,"Sign-out deleted history.");
            google_oauth_user_t::select("client.json","oauth@example.invalid");
            check(google_oauth_user_t::restore("client.json")->address()=="oauth@example.invalid","Selection was not durable.");
            google_oauth_user_t::sign_out("client.json",second->address());
            check(google_oauth_user_t::restore("client.json")->address()=="oauth@example.invalid",
                  "Signing out a background user cleared another user's selection.");
            google_oauth_user_t::remove("client.json",second->address());
            (void)second->refresh(control); // Simulate a delayed refresh from a removed user.
            check(google_oauth_user_t::saved_accounts("client.json").size()==1,"Refresh resurrected a removed account.");
            std::println("PASS: account history, cached photo, durable selection, sign-out and removal race.");
            return 0;
        }
        auto user=mode=="login" ? google_oauth_user_t::login("client.json",control) :
            google_oauth_user_t::restore("client.json");
        check(user && user->address()=="oauth@example.invalid","Wrong authorized user.");
        auto account=std::make_shared<google_account_t>();
        account->init_oauth(user);
        if (mode=="relogin")
        {
            auto next=google_oauth_user_t::login("client.json",control);
            account->update_oauth(next);
            const auto current=next->tokens().access;
            check(account->get_token()==current,"Re-login did not update the stable account.");
            (void)user->refresh(control); // Late completion from the old device key.
            auto restored=google_oauth_user_t::restore("client.json");
            check(restored->credential_id()==next->credential_id() && restored->tokens().access==current,
                "Old refresh overwrote the new login's vault.");
            user=next;
        }
        const auto token=account->get_token();
        check(token.starts_with("fixture-access-"),"Saved access token was not usable.");
        google_easy_t probe;
        auto host=user->picture();
        host.resize(host.rfind('/'));
        probe.host_set(host);
        probe.path_set("/probe");
        probe.m_account_context=account;
        probe.method_get();
        probe.run();
        account->reject_token(token);
        std::vector<std::future<std::string>> readers;
        for (int i{};i<16;++i) readers.push_back(std::async(std::launch::async,[&]{return account->get_token();}));
        const auto next=readers[0].get();
        check(next!=token,"Rejected access token was reused.");
        for (std::size_t i=1;i<readers.size();++i) check(readers[i].get()==next,"Concurrent refreshes diverged.");
        account->reject_token(token);
        check(account->get_token()==next,"Late 401 invalidated the new access token.");
        check(!user->download_picture(control).empty(),"Profile image was not downloaded.");
        google_easy_t create;
        create.m_account_context=account;
        check(blob_upload_run(create,"fixture", "[]").starts_with("created-"),"appData blob creation failed.");
        google_batch_t reserve;
        reserve.m_account_context=account;
        reserve.boundary_set("fixture");
        reserve.path_set("/drive/v3");
        blob_upload(reserve,"fixture-reserve-a", "");
        blob_upload(reserve,"fixture-reserve-b", "");
        reserve.method_post();
        reserve.run();
        std::atomic_bool close{};
        storage_runtime_t runtime{&close};
        auto session=std::make_shared<storage_session_t>();
        session->account=account;
        session->oauth=user;
        session->address=user->address();
        session->storage_id=user->storage_id();
        session->root_path="fixture-clear-root";
        storage_atomic_write("fixture-queue",nlohmann::json(std::vector<std::string>(21000,"placeholder")).dump());
        session->queue=std::make_shared<blob_queue_t>("fixture-queue",account.get(),&close);
        std::atomic_uint64_t progress{},total{};
        check(runtime.clear(session,control,progress,total).starts_with("created-") && total==2 && progress==2,
            "appData clear did not finish within its own data space.");
        account->stop();
        std::println("PASS: OAuth login/restore, cached access, shared background refresh and late 401.");
        return 0;
    }
    catch (const std::exception& error) { std::println("{}",error.what());return 1; }
}
