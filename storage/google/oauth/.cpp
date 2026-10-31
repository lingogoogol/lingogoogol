module;
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <ncrypt.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#undef curl_easy_setopt

module storage.google.oauth;
import std;
import storage.state;
import storage.encode;

namespace
{
    using json = nlohmann::json;
    constexpr auto token_endpoint = "https://oauth2.googleapis.com/token";
    constexpr auto auth_endpoint = "https://accounts.google.com/o/oauth2/v2/auth";
    constexpr auto drive_scope = "https://www.googleapis.com/auth/drive.appdata";
    constexpr auto cloud_scopes = "https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile";
    std::mutex vault_mutex;

    auto require(bool ok, const char* message) -> void
    {
        if (!ok) throw std::runtime_error{message};
    }
    // The storage encoder preserves Base64 padding for blob contents. OAuth
    // PKCE and JWS require the unpadded Base64URL form instead.
    auto oauth_base64(std::string value) -> std::string
    {
        auto result = base64url_encode(std::move(value));
        while (!result.empty() && result.back()=='=') result.pop_back();
        return result;
    }
    auto oauth_unbase64(std::string value) -> std::string
    {
        while (value.size()%4) value += '=';
        return base64url_decode(std::move(value));
    }
    auto random_bytes(std::size_t count) -> std::string
    {
        std::string result(count, '\0');
        require(BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(result.data()),
            static_cast<ULONG>(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0,
            "Could not generate OAuth randomness.");
        return result;
    }
    auto sha256(std::string_view value) -> std::string
    {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), digest);
        return {reinterpret_cast<char*>(digest), sizeof(digest)};
    }
    auto percent(std::string_view value) -> std::string
    {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string result;
        for (unsigned char c : value)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
                result += static_cast<char>(c);
            else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
        }
        return result;
    }
    auto unpercent(std::string_view value) -> std::string
    {
        auto hex = [](char c) -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        std::string result;
        for (std::size_t i{}; i < value.size(); ++i)
        {
            if (value[i] == '%')
            {
                require(i + 2 < value.size() && hex(value[i+1]) >= 0 && hex(value[i+2]) >= 0,
                        "Invalid OAuth callback encoding.");
                result += static_cast<char>((hex(value[i+1]) << 4) | hex(value[i+2]));
                i += 2;
            }
            else result += value[i] == '+' ? ' ' : value[i];
        }
        return result;
    }
    struct client_t
    {
        std::string id, secret;
        oauth_purpose_t purpose;
        auto scope() const -> std::string { return purpose==oauth_purpose_t::cloud ? cloud_scopes : drive_scope; }
        auto vault_id() const -> std::string
        { return purpose==oauth_purpose_t::cloud ? id+":cloud-platform" : id; }
        explicit client_t(const std::string& path, oauth_purpose_t kind = oauth_purpose_t::drive) : purpose{kind}
        {
            try
            {
                const auto data = json::parse(storage_read(path)).at("installed");
                id = data.at("client_id").get<std::string>();
                secret = data.value("client_secret", "");
                require(!id.empty() && data.at("token_uri") == token_endpoint &&
                    (data.at("auth_uri") == auth_endpoint ||
                     data.at("auth_uri") == "https://accounts.google.com/o/oauth2/auth"),
                    "Invalid desktop OAuth endpoints.");
            }
            catch (...) { throw std::runtime_error{"Invalid Google desktop OAuth client.json."}; }
        }
    };
    struct key_t
    {
        NCRYPT_PROV_HANDLE provider{};
        NCRYPT_KEY_HANDLE handle{};
        bool remove_on_close{};
        explicit key_t(const std::wstring& name, bool create, bool signing = false)
        {
            require(NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0) == 0,
                    "Windows CNG key storage is unavailable.");
            auto status = NCryptOpenKey(provider, &handle, name.c_str(), 0, 0);
            if (status != 0 && create && status == NTE_BAD_KEYSET)
            {
                status = NCryptCreatePersistedKey(provider, &handle,
                    signing ? BCRYPT_ECDSA_P256_ALGORITHM : BCRYPT_RSA_ALGORITHM,
                    name.c_str(), 0, 0);
                if (status == NTE_EXISTS) status = NCryptOpenKey(provider, &handle, name.c_str(), 0, 0);
                else if (status == 0)
                {
                    DWORD policy{}; // Private keys must never be exportable.
                    status = NCryptSetProperty(handle, NCRYPT_EXPORT_POLICY_PROPERTY,
                        reinterpret_cast<PBYTE>(&policy), sizeof(policy), 0);
                    if (!signing && status == 0)
                    {
                        DWORD bits{2048};
                        status = NCryptSetProperty(handle, NCRYPT_LENGTH_PROPERTY,
                            reinterpret_cast<PBYTE>(&bits), sizeof(bits), 0);
                    }
                    if (status == 0) status = NCryptFinalizeKey(handle, 0);
                    if (status != 0) { NCryptDeleteKey(handle, 0); handle = 0; }
                }
            }
            if (status != 0)
            {
                if (handle) NCryptFreeObject(handle);
                NCryptFreeObject(provider);
                provider = handle = 0;
                throw std::runtime_error{"Could not open the Windows CNG OAuth key."};
            }
        }
        ~key_t()
        {
            if (handle)
            {
                if (remove_on_close) NCryptDeleteKey(handle, 0);
                else NCryptFreeObject(handle);
            }
            if (provider) NCryptFreeObject(provider);
        }
        key_t(const key_t&) = delete;
        auto proof(std::string jti, const std::string& nonce) -> std::string
        {
            std::array<unsigned char, sizeof(BCRYPT_ECCKEY_BLOB) + 64> blob{};
            DWORD count{};
            require(NCryptExportKey(handle, 0, BCRYPT_ECCPUBLIC_BLOB, nullptr, blob.data(),
                static_cast<DWORD>(blob.size()), &count, 0) == 0 && count == blob.size(),
                "Could not read the DPoP public key.");
            BCRYPT_ECCKEY_BLOB header{};
            std::memcpy(&header, blob.data(), sizeof(header));
            require(header.dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC && header.cbKey == 32,
                "Invalid DPoP key algorithm.");
            auto coordinate = [&](std::size_t offset)
            { return oauth_base64({reinterpret_cast<char*>(blob.data()+sizeof(header)+offset),32}); };
            auto jwt = oauth_base64(json{{"typ","dpop+jwt"},{"alg","ES256"},
                {"jwk",{{"kty","EC"},{"crv","P-256"},{"x",coordinate(0)},{"y",coordinate(32)}}}}.dump());
            auto claims = json{{"jti",std::move(jti)},{"htm","POST"},{"htu",token_endpoint},
                {"iat",std::time(nullptr)}};
            if (!nonce.empty()) claims["nonce"] = nonce;
            jwt += "." + oauth_base64(claims.dump());
            auto digest = sha256(jwt);
            std::string signature(64, '\0');
            // CNG returns raw R || S, which is already the JWS ES256 format.
            require(NCryptSignHash(handle, nullptr, reinterpret_cast<PBYTE>(digest.data()), 32,
                reinterpret_cast<PBYTE>(signature.data()), 64, &count, 0) == 0 && count == 64,
                "Could not sign the DPoP proof.");
            return jwt + "." + oauth_base64(signature);
        }
    };
    auto widen_ascii(std::string_view value) -> std::wstring { return {value.begin(), value.end()}; }
    auto vault_directory(const client_t& client) -> std::filesystem::path
    {
        std::array<wchar_t,32768> path{};
        const auto count = GetEnvironmentVariableW(L"LOCALAPPDATA", path.data(), path.size());
        require(count > 0 && count < path.size(), "Windows LocalAppData is unavailable.");
        return std::filesystem::path{path.data()} / "lingogoogol" / "storage" / "oauth" / storage_digest(client.vault_id());
    }
    auto crypt_key_name(const client_t& client, const std::string& account) -> std::wstring
    { return L"lingogoogol.storage.tokens." + widen_ascii(storage_digest(client.vault_id() + ":" + account)); }

    auto wrap(key_t& key, std::string input, bool decrypt) -> std::string
    {
        std::string output(256, '\0');
        DWORD size{};
        BCRYPT_OAEP_PADDING_INFO padding{BCRYPT_SHA256_ALGORITHM,nullptr,0};
        auto operation = decrypt ? NCryptDecrypt : NCryptEncrypt;
        const auto ok = operation(key.handle, reinterpret_cast<PBYTE>(input.data()),
            static_cast<DWORD>(input.size()), &padding, reinterpret_cast<PBYTE>(output.data()),
            static_cast<DWORD>(output.size()), &size, NCRYPT_PAD_OAEP_FLAG) == 0;
        SecureZeroMemory(input.data(), input.size());
        require(ok, "Windows CNG could not protect the OAuth tokens.");
        output.resize(size);
        return output;
    }
    auto aes(std::string_view input, const std::string& key, const std::string& nonce,
             std::string& tag, bool encrypt) -> std::string
    {
        auto ctx = std::unique_ptr<EVP_CIPHER_CTX,decltype(&EVP_CIPHER_CTX_free)>{EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free};
        require(ctx != nullptr && key.size() == 32 && nonce.size() == 12 &&
            (encrypt || tag.size() == 16) && input.size() <= 1024*1024, "Invalid OAuth token envelope.");
        auto bytes = [](const std::string& s) { return reinterpret_cast<const unsigned char*>(s.data()); };
        require(EVP_CipherInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, bytes(key), bytes(nonce), encrypt) == 1,
            "Could not initialize OAuth token encryption.");
        if (!encrypt) require(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, tag.data()) == 1,
            "Invalid OAuth token authentication tag.");
        std::string output(input.size()+16,'\0');
        int count{}, final{};
        require(EVP_CipherUpdate(ctx.get(), reinterpret_cast<unsigned char*>(output.data()), &count,
            reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size())) == 1,
            "Could not process encrypted OAuth tokens.");
        if (EVP_CipherFinal_ex(ctx.get(), reinterpret_cast<unsigned char*>(output.data())+count, &final) != 1)
        {
            SecureZeroMemory(output.data(), output.size());
            throw std::runtime_error{"OAuth token authentication failed."};
        }
        output.resize(count+final);
        if (encrypt)
        {
            tag.resize(16);
            require(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data()) == 1,
                "Could not authenticate encrypted OAuth tokens.");
        }
        return output;
    }
    auto load_vault_locked(const client_t& client, const std::string& account) -> json;
    auto save_vault(const client_t& client, const std::string& account, const json& data, bool select) -> void
    {
        std::lock_guard lock{vault_mutex};
        if (!select)
        {
            // Removal wins over an already-running token refresh or photo download.
            if (!std::filesystem::exists(vault_directory(client)/(account+".json"))) return;
            // A previous login may still finish an in-flight refresh after a
            // new consent issued another device key. Never replace that login's
            // durable credentials with the superseded refresh token.
            const auto current = load_vault_locked(client,account);
            if (current.at("dpop_key") != data.at("dpop_key")) return;
        }
        key_t key{crypt_key_name(client, account),true};
        auto secret = random_bytes(32);
        const auto nonce = random_bytes(12);
        std::string tag;
        auto plain = data.dump();
        std::string cipher, wrapped;
        try { cipher = aes(plain,secret,nonce,tag,true); wrapped = wrap(key,secret,false); }
        catch (...) { SecureZeroMemory(secret.data(),secret.size()); SecureZeroMemory(plain.data(),plain.size()); throw; }
        SecureZeroMemory(secret.data(),secret.size());
        SecureZeroMemory(plain.data(),plain.size());
        const auto path = vault_directory(client);
        storage_atomic_write(path/(account+".json"), json{{"version",1},{"key",oauth_base64(wrapped)},
            {"nonce",oauth_base64(nonce)},{"tag",oauth_base64(tag)},
            {"cipher",oauth_base64(cipher)}}.dump());
        if (select) storage_atomic_write(path/"active",account);
    }
    auto load_vault_locked(const client_t& client, const std::string& account) -> json
    {
        const auto path = vault_directory(client)/(account+".json");
        require(std::filesystem::file_size(path) <= 2*1024*1024, "OAuth token envelope is too large.");
        const auto envelope = json::parse(storage_read(path));
        require(envelope.at("version") == 1, "Unknown OAuth token envelope version.");
        key_t key{crypt_key_name(client,account),false};
        auto secret = wrap(key,oauth_unbase64(envelope.at("key")),true);
        auto tag = oauth_unbase64(envelope.at("tag"));
        std::string plain;
        try { plain = aes(oauth_unbase64(envelope.at("cipher")),secret,
            oauth_unbase64(envelope.at("nonce")),tag,false); }
        catch (...) { SecureZeroMemory(secret.data(),secret.size()); throw; }
        SecureZeroMemory(secret.data(),secret.size());
        try { auto data = json::parse(plain); SecureZeroMemory(plain.data(),plain.size()); return data; }
        catch (...) { SecureZeroMemory(plain.data(),plain.size()); throw; }
    }
    auto load_vault(const client_t& client, const std::string& account) -> json
    {
        std::lock_guard lock{vault_mutex};
        return load_vault_locked(client,account);
    }
    struct response_t { long status{}; std::string body, nonce; };
    auto request(const std::string& url, const std::string& body, const std::vector<std::string>& headers,
                 const std::shared_ptr<request_control_t>& control, const std::string& method = {}) -> response_t
    {
        auto easy = std::unique_ptr<CURL,decltype(&curl_easy_cleanup)>{curl_easy_init(),curl_easy_cleanup};
        require(easy != nullptr,"Could not initialize the OAuth connection.");
        curl_slist* list{};
        for (const auto& header : headers)
        {
            auto next = curl_slist_append(list,header.c_str());
            if (!next) { curl_slist_free_all(list); throw std::bad_alloc{}; }
            list = next;
        }
        auto cleanup = std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)>{list,curl_slist_free_all};
        response_t response;
        // Never enable curl diagnostics here: request/response bodies contain credentials.
        curl_easy_setopt(easy.get(),CURLOPT_URL,url.c_str());
        curl_easy_setopt(easy.get(),CURLOPT_HTTPHEADER,list);
        curl_easy_setopt(easy.get(),CURLOPT_CONNECTTIMEOUT,15L);
        curl_easy_setopt(easy.get(),CURLOPT_TIMEOUT,45L);
        curl_easy_setopt(easy.get(),CURLOPT_PROTOCOLS_STR,"https");
        curl_easy_setopt(easy.get(),CURLOPT_VERBOSE,0L);
        curl_easy_setopt(easy.get(),CURLOPT_WRITEFUNCTION,+[](char* p,std::size_t a,std::size_t b,void* out)->std::size_t
        {
            auto& value = static_cast<response_t*>(out)->body;
            if (a*b > 1024*1024-value.size()) return 0;
            value.append(p,a*b); return a*b;
        });
        curl_easy_setopt(easy.get(),CURLOPT_WRITEDATA,&response);
        curl_easy_setopt(easy.get(),CURLOPT_HEADERFUNCTION,+[](char* p,std::size_t a,std::size_t b,void* out)->std::size_t
        {
            std::string_view line{p,a*b};
            constexpr std::string_view prefix{"dpop-nonce:"};
            bool match = line.size() >= prefix.size();
            for (std::size_t i{};match && i<prefix.size();++i)
                match = std::tolower(static_cast<unsigned char>(line[i])) == prefix[i];
            if (match)
            {
                line.remove_prefix(prefix.size());
                while (!line.empty() && (line.front()==' ' || line.front()=='\t')) line.remove_prefix(1);
                while (!line.empty() && (line.back()=='\r' || line.back()=='\n' || line.back()==' ')) line.remove_suffix(1);
                if (line.size() > 8192) return 0;
                static_cast<response_t*>(out)->nonce = line;
            }
            return a*b;
        });
        curl_easy_setopt(easy.get(),CURLOPT_HEADERDATA,&response);
        curl_easy_setopt(easy.get(),CURLOPT_NOPROGRESS,0L);
        curl_easy_setopt(easy.get(),CURLOPT_XFERINFOFUNCTION,+[](void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t)->int
        { return static_cast<request_control_t*>(p)->cancelled ? 1 : 0; });
        curl_easy_setopt(easy.get(),CURLOPT_XFERINFODATA,control.get());
        curl_easy_setopt(easy.get(),CURLOPT_OPENSOCKETFUNCTION,+[](void* p,curlsocktype,
            struct curl_sockaddr* address)->curl_socket_t
        { return static_cast<curl_socket_t>(static_cast<request_control_t*>(p)->socket_open(
            address->family,address->socktype,address->protocol)); });
        curl_easy_setopt(easy.get(),CURLOPT_OPENSOCKETDATA,control.get());
        curl_easy_setopt(easy.get(),CURLOPT_CLOSESOCKETFUNCTION,+[](void* p,curl_socket_t socket)->int
        { return static_cast<request_control_t*>(p)->socket_close(socket); });
        curl_easy_setopt(easy.get(),CURLOPT_CLOSESOCKETDATA,control.get());
        if (!body.empty())
        {
            curl_easy_setopt(easy.get(),CURLOPT_POSTFIELDS,body.c_str());
            curl_easy_setopt(easy.get(),CURLOPT_POSTFIELDSIZE,static_cast<long>(body.size()));
        }
        if (!method.empty()) curl_easy_setopt(easy.get(),CURLOPT_CUSTOMREQUEST,method.c_str());
        const auto status = curl_easy_perform(easy.get());
        require(!control->cancelled,"Google login was cancelled.");
        require(status == CURLE_OK,"Google OAuth network request failed.");
        curl_easy_getinfo(easy.get(),CURLINFO_RESPONSE_CODE,&response.status);
        return response;
    }
    auto exchange(const client_t& client, key_t& key, const std::string& body, std::string code,
                  oauth_tokens_t previous, const std::shared_ptr<request_control_t>& control) -> oauth_tokens_t
    {
        for (int attempt{};attempt<3;++attempt)
        {
            const auto started = std::time(nullptr);
            const auto proof = key.proof(code.empty() ? oauth_base64(random_bytes(24)) :
                oauth_base64(sha256(code)),previous.nonce);
            const auto response = request(token_endpoint,body+"&client_id="+percent(client.id)+
                "&client_secret="+percent(client.secret),
                {"Content-Type: application/x-www-form-urlencoded","DPoP: "+proof},control);
            json data;
            try { data = json::parse(response.body); }
            catch (...) { throw std::runtime_error{"Invalid Google OAuth response."}; }
            if (!response.nonce.empty()) previous.nonce = response.nonce;
            if (response.status == 400 && data.value("error", "") == "use_dpop_nonce" && !response.nonce.empty())
                continue;
            if (response.status >= 500 && attempt<2) continue;
            require(response.status == 200,"Google rejected the OAuth token exchange; sign in again.");
            try
            {
                auto result = previous;
                result.access = data.at("access_token").get<std::string>();
                if (data.contains("refresh_token")) result.refresh = data.at("refresh_token").get<std::string>();
                require(data.at("expires_in").is_number_integer(),"Invalid OAuth token lifetime.");
                const auto lifetime = data.at("expires_in").get<std::int64_t>();
                // Google binds the REFRESH token only. Drive still receives Bearer.
                require(data.at("token_type") == "Bearer" && !result.access.empty() &&
                    !result.refresh.empty() && lifetime>0 && lifetime<=86400,"Invalid OAuth tokens.");
                if (!code.empty())
                {
                    require(!response.nonce.empty(),"Google did not confirm DPoP nonce support.");
                    std::istringstream scopes{data.value("scope","")};
                    std::set<std::string> granted;
                    for (std::string scope; scopes >> scope;) granted.insert(scope);
                    std::istringstream requested{client.scope()};
                    for (std::string scope; requested >> scope;)
                        require(granted.contains(scope),"A required Google permission was not granted.");
                }
                result.expires = started+lifetime;
                return result;
            }
            catch (...) { throw std::runtime_error{"Google returned incomplete tokens or required permission was not granted."}; }
        }
        throw std::runtime_error{"Google OAuth nonce negotiation failed."};
    }
    struct loopback_t
    {
        SOCKET listener{INVALID_SOCKET};
        bool winsock{};
        std::string host, redirect;
        loopback_t()
        {
            WSADATA data{};
            require(WSAStartup(MAKEWORD(2,2),&data)==0,"Could not initialize OAuth loopback networking.");
            winsock = true;
            listener = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            BOOL exclusive{TRUE};
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            int length{sizeof(address)};
            if (listener == INVALID_SOCKET ||
                setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive)) != 0 ||
                bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address)) != 0 ||
                getsockname(listener,reinterpret_cast<sockaddr*>(&address),&length) != 0 || listen(listener,4) != 0)
            {
                if (listener != INVALID_SOCKET) closesocket(listener);
                WSACleanup(); winsock = false;
                throw std::runtime_error{"Could not listen on the OAuth loopback address."};
            }
            host = "127.0.0.1:"+std::to_string(ntohs(address.sin_port));
            redirect = "http://"+host+"/callback";
        }
        ~loopback_t() { if(listener!=INVALID_SOCKET) closesocket(listener); if(winsock) WSACleanup(); }
        auto wait(const std::string& state, const std::shared_ptr<request_control_t>& control) -> std::string
        {
            const auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes{5};
            while (!control->cancelled && std::chrono::steady_clock::now()<deadline)
            {
                fd_set sockets; FD_ZERO(&sockets); FD_SET(listener,&sockets);
                timeval timeout{0,100000};
                if (select(0,&sockets,nullptr,nullptr,&timeout)<=0) continue;
                const auto connection = accept(listener,nullptr,nullptr);
                if (connection == INVALID_SOCKET) continue;
                struct cleanup_t { SOCKET value; ~cleanup_t(){closesocket(value);} } cleanup{connection};
                DWORD read_timeout{100};
                setsockopt(connection,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&read_timeout),sizeof(read_timeout));
                std::string input;
                std::array<char,2048> buffer{};
                const auto read_deadline = std::chrono::steady_clock::now()+std::chrono::seconds{2};
                while (!control->cancelled && input.size()<8192 && input.find("\r\n\r\n")==std::string::npos &&
                       std::chrono::steady_clock::now()<read_deadline)
                {
                    const auto count = recv(connection,buffer.data(),static_cast<int>(buffer.size()),0);
                    if (count > 0) input.append(buffer.data(),count);
                    else if (count == 0 || WSAGetLastError()!=WSAETIMEDOUT) break;
                }
                bool accepted{}; std::string code; bool denied{};
                try
                {
                    const auto end = input.find(" HTTP/1.");
                    require(input.starts_with("GET /callback?") && end!=std::string::npos &&
                        input.find("\r\nHost: "+host+"\r\n")!=std::string::npos,
                        "Unexpected OAuth callback.");
                    std::string_view query{input.data()+14,end-14};
                    std::map<std::string,std::string> params;
                    while (!query.empty())
                    {
                        const auto part = query.substr(0,query.find('&'));
                        const auto split = part.find('=');
                        require(split!=std::string_view::npos,"Invalid OAuth callback query.");
                        require(params.emplace(unpercent(part.substr(0,split)),unpercent(part.substr(split+1))).second,
                            "Duplicate OAuth callback parameter.");
                        if (part.size()==query.size()) break;
                        query.remove_prefix(part.size()+1);
                    }
                    require(params["state"]==state,"OAuth state did not match.");
                    if (params.contains("iss")) require(params["iss"]=="https://accounts.google.com","Unexpected OAuth issuer.");
                    denied = params.contains("error");
                    code = params["code"];
                    accepted = denied || !code.empty();
                }
                catch (...) { }
                const std::string page = accepted ? "Google sign-in completed. You can return to Storage." : "Invalid sign-in callback.";
                const std::string reply = std::string{"HTTP/1.1 "}+(accepted?"200 OK":"400 Bad Request")+
                    "\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nContent-Security-Policy: default-src 'none'\r\nConnection: close\r\nContent-Length: "+std::to_string(page.size())+"\r\n\r\n"+page;
                send(connection,reply.data(),static_cast<int>(reply.size()),0);
                if (accepted)
                {
                    require(!denied,"Google sign-in was declined.");
                    return code;
                }
            }
            throw std::runtime_error{control->cancelled ? "Google login was cancelled." : "Google login timed out."};
        }
    };
}

