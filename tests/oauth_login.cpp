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
