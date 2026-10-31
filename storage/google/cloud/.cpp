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

auto google_cloud_create_account(const std::shared_ptr<google_oauth_user_t>& user,
    const std::shared_ptr<request_control_t>& control, const std::string& project,
    const std::string& name, const std::filesystem::path& directory) -> std::filesystem::path
{
    google_cloud_validate_account(name);
    check(!project.empty() && project.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos,
        "無效的 Google Cloud project ID");
    std::filesystem::create_directories(directory);
    const auto path = directory/(project+"-"+name+".json");
    check(!std::filesystem::exists(path),"金鑰檔案已存在，請選擇其他服務帳戶名稱");
    const auto base = std::string{iam}+"projects/"+project+"/serviceAccounts";
    const auto account = call(user,control,"POST",base,{{"accountId",name},{"serviceAccount",{{"displayName",name}}}});
    const auto email = account.at("email").get<std::string>();
    check(email==name+"@"+project+".iam.gserviceaccount.com","Google Cloud 傳回非預期的服務帳戶");
    // Key POSTs are intentionally sent once. Google cannot retrieve a private
    // key again, and replaying an ambiguous response would leak extra keys.
    const auto key = call(user,control,"POST",base+"/"+encode(email)+"/keys",
        {{"privateKeyType","TYPE_GOOGLE_CREDENTIALS_FILE"},{"keyAlgorithm","KEY_ALG_RSA_2048"}});
    try
    {
        auto private_key = base64url_decode(key.at("privateKeyData").get<std::string>());
        const auto credential = json::parse(private_key);
        check(credential.at("type")=="service_account" && credential.at("client_email")==email &&
            credential.at("project_id")==project && !credential.at("private_key").get<std::string>().empty(),
            "Google Cloud 傳回不完整的金鑰");
        save_key(path,private_key);
        std::fill(private_key.begin(),private_key.end(),'\0');
    }
    catch (...)
    {
        // If the downloaded key cannot be saved, revoke precisely that key.
        // Never delete a pre-existing account or touch its other keys.
        const auto failure = std::current_exception();
        const auto key_name = key.value("name","");
        const auto expected = "projects/"+project+"/serviceAccounts/"+email+"/keys/";
        if (key_name.starts_with(expected) && key_name.substr(expected.size()).find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-")==std::string::npos)
            try
            {
                call(user,std::make_shared<request_control_t>(),"DELETE",std::string{iam}+key_name);
            }
            catch (...) { throw std::runtime_error{"金鑰未能保存且無法撤銷，請到 Google Cloud IAM 撤銷本次金鑰"}; }
        std::rethrow_exception(failure);
    }
    return path;
}