struct google_oauth_user_t::implementation_t
{
    client_t client;
    std::string email, photo, key_name, portrait;
    oauth_tokens_t tokens;
    std::unique_ptr<key_t> key;
    mutable std::mutex mutex;
    explicit implementation_t(const std::string& path, oauth_purpose_t purpose = oauth_purpose_t::drive):client{path,purpose}{}
    auto save(bool select = false) -> void
    {
        save_vault(client,storage_digest(email),json{{"client_id",client.id},{"scope",client.scope()},{"email",email},
            {"photo",photo},{"portrait",portrait},{"dpop_key",key_name},{"access",tokens.access},
            {"refresh",tokens.refresh},{"nonce",tokens.nonce},{"expires",tokens.expires}},select);
    }
};
google_oauth_user_t::google_oauth_user_t(std::unique_ptr<implementation_t> impl):m_impl{std::move(impl)}{}
google_oauth_user_t::~google_oauth_user_t() = default;

auto google_oauth_user_t::restore(const std::string& path, std::string expected_email, oauth_purpose_t purpose)
    -> std::shared_ptr<google_oauth_user_t>
{
    auto impl = std::make_unique<implementation_t>(path,purpose);
    const auto directory = vault_directory(impl->client);
    if (expected_email.empty() && !std::filesystem::exists(directory/"active")) return {};
    try
    {
        const auto account = expected_email.empty()?storage_read(directory/"active"):storage_digest(expected_email);
        require(account.size()==64 && account.find_first_not_of("0123456789abcdef")==std::string::npos,"Invalid saved OAuth account.");
        const auto saved = load_vault(impl->client,account);
        impl->email = saved.at("email");
        require(saved.at("client_id")==impl->client.id && saved.at("scope")==impl->client.scope() && storage_digest(impl->email)==account &&
            (expected_email.empty() || expected_email==impl->email),"Saved Google account does not match.");
        impl->photo = saved.value("photo","");
        impl->portrait = saved.value("portrait","");
        impl->key_name = saved.at("dpop_key");
        require(impl->key_name.starts_with("lingogoogol.storage.dpop."),"Invalid saved DPoP key name.");
        impl->key = std::make_unique<key_t>(widen_ascii(impl->key_name),false,true);
        impl->tokens = {saved.at("access"),saved.at("refresh"),saved.at("nonce"),saved.at("expires")};
        require(!impl->email.empty() && !impl->tokens.refresh.empty(),"Incomplete saved OAuth credentials.");
    }
    catch (...) { throw std::runtime_error{"Could not restore Google login; sign in again on this Windows user account."}; }
    return std::shared_ptr<google_oauth_user_t>{new google_oauth_user_t{std::move(impl)}};
}
auto google_oauth_user_t::login(const std::string& path, const std::shared_ptr<request_control_t>& control, oauth_purpose_t purpose)
    -> std::shared_ptr<google_oauth_user_t>
{
    auto impl = std::make_unique<implementation_t>(path,purpose);
    impl->key_name = "lingogoogol.storage.dpop."+storage_digest(random_bytes(32));
    impl->key = std::make_unique<key_t>(widen_ascii(impl->key_name),true,true);
    impl->key->remove_on_close = true;
    const auto verifier = oauth_base64(random_bytes(32));
    const auto state = oauth_base64(random_bytes(32));
    loopback_t loopback;
    const auto url = std::string{auth_endpoint}+"?client_id="+percent(impl->client.id)+
        "&redirect_uri="+percent(loopback.redirect)+"&response_type=code&scope="+percent(impl->client.scope())+
        "&access_type=offline&prompt=consent&code_challenge_method=S256&code_challenge="+
        oauth_base64(sha256(verifier))+"&state="+state;
    // The URL contains only ASCII because every variable component is encoded.
    const auto browser = ShellExecuteW(nullptr,L"open",widen_ascii(url).c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    require(reinterpret_cast<std::intptr_t>(browser)>32,"Could not open the browser for Google login.");
    const auto code = loopback.wait(state,control);
    impl->tokens = exchange(impl->client,*impl->key,"grant_type=authorization_code&code="+percent(code)+
        "&redirect_uri="+percent(loopback.redirect)+"&code_verifier="+percent(verifier),code,{},control);
    const auto profile_url = purpose==oauth_purpose_t::cloud ? "https://www.googleapis.com/oauth2/v2/userinfo" :
        "https://www.googleapis.com/drive/v3/about?fields=user(emailAddress,photoLink)";
    const auto response = request(profile_url,"", {"Authorization: Bearer "+impl->tokens.access},control);
    require(response.status==200,"Could not read the signed-in Google profile.");
    try
    {
        const auto data = json::parse(response.body);
        const auto user = purpose==oauth_purpose_t::cloud ? data : data.at("user");
        impl->email = user.at(purpose==oauth_purpose_t::cloud ? "email" : "emailAddress");
        impl->photo = user.value(purpose==oauth_purpose_t::cloud ? "picture" : "photoLink","");
        if (purpose==oauth_purpose_t::cloud)
            require(user.value("verified_email",false),"Google email address is not verified.");
        require(!impl->email.empty(),"Google profile has no email address.");
    }
    catch (...) { throw std::runtime_error{"Google returned an incomplete user profile."}; }
    impl->save(true);
    impl->key->remove_on_close = false;
    return std::shared_ptr<google_oauth_user_t>{new google_oauth_user_t{std::move(impl)}};
}
auto google_oauth_user_t::saved_accounts(const std::string& path, oauth_purpose_t purpose) -> std::vector<oauth_saved_account_t>
{
    const client_t client{path,purpose};
    std::lock_guard lock{vault_mutex};
    const auto directory = vault_directory(client);
    std::vector<oauth_saved_account_t> accounts;
    if (!std::filesystem::exists(directory)) return accounts;
    for (const auto& entry : std::filesystem::directory_iterator{directory})
    {
        const auto id = entry.path().stem().string();
        if (entry.path().extension()!=".json" || id.size()!=64 ||
            id.find_first_not_of("0123456789abcdef")!=std::string::npos) continue;
        try
        {
            const auto saved = load_vault_locked(client,id);
            const auto email = saved.at("email").get<std::string>();
            require(saved.at("client_id")==client.id && saved.at("scope")==client.scope() &&
                storage_digest(email)==id,"Saved Google account does not match.");
            accounts.push_back({email,oauth_unbase64(saved.value("portrait",""))});
        }
        catch (...) { /* An unreadable vault must not hide the other accounts. */ }
    }
    std::ranges::sort(accounts,{},&oauth_saved_account_t::address);
    return accounts;
}
auto google_oauth_user_t::select(const std::string& path, const std::string& email) -> void
{
    const client_t client{path};
    std::lock_guard lock{vault_mutex};
    const auto id = storage_digest(email);
    require(load_vault_locked(client,id).at("email")==email,"Saved Google account does not match.");
    storage_atomic_write(vault_directory(client)/"active",id);
}
auto google_oauth_user_t::sign_out(const std::string& path, const std::string& email) -> void
{
    const client_t client{path};
    std::lock_guard lock{vault_mutex};
    const auto active = vault_directory(client)/"active";
    // Signing out a background user must preserve another user's selection.
    if (email.empty() || (std::filesystem::exists(active) && storage_read(active)==storage_digest(email)))
        std::filesystem::remove(active);
}
auto google_oauth_user_t::remove(const std::string& path, const std::string& email, oauth_purpose_t purpose) -> void
{
    const client_t client{path,purpose};
    std::lock_guard lock{vault_mutex};
    const auto id = storage_digest(email);
    const auto directory = vault_directory(client);
    const auto saved = load_vault_locked(client,id);
    require(saved.at("email")==email,"Saved Google account does not match.");
    key_t wrapping{crypt_key_name(client,id),false};
    const auto signing_name = saved.at("dpop_key").get<std::string>();
    require(signing_name.starts_with("lingogoogol.storage.dpop."),"Invalid saved DPoP key name.");
    key_t signing{widen_ascii(signing_name),false,true};
    if (std::filesystem::exists(directory/"active") && storage_read(directory/"active")==id)
        std::filesystem::remove(directory/"active");
    std::filesystem::remove(directory/(id+".json"));
    wrapping.remove_on_close = signing.remove_on_close = true;
}
auto google_oauth_user_t::refresh(const std::shared_ptr<request_control_t>& control) -> oauth_tokens_t
{
    std::lock_guard lock{m_impl->mutex};
    auto next = exchange(m_impl->client,*m_impl->key,"grant_type=refresh_token&refresh_token="+
        percent(m_impl->tokens.refresh),{},m_impl->tokens,control);
    const auto old = m_impl->tokens;
    m_impl->tokens = std::move(next);
    try { m_impl->save(); }
    catch (...) { m_impl->tokens = old; throw; }
    return m_impl->tokens;
}
auto google_oauth_user_t::tokens() const -> oauth_tokens_t
{ std::lock_guard lock{m_impl->mutex}; return m_impl->tokens; }
auto google_oauth_user_t::address() const -> std::string { return m_impl->email; }
auto google_oauth_user_t::credential_id() const -> std::string
{ return storage_digest(m_impl->client.id+":"+m_impl->key_name); }
auto google_oauth_user_t::storage_id() const -> std::string
{ return "appdata:"+storage_digest(m_impl->client.id)+":"+m_impl->email; }
auto google_oauth_user_t::picture() const -> std::string { return m_impl->photo; }
auto google_oauth_user_t::download_picture(const std::shared_ptr<request_control_t>& control) const -> std::string
{
    if (!m_impl->photo.starts_with("https://")) return {};
    try
    {
        // The public photo URL gets no Authorization header or OAuth credentials.
        const auto response = request(m_impl->photo,"",{},control);
        if (response.status==200)
        {
            // Merge only the cached image into the latest envelope. A photo
            // finishing after token rotation must not restore stale tokens.
            const auto id = storage_digest(m_impl->email);
            try
            {
                std::lock_guard lock{m_impl->mutex};
                auto saved = load_vault(m_impl->client,id);
                if (saved.at("dpop_key")==m_impl->key_name && response.body.size()<=256*1024)
                {
                    // Bound the optional cache inside the 1 MiB token envelope.
                    // Failed caching must not poison later token refresh saves.
                    const auto portrait = oauth_base64(response.body);
                    saved["portrait"] = portrait;
                    save_vault(m_impl->client,id,saved,false);
                    m_impl->portrait = portrait;
                }
            }
            catch (...) { /* The portrait remains usable when caching fails. */ }
            return response.body;
        }
    }
    catch (...) { }
    return {};
}

auto google_oauth_user_t::cloud_request(const std::string& method, const std::string& url,
    const std::string& body, const std::shared_ptr<request_control_t>& control) -> oauth_response_t
{
    require(m_impl->client.purpose==oauth_purpose_t::cloud,"Cloud authorization is required.");
    require(url.starts_with("https://cloudresourcemanager.googleapis.com/") ||
        url.starts_with("https://serviceusage.googleapis.com/") ||
        url.starts_with("https://iam.googleapis.com/"),"Unexpected Google Cloud endpoint.");
    for (int attempt{}; attempt<2; ++attempt)
    {
        auto token = tokens();
        if (token.expires<=std::time(nullptr)+60) token = refresh(control);
        const auto response = request(url,body,{"Authorization: Bearer "+token.access,
            "Content-Type: application/json; charset=UTF-8"},control,method);
        if (response.status!=401 || attempt) return {response.status,response.body};
        refresh(control);
    }
    throw std::runtime_error{"Google Cloud authorization failed."};
}
