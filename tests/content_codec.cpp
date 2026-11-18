#include <nlohmann/json.hpp>
import std;
import storage.encode;
import storage.state;

namespace
{
    auto check(bool value, const char* message) -> void
    {
        if (!value) throw std::runtime_error{message};
    }
    auto reject(auto action) -> void
    {
        bool rejected{};
        try { action(); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Malformed input was accepted");
    }
    auto blob(const nlohmann::json& name, const std::string& content) -> nlohmann::json
    {
        return {{"name",name.dump()},{"description",content}};
    }
    auto roundtrip(const std::string& input, bool compress) -> void
    {
        auto name = nlohmann::json{{"parent","chunk"},{"parent_account","owner"}};
        const auto content = content_blob_encode(input, compress, name);
        check(content_blob_decode(blob(name,content)) == input, "Content roundtrip failed");
        const auto wire = blob(name,content).dump();
        check(content_blob_decode(nlohmann::json::parse(wire)) == input, "UTF-8 JSON roundtrip failed");
        check(content_blob_decode(nlohmann::json::parse(blob(name,content).dump(-1,' ',true))) == input,
              "Escaped Unicode JSON roundtrip failed");
        check(content.size() <= 25000, "Character limit exceeded");
        check(!name.contains("e") && !name.contains("n"), "Unexpected encoding tags");
        if (name.contains("c"))
            check(compress && content_zstd_available() && name.at("c") == "zstd" &&
                  name.at("r") == input.size(), "Compression tags mismatch");
    }

    auto persistence() -> void
    {
        const auto previous = std::filesystem::current_path();
        const auto scratch = previous / ("content-codec-fixture-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(scratch), "Cannot create isolated test directory");
        struct cleanup_t
        {
            std::filesystem::path previous, scratch;
            ~cleanup_t()
            {
                std::filesystem::current_path(previous);
                std::filesystem::remove_all(scratch);
            }
        } cleanup{previous,scratch};
        std::filesystem::current_path(scratch);
        auto settings = storage_settings_t::load();
        check(!settings.content_zstd, "Wrong settings defaults");
        settings.content_zstd = true;
        settings.save();
        settings = storage_settings_t::load();
        check(settings.content_zstd, "Settings did not persist");
        auto journal = task_journal_t::create({{"kind","upload-file"},
            {"content_zstd",settings.content_zstd}});
        const auto id = journal->path();
        journal.reset();
        settings.content_zstd = false;
        settings.save();
        const auto restored = task_journal_t::restore();
        check(restored.size() == 1 && restored.front()->path() == id, "Task journal was not restored");
        const auto intent = restored.front()->data();
        check(intent.at("content_zstd") == true, "Saved task compression changed with settings");
        auto legacy = task_journal_t::create({{"kind","upload-file"}});
        check(!legacy->data().value("content_zstd",false), "Legacy journal defaults changed");
        storage_atomic_write("settings.json",R"({"content_zstd":"true"})");
        reject([] { storage_settings_t::load(); });
        storage_atomic_write("settings.json","{}");
        settings = storage_settings_t::load();
        check(!settings.content_zstd, "Legacy settings defaults changed");
    }
}

auto main() -> int
{
    try
    {
        persistence();
        std::mt19937 random{0x14c0de};
        for (std::size_t size{}; size <= 200; ++size)
        {
            std::string input(size, '\0');
            for (auto& c : input) c = static_cast<char>(random() & 255);
            for (bool compress : {false,true})
            {
                roundtrip(input,compress);
                roundtrip(std::string(size,'x'),compress);
            }
        }
        {
            constexpr std::size_t capacity{18750};
            std::string input(capacity,'\0');
            for (auto& c : input) c = static_cast<char>(random() & 255);
            for (bool compress : {false,true})
            {
                roundtrip(input,compress);
                roundtrip(std::string(capacity,'A'),compress);
            }
            auto name = nlohmann::json::object();
            content_blob_encode(input,true,name);
            check(!name.contains("c"), "Incompressible data must fall back to raw");
            reject([&] { content_blob_encode(input+'x',false,name); });
            const auto packed = content_blob_encode(std::string(capacity,'A'),true,name);
            check(name.contains("c") == content_zstd_available(), "Missing zstd tag");
            if (content_zstd_available())
            {
                auto damaged = name;
                damaged["r"] = capacity - 1;
                reject([&] { content_blob_decode(blob(damaged,packed)); });
                const auto frame = base64url_decode(packed);
                reject([&] { content_blob_decode(blob(name,base64url_encode(frame + "x"))); });
                reject([&] { content_blob_decode(blob(name,base64url_encode(frame + frame))); });
                reject([&] { content_blob_decode(blob(name,base64url_encode(frame.substr(0,frame.size()-1)))); });
                auto corrupt = packed;
                corrupt[0] ^= 1;
                reject([&] { content_blob_decode(blob(name,corrupt)); });
            }
        }
        reject([] { content_blob_decode(blob({{"e","unknown"}},"")); });
        reject([] { content_blob_decode(blob({{"c","unknown"}},"")); });
        reject([] { content_blob_decode(blob(nlohmann::json::object(),std::string(25001,'A'))); });
        reject([] { content_blob_decode(blob(nlohmann::json::object(),"!")); });
        for (auto value : {nlohmann::json(-1),nlohmann::json(1.0),nlohmann::json("1"),nlohmann::json(18751u)})
            reject([&] { content_blob_decode(blob({{"c","zstd"},{"r",value}},"")); });
        reject([] { content_blob_decode(blob({{"c","zstd"}},"")); });
        if (!content_zstd_available())
            reject([] { content_blob_decode(blob({{"c","zstd"},{"r",1}},base64url_encode("x"))); });
        check(content_blob_decode({{"description",base64url_encode("legacy")}}) == "legacy",
              "Old description-only response is unreadable");
        std::cout << "PASS: settings/task persistence, lengths 0..200, full 18750-byte blobs, JSON, legacy, tags, raw fallback, malformed inputs; zstd="
                  << content_zstd_available() << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
