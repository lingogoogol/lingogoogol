#include <nlohmann/json.hpp>
#include "storage/blob_format.hpp"
#include "storage/base64url.hpp"
import std;
import storage.encode;
import storage.state;

namespace
{
    auto check(bool value, const char* message) -> void
    { if (!value) throw std::runtime_error{message}; }
    auto reject(auto action) -> void
    {
        bool rejected{};
        try { action(); } catch (const std::exception&) { rejected=true; }
        check(rejected,"Malformed input was accepted");
    }
    auto blob(const nlohmann::json& name, const std::string& content) -> nlohmann::json
    { return {{"name",name.dump()},{"description",content}}; }
    auto roundtrip(const std::string& input) -> void
    {
        auto name = nlohmann::json{{"parent","chunk"},{"parent_account","owner"}};
        const auto content = content_blob_encode(input,name);
        check(name.at("version")==storage_format::current_version,"Missing current version");
        check(!name.contains("e") && !name.contains("n") && !name.contains("c") && !name.contains("r"),"Unexpected tags");
        check(content.size()<=25000,"Description limit exceeded");
        check(content_blob_decode(blob(name,content))==input,"Roundtrip failed");
        name["version"]=1;
        check(content_blob_decode(nlohmann::json::parse(blob(name,content).dump()))==input,"Version 1 read failed");
        const auto frame=base64url_decode(content);
        check(!frame.empty(),"Empty input still requires a complete zstd frame");
        check(content_blob_decode(blob(nlohmann::json::object(),content))==frame,
              "Unversioned content was incorrectly decompressed");
        check(content_blob_decode(blob({{"e","unused"},{"n",-1},{"c","zstd"},{"r",input.size()}},content))==frame,
              "Discarded format tags changed unversioned decoding");
        reject([&]{content_blob_decode(blob({{"version",2}},base64url_encode(frame+"x")));});
        reject([&]{content_blob_decode(blob({{"version",2}},base64url_encode(frame+frame)));});
        reject([&]{content_blob_decode(blob({{"version",2}},base64url_encode(frame.substr(0,frame.size()-1))));});
    }
}

auto main() -> int
{
    try
    {
        std::mt19937 random{0x14c0de};
        for (std::size_t size=0;size<=200;++size)
        {
            std::string input(size,'\0');
            for(auto& byte:input) byte=static_cast<char>(random()&255);
            roundtrip(input); roundtrip(std::string(size,'x'));
        }
        // The Android build encodes with storage/base64url.hpp; both directions
        // must interoperate with this platform's base64url_encode/decode.
        for (std::size_t size=0;size<=1000;++size)
        {
            std::string input(size,'\0');
            for(auto& byte:input) byte=static_cast<char>(random()&255);
            const auto encoded=base64url_encode(input);
            check(storage_base64url::decode(encoded)==input,"Portable decoder rejected native Base64URL");
            check(base64url_decode(storage_base64url::encode(input))==input,"Native decoder rejected portable Base64URL");
            auto trimmed=storage_base64url::encode(input);
            while(!trimmed.empty() && trimmed.back()=='=') trimmed.pop_back();
            check(storage_base64url::decode(trimmed)==input,"Portable decoder rejected unpadded Base64URL");
        }
        check(!storage_base64url::decode("!") && !storage_base64url::decode("A") && !storage_base64url::decode("QQ=A"),
              "Portable decoder accepted malformed Base64URL");
        std::string full(storage_format::content_capacity,'\0');
        for(auto& byte:full) byte=static_cast<char>(random()&255);
        roundtrip(full); roundtrip(std::string(full.size(),'A'));
        auto name=nlohmann::json::object();
        reject([&]{content_blob_encode(full+'x',name);});
        // Legacy tasks keep both their 18,750-byte offsets and wire format.
        std::string legacy(storage_format::legacy_content_capacity,'x');
        auto old_name=nlohmann::json::object();
        auto old_payload=content_blob_encode(legacy,old_name,0);
        check(!old_name.contains("version") && !old_name.contains("c"),"Legacy upload changed format");
        check(base64url_decode(old_payload)==legacy,"Version 0 upload was compressed");
        check(content_blob_decode(blob(old_name,old_payload))==legacy,"Version 0 roundtrip failed");
        check(content_blob_decode({{"description",base64url_encode("legacy")}})=="legacy","Old checkpoint read failed");
        for(const auto& invalid:{nlohmann::json(-1),nlohmann::json(3),nlohmann::json(1.5),nlohmann::json("2"),nlohmann::json(true)})
            reject([&]{content_blob_decode(blob({{"version",invalid}},base64url_encode("x")));});
        reject([]{content_blob_decode(blob({{"version",2}},base64url_encode("not zstd")));});
        reject([]{content_blob_decode(blob(nlohmann::json::object(),"!"));});
        reject([]{content_blob_decode(blob(nlohmann::json::object(),std::string(25001,'A')));});
        reject([]{storage_format::minimum({{"version",2},{"version_min",3}});});
        const auto previous=std::filesystem::current_path();
        const auto scratch=std::filesystem::temp_directory_path()/(
            "versioned-journal-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(scratch);
        struct cleanup_t{std::filesystem::path previous,scratch;~cleanup_t(){std::filesystem::current_path(previous);std::filesystem::remove_all(scratch);}} cleanup{previous,scratch};
        std::filesystem::current_path(scratch);
        const auto timestamp=storage_format::now();
        auto journal=task_journal_t::create({{"kind","upload-file"},{"blob_version",storage_format::current_version},{"time",timestamp}});
        const auto path=journal->path();journal.reset();
        auto restored=task_journal_t::restore();
        check(restored.size()==1 && restored.front()->path()==path && restored.front()->data().at("blob_version")==2 &&
              restored.front()->data().at("time")==timestamp,"Restored upload lost format or timestamp");
        restored.front()->finish();restored.clear();
        std::cout<<"PASS: versions 0/1/2, mandatory zstd, full incompressible slices, empty frames, uncompressed legacy uploads/checkpoints, malformed frames and versions.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
