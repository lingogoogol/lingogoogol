module;
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#include <nlohmann/json.hpp>
module storage.google.cloud;
import std;
import storage.encode;

namespace
{
    using json = nlohmann::json;
    constexpr auto resources = "https://cloudresourcemanager.googleapis.com/v3/";
    constexpr auto services = "https://serviceusage.googleapis.com/v1/";
    constexpr auto iam = "https://iam.googleapis.com/v1/";

    auto check(bool ok, const char* message) -> void
    { if (!ok) throw std::runtime_error{message}; }

    auto encode(std::string_view value) -> std::string
    {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string result;
        for (unsigned char c : value)
            if ((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-' || c=='_' || c=='.')
                result += c;
            else { result += '%'; result += hex[c>>4]; result += hex[c&15]; }
        return result;
    }

    auto known_projects_path(const std::shared_ptr<google_oauth_user_t>& user) -> std::filesystem::path
    {
        // storage_id includes the OAuth client and Google identity. Remember
        // only IDs; names, permissions and lifecycle state must come from GET.
        return std::filesystem::path{"cloud_project/known"}/storage_digest(user->storage_id());
    }

    auto response_value(const oauth_response_t& response,
                        const std::string& method, const std::string& url) -> json
    {
        if (response.status<200 || response.status>=300)
        {
            // Never include response bodies: the same transport also receives
            // private keys. Status plus the failing API identifies the step.
            if (response.status==403)
            {
                const auto error = json::parse(response.body,nullptr,false);
                if (!error.is_discarded() && error.contains("error") && error.at("error").is_object())
                    for (const auto& detail : error.at("error").value("details",json::array()))
                        if (detail.value("reason","")=="SERVICE_DISABLED")
                        {
                            const auto metadata = detail.value("metadata",json::object());
                            const auto service = metadata.value("service","");
                            const auto consumer = metadata.value("consumer","");
                            if (!service.empty() && service.find_first_not_of("abcdefghijklmnopqrstuvwxyz.")==std::string::npos &&
                                consumer.starts_with("projects/") && consumer.size()>9 &&
                                consumer.substr(9).find_first_not_of("0123456789")==std::string::npos)
                                throw std::runtime_error{"請先在 API 呼叫專案 "+consumer+" 啟用 "+service};
                        }
            }
            auto hint = response.status==403 ? "：權限不足、API 未啟用或組織政策禁止此操作" :
                        response.status==409 ? "：名稱已被使用，請改用其他名稱" :
                        response.status==429 ? "：已達配額限制，請稍後重試" : "";
            throw std::runtime_error{std::format("Google Cloud {} {} 失敗（HTTP {}）{}",
                method,url.substr(8,url.find('/',8)-8),response.status,hint)};
        }
        return response.body.empty() ? json::object() : json::parse(response.body);
    }

    auto call(const std::shared_ptr<google_oauth_user_t>& user,
              const std::shared_ptr<request_control_t>& control,
              const std::string& method, const std::string& url, const json& body = nullptr) -> json
    {
        return response_value(user->cloud_request(method,url,body.is_null() ? "" : body.dump(),control),method,url);
    }

    auto operation(const std::shared_ptr<google_oauth_user_t>& user,
                   const std::shared_ptr<request_control_t>& control,
                   const std::string& host, json value) -> void
    {
        const auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes{5};
        while (!value.value("done",false))
        {
            const auto name = value.at("name").get<std::string>();
            check(name.starts_with("operations/") && name.find("..") == std::string::npos &&
                name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/-._")==std::string::npos,
                "Google Cloud 傳回無效的操作 ID");
            for (int i{};i<20;++i)
            {
                check(!control->cancelled,"操作已取消");
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
            }
            check(std::chrono::steady_clock::now()<deadline,"Google Cloud 操作逾時，請稍後重試");
            value = call(user,control,"GET",host+name);
        }
        check(!value.contains("error"),"Google Cloud 未能完成操作，請確認專案權限、配額及組織政策");
    }

    auto save_key(const std::filesystem::path& path, const std::string& content) -> void
    {
        // MoveFileEx without REPLACE_EXISTING preserves another process's file
        // even if it appeared after the initial existence check.
        const auto temporary = path.wstring()+L"."+std::to_wstring(GetCurrentProcessId())+L".tmp";
        storage_atomic_write(temporary,content);
        if (!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(temporary.c_str());
            throw std::runtime_error{"無法保存服務帳戶金鑰，既有檔案不會被覆寫"};
        }
    }
}

auto google_cloud_projects(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control) -> std::vector<cloud_project_t>
{
    std::vector<cloud_project_t> projects;
    std::string token;
    do
    {
        auto url = std::string{resources}+"projects:search?pageSize=100";
        if (!token.empty()) url += "&pageToken="+encode(token);
        const auto result = call(user,control,"GET",url);
        for (const auto& entry : result.value("projects",json::array()))
        {
            const auto id = entry.at("projectId").get<std::string>();
            if (id.starts_with("storage-ai") && entry.value("state","")=="ACTIVE")
                projects.push_back({id,entry.at("displayName")});
        }
        token = result.value("nextPageToken","");
    } while (!token.empty());
    const auto known_path = known_projects_path(user);
    if (std::filesystem::exists(known_path))
    {
        const auto known = json::parse(storage_read(known_path));
        check(known.is_object(),"已保存的專案清單格式無效");
        for (const auto& [id,unused] : known.items())
        {
            check(id.starts_with("storage-ai") && id.size()<=30 && id.back()!='-' &&
                id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,
                "已保存的專案 ID 無效");
            if (std::ranges::find(projects,id,&cloud_project_t::id)!=projects.end()) continue;
            // Projects.search is eventually consistent, including after a
            // process restart. Resolve known IDs that have not been indexed.
            const auto url = std::string{resources}+"projects/"+id;
            const auto response = user->cloud_request("GET",url,"",control);
            if (response.status==403 || response.status==404) continue;
            const auto metadata = response_value(response,"GET",url);
            check(metadata.at("projectId")==id,"Google Cloud 傳回非預期的專案 ID");
            if (metadata.value("state","")=="ACTIVE")
                projects.push_back({id,metadata.at("displayName")});
        }
    }
    std::ranges::sort(projects,{},&cloud_project_t::id);
    return projects;
}

static auto create_project(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& name,
    const std::filesystem::path& record, bool retain_record) -> std::string
{
    check(!name.empty(),"請輸入專案名稱");
    // Retain an uncertain creation's ID/operation across retries and restarts.
    auto saved = std::filesystem::exists(record) ? json::parse(storage_read(record)) : json::object();
    const bool fresh = !saved.contains("id");
    if (fresh)
    {
        std::random_device random;
        saved = {{"id","storage-ai-"+storage_digest(std::to_string(random())+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).substr(0,16)}};
        storage_atomic_write(record,saved.dump());
    }
    const auto project = saved.at("id").get<std::string>();
    if (!saved.value("active",false) && !saved.contains("operation"))
    {
        bool create = fresh;
        if (!fresh)
        {
            const auto url = std::string{resources}+"projects/"+project;
            const auto existing = user->cloud_request("GET",url,"",control);
            // Resource Manager can hide a nonexistent project behind 403.
            // A lookup must not prevent creation. On an uncertain retry, POST
            // only the saved ID; a conflict remains an error, never a new ID.
            create = existing.status==403 || existing.status==404;
            if (!create) response_value(existing,"GET",url);
        }
        if (create)
        {
            saved["operation"] = call(user,control,"POST",std::string{resources}+"projects",
                {{"projectId",project},{"displayName",name}});
            storage_atomic_write(record,saved.dump());
        }
    }
    if (saved.contains("operation")) operation(user,control,resources,saved.at("operation"));
    const auto metadata = call(user,control,"GET",std::string{resources}+"projects/"+project);
    check(metadata.value("state","")=="ACTIVE" && metadata.value("projectId","")==project,
        "專案尚未啟用或 Google Cloud 傳回非預期的專案 ID");
    const auto known_path = known_projects_path(user);
    auto known = std::filesystem::exists(known_path) ? json::parse(storage_read(known_path)) : json::object();
    check(known.is_object(),"已保存的專案清單格式無效");
    known[project] = true;
    storage_atomic_write(known_path,known.dump());
    if (!retain_record) std::filesystem::remove(record);
    else if (!saved.value("active",false))
        storage_atomic_write(record,json{{"id",project},{"active",true}}.dump());
    return project;
}

auto google_cloud_create_project(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& name) -> std::string
{
    return create_project(user,control,name,
        std::filesystem::path{"cloud_project"}/storage_digest(user->storage_id()+":"+name),false);
}

static auto enable_drive(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& project) -> void
{
    const auto metadata = call(user,control,"GET",std::string{resources}+"projects/"+project);
    const auto resource = metadata.at("name").get<std::string>();
    check(resource.starts_with("projects/") && resource.size()>9 &&
        resource.substr(9).find_first_not_of("0123456789")==std::string::npos,"無效的 Google Cloud project number");
    operation(user,control,services,call(user,control,"POST",
        std::string{services}+resource+"/services:batchEnable",
        {{"serviceIds",{"drive.googleapis.com"}}}));
}

auto google_cloud_accounts(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& project)
    -> std::vector<cloud_service_account_t>
{
    check(project.starts_with("storage-ai") && project.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,"無效的專案 ID");
    // JWT requests consume the service account's project quota. Enable Drive
    // on the selected project, rather than the OAuth client's consumer project.
    enable_drive(user,control,project);
    const auto registry = std::filesystem::path{"key/accounts.json"};
    const auto managed = std::filesystem::exists(registry) ? json::parse(storage_read(registry)) : json::object();
    std::vector<cloud_service_account_t> accounts;
    std::string token;
    do
    {
        auto url = std::string{iam}+"projects/"+project+"/serviceAccounts?pageSize=100";
        if (!token.empty()) url += "&pageToken="+encode(token);
        const auto result = call(user,control,"GET",url);
        for (const auto& entry : result.value("accounts",json::array()))
        {
            const auto address = entry.at("email").get<std::string>();
            if (!entry.value("disabled",false) && managed.contains(address))
                accounts.push_back({address,managed.at(address)});
        }
        token = result.value("nextPageToken","");
    } while (!token.empty());
    std::ranges::sort(accounts,{},&cloud_service_account_t::address);
    return accounts;
}

auto google_cloud_prepare(const std::shared_ptr<google_oauth_user_t>& user,
                          const std::shared_ptr<request_control_t>& control) -> std::string
{
    // Projects.search is eventually consistent. Remember a chosen ID across
    // cancelled name dialogs and process restarts, and validate it with GET.
    // storage_id is stable across re-login and includes both client and email;
    // this file contains project IDs/operations only, never credentials.
    const auto record = std::filesystem::path{"cloud_project"}/storage_digest(user->storage_id());
    auto saved = json::object();
    if (std::filesystem::exists(record)) saved = json::parse(storage_read(record));
    std::string project = saved.value("project","");
    bool pending = saved.value("pending_create",false);
    bool create{};
    if (project.empty())
    {
        std::set<std::string> matches;
        std::string token;
        do
        {
            auto url = std::string{resources}+"projects:search?query=displayName%3Astorage-ai&pageSize=100";
            if (!token.empty()) url += "&pageToken="+encode(token);
            const auto result = call(user,control,"GET",url);
            for (const auto& entry : result.value("projects",json::array()))
                if (entry.value("displayName","")=="storage-ai" && entry.value("state","")=="ACTIVE")
                    matches.insert(entry.at("projectId").get<std::string>());
            token = result.value("nextPageToken","");
        } while (!token.empty());
        if (!matches.empty()) project = *matches.begin();
        else
        {
            // Persist BEFORE the POST. Losing its response must never select
            // a fresh ID on retry and silently create another project.
            std::random_device random;
            project = "storage-ai-"+storage_digest(user->address()+std::to_string(random())+
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).substr(0,16);
            pending = true;
            create = true;
        }
        saved = {{"project",project},{"pending_create",pending}};
        storage_atomic_write(record,saved.dump());
    }
    check(project.size()>=6 && project.size()<=30 && project.front()>='a' && project.front()<='z' &&
        project.back()!='-' && project.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,
        "保存的 Google Cloud project ID 無效，請檢查 cloud_project 紀錄");
    auto begin_creation = [&]
    {
        saved["operation"] = call(user,control,"POST",std::string{resources}+"projects",
            {{"projectId",project},{"displayName","storage-ai"}});
        storage_atomic_write(record,saved.dump());
        operation(user,control,resources,saved.at("operation"));
    };
    if (create) begin_creation();
    else if (pending && saved.contains("operation")) operation(user,control,resources,saved.at("operation"));
    const auto url = std::string{resources}+"projects/"+project;
    auto response = user->cloud_request("GET",url,"",control);
    if (pending && !saved.contains("operation") && response.status==404)
    {
        // A previous POST might still be in flight. Reusing the SAME ID is safe;
        // a conflict stops here instead of creating a differently named project.
        begin_creation();
        response = user->cloud_request("GET",url,"",control);
    }
    // 403/404, network failures, deletion and renaming must never be mistaken
    // for an empty search result. Preserve the ID and report the failure.
    const auto metadata = response_value(response,"GET",url);
    check(metadata.value("displayName","")=="storage-ai" && metadata.value("state","")=="ACTIVE" &&
        metadata.value("projectId","")==project,"已記住的專案已更名、刪除或無法使用，請先確認專案狀態");
    storage_atomic_write(record,json{{"project",project},{"pending_create",false}}.dump());
    const auto resource = metadata.at("name").get<std::string>();
    check(resource.starts_with("projects/") && resource.size()>9 &&
        resource.substr(9).find_first_not_of("0123456789")==std::string::npos,"無效的 Google Cloud project number");
    // The user token uses the application's OAuth consumer for provisioning.
    // Enable only Drive here: subsequent JWT calls by the new service account
    // use its own project. Do not enable IAM on every target project merely
    // because IAM resources are being created there.
    operation(user,control,services,call(user,control,"POST",
        std::string{services}+resource+"/services:batchEnable",
        {{"serviceIds",{"drive.googleapis.com"}}}));
    return project;
}

auto google_cloud_validate_account(const std::string& name) -> void
{
    const auto digit = [](char c) { return c>='0' && c<='9'; };
    const auto letter = [](char c) { return c>='a' && c<='z'; };
    check(name.size()>=6 && name.size()<=30 && letter(name.front()) &&
        (letter(name.back()) || digit(name.back())) &&
        std::ranges::all_of(name,[&](char c) { return letter(c) || digit(c) || c=='-'; }),
        "名稱須為 6–30 個小寫英文字母、數字或連字號，以字母開頭，不能以連字號結尾");
}

static auto create_account(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& project,
    const std::string& name, const std::filesystem::path& directory,
    const std::filesystem::path& key_attempt = {}) -> std::filesystem::path
{
    google_cloud_validate_account(name);
    check(!project.empty() && project.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,
        "無效的 Google Cloud project ID");
    std::filesystem::create_directories(directory);
    const auto path = directory/(project+"-"+name+".json");
    const auto email = name+"@"+project+".iam.gserviceaccount.com";
    const auto register_key = [&]
    {
        const auto registry = directory/"accounts.json";
        auto managed = std::filesystem::exists(registry) ? json::parse(storage_read(registry)) : json::object();
        if (managed.value(email,std::string{})==path.generic_string()) return;
        managed[email] = path.generic_string();
        storage_atomic_write(registry,managed.dump(2));
    };
    const auto validate_key = [&](std::string_view private_key)
    {
        const auto credential = json::parse(private_key);
        check(credential.at("type")=="service_account" && credential.at("client_email")==email &&
            credential.at("project_id")==project && !credential.at("private_key").get<std::string>().empty(),
            "Google Cloud 傳回不完整的金鑰");
    };
    if (std::filesystem::exists(path))
    {
        check(!key_attempt.empty(),"金鑰檔案已存在，請選擇其他服務帳戶名稱");
        validate_key(storage_read(path));
        register_key();
        std::filesystem::remove(key_attempt);
        return path;
    }
    const auto base = std::string{iam}+"projects/"+project+"/serviceAccounts";
    const auto account_url = base+"/"+encode(email);
    if (!key_attempt.empty() && std::filesystem::exists(key_attempt))
        throw std::runtime_error{"金鑰建立結果未知，請在 Google Cloud IAM 核對 "+email+
            " 的金鑰後，再依 README 說明繼續"};
    const auto account_body = json{{"accountId",name},{"serviceAccount",{{"displayName",name}}}};
    const auto account_deadline = std::chrono::steady_clock::now()+std::chrono::minutes{5};
    oauth_response_t created;
    for (;;)
    {
        check(!control->cancelled,"操作已取消");
        created = user->cloud_request("POST",base,account_body.dump(),control);
        if (created.status!=429) break;
        auto delay = std::chrono::seconds{60};
        const auto error = json::parse(created.body,nullptr,false);
        if (!error.is_discarded() && error.contains("error") && error.at("error").is_object())
            for (const auto& detail : error.at("error").value("details",json::array()))
                if (detail.value("@type","")=="type.googleapis.com/google.rpc.RetryInfo")
                {
                    const auto text = detail.value("retryDelay","");
                    if (!text.ends_with('s')) continue;
                    int seconds{};
                    const auto [end,status] = std::from_chars(text.data(),text.data()+text.size()-1,seconds);
                    if (status==std::errc{} && end==text.data()+text.size()-1 && seconds>0)
                        delay=std::chrono::seconds{seconds};
                }
        const auto wake = std::chrono::steady_clock::now()+delay;
        if (wake>account_deadline) break;
        while (std::chrono::steady_clock::now()<wake)
        {
            check(!control->cancelled,"操作已取消");
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
    }
    const auto account = !key_attempt.empty() && created.status==409
        ? call(user,control,"GET",account_url) : response_value(created,"POST",base);
    check(account.at("email")==email && !account.value("disabled",false),"Google Cloud 傳回非預期或已停用的服務帳戶");
    // Retry explicit 404s while a new account propagates through IAM. An
    // uncertain response must still stop: replaying it could create extra keys.
    const auto key_url = account_url+"/keys";
    const auto body = json{{"privateKeyType","TYPE_GOOGLE_CREDENTIALS_FILE"},{"keyAlgorithm","KEY_ALG_RSA_2048"}};
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes{5};
    auto delay = std::chrono::milliseconds{1000};
    std::random_device random;
    oauth_response_t response;
    for (;;)
    {
        check(!control->cancelled,"操作已取消");
        if (!key_attempt.empty()) storage_atomic_write(key_attempt,email);
        response = user->cloud_request("POST",key_url,body.dump(),control);
        if (!key_attempt.empty() && response.status>=400 && response.status<500 && response.status!=408)
            std::filesystem::remove(key_attempt);
        if (response.status!=404 || std::chrono::steady_clock::now()>=deadline) break;
        const auto wake = std::min(deadline,std::chrono::steady_clock::now()+delay+
            std::chrono::milliseconds{random()%1000});
        while (std::chrono::steady_clock::now()<wake)
        {
            check(!control->cancelled,"操作已取消");
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
        delay=std::min(delay*2,std::chrono::milliseconds{30000});
    }
    const auto key = response_value(response,"POST",key_url);
    bool key_saved{};
    try
    {
        auto private_key = base64url_decode(key.at("privateKeyData").get<std::string>());
        validate_key(private_key);
        save_key(path,private_key);
        key_saved=true;
        register_key();
        std::fill(private_key.begin(),private_key.end(),'\0');
    }
    catch (...)
    {
        // If the downloaded key cannot be saved, revoke precisely that key.
        // Never delete a pre-existing account or touch its other keys.
        const auto failure = std::current_exception();
        if (key_saved && !key_attempt.empty()) std::rethrow_exception(failure);
        if (key_saved) std::filesystem::remove(path);
        const auto key_name = key.value("name","");
        const auto expected = "projects/"+project+"/serviceAccounts/"+email+"/keys/";
        if (key_name.starts_with(expected) && key_name.substr(expected.size()).find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==std::string::npos)
            try
            {
                call(user,std::make_shared<request_control_t>(),"DELETE",std::string{iam}+key_name);
                if (!key_attempt.empty()) std::filesystem::remove(key_attempt);
            }
            catch (...) { throw std::runtime_error{"金鑰未能保存且無法撤銷，請到 Google Cloud IAM 撤銷本次金鑰"}; }
        std::rethrow_exception(failure);
    }
    if (!key_attempt.empty()) std::filesystem::remove(key_attempt);
    return path;
}

auto google_cloud_create_account(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& project,
    const std::string& name, const std::filesystem::path& directory) -> std::filesystem::path
{
    return create_account(user,control,project,name,directory);
}

auto google_cloud_provision(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& setup_id,
    const std::vector<std::size_t>& account_counts, std::atomic_size_t& completed)
    -> std::vector<std::string>
{
    check(setup_id.size()==64 && setup_id.find_first_not_of("0123456789abcdef")==std::string::npos,
        "保存的並行配置 ID 無效");
    const auto directory = std::filesystem::path{"cloud_project/parallel"}/setup_id;
    std::vector<std::string> addresses;
    std::string step;
    try
    {
        for (std::size_t i=0;i<account_counts.size();++i)
        {
            const auto number = std::to_string(i+1);
            step = "建立專案 "+number+"/"+std::to_string(account_counts.size());
            check(!control->cancelled,"操作已取消");
            const auto record = directory/("project-"+number);
            const auto project = create_project(user,control,"storage-ai-"+setup_id.substr(0,8)+"-"+number,record,true);
            step = "啟用 Drive API（"+project+"）";
            const auto ready = directory/("drive-"+number);
            if (!std::filesystem::exists(ready))
            {
                enable_drive(user,control,project);
                storage_atomic_write(ready,project);
            }
            for (std::size_t j=0;j<account_counts[i];++j)
            {
                const auto name = "storage-"+std::to_string(j+1);
                step = "配置 "+project+"/"+name;
                check(!control->cancelled,"操作已取消");
                create_account(user,control,project,name,"key",directory/("key-"+number+"-"+std::to_string(j+1)));
                addresses.push_back(name+"@"+project+".iam.gserviceaccount.com");
                completed.store(addresses.size());
            }
        }
    }
    catch (const std::exception& error) { throw std::runtime_error{step+"："+error.what()}; }
    return addresses;
}
