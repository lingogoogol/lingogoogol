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
    auto roundtrip(const std::string& input, const std::string& encoding, bool compress) -> void
    {
        auto name = nlohmann::json{{"parent","chunk"},{"parent_account","owner"}};
        const auto content = content_blob_encode(input, encoding, compress, name);
        check(content_blob_decode(blob(name,content)) == input, "Content roundtrip failed");
        const auto wire = blob(name,content).dump();
        check(content_blob_decode(nlohmann::json::parse(wire)) == input, "UTF-8 JSON roundtrip failed");
        check(content_blob_decode(nlohmann::json::parse(blob(name,content).dump(-1,' ',true))) == input,
              "Escaped Unicode JSON roundtrip failed");
        check(content.size() / (encoding == "cjk14" ? 3 : 1) <= 25000, "Character limit exceeded");
        check(name.contains("e") == (encoding == "cjk14"), "Encoding tag mismatch");
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
        check(settings.content_encoding == "base64url" && !settings.content_zstd, "Wrong settings defaults");
        settings.content_encoding = "cjk14";
        settings.content_zstd = true;
        settings.save();
        settings = storage_settings_t::load();
        check(settings.content_encoding == "cjk14" && settings.content_zstd, "Settings did not persist");
        auto journal = task_journal_t::create({{"kind","upload-file"},
            {"content_encoding",settings.content_encoding},{"content_zstd",settings.content_zstd}});
        const auto id = journal->path();
        journal.reset();
        settings.content_encoding = "base64url";
        settings.content_zstd = false;
        settings.save();
        const auto restored = task_journal_t::restore();
        check(restored.size() == 1 && restored.front()->path() == id, "Task journal was not restored");
        const auto intent = restored.front()->data();
        check(intent.at("content_encoding") == "cjk14" && intent.at("content_zstd") == true &&
              content_bytes_per_blob(intent.at("content_encoding").get<std::string>()) == 43750,
              "Saved task format changed with settings");
        auto legacy = task_journal_t::create({{"kind","upload-file"}});
        check(legacy->data().value("content_encoding",std::string{"base64url"}) == "base64url" &&
              !legacy->data().value("content_zstd",false), "Legacy journal defaults changed");
        settings.content_encoding = "invalid";
        reject([&] { settings.save(); });
        storage_atomic_write("settings.json",R"({"content_encoding":"invalid"})");
        reject([] { storage_settings_t::load(); });
        storage_atomic_write("settings.json",R"({"content_zstd":"true"})");
        reject([] { storage_settings_t::load(); });
        storage_atomic_write("settings.json","{}");
        settings = storage_settings_t::load();
        check(settings.content_encoding == "base64url" && !settings.content_zstd, "Legacy settings defaults changed");
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
            check(cjk14_decode(cjk14_encode(input),size) == input, "CJK14 roundtrip failed");
            for (const auto& encoding : {std::string{"base64url"},std::string{"cjk14"}})
                for (bool compress : {false,true})
                {
                    roundtrip(input,encoding,compress);
                    roundtrip(std::string(size,'x'),encoding,compress);
                }
        }
        check(cjk14_encode(std::string(7,'\0')) == std::string("\xe4\xb8\x80\xe4\xb8\x80\xe4\xb8\x80\xe4\xb8\x80"),
              "Big endian zero vector mismatch");
        check(cjk14_encode(std::string(7,'\xff')) == std::string("\xe8\xb7\xbf\xe8\xb7\xbf\xe8\xb7\xbf\xe8\xb7\xbf"),
              "Big endian all-ones vector mismatch");
        check(cjk14_encode(std::string{"\x01\x00\x00\x00\x00\x00\x00",7}) ==
              std::string("\xe4\xb9\x80\xe4\xb8\x80\xe4\xb8\x80\xe4\xb8\x80"),
              "Big endian ordering mismatch");
        for (const auto& encoding : {std::string{"base64url"},std::string{"cjk14"}})
        {
            const auto capacity = content_bytes_per_blob(encoding);
            check(capacity == (encoding == "cjk14" ? 43750 : 18750), "Wrong slice capacity");
            std::string input(capacity,'\0');
            for (auto& c : input) c = static_cast<char>(random() & 255);
            for (bool compress : {false,true})
            {
                roundtrip(input,encoding,compress);
                roundtrip(std::string(capacity,'A'),encoding,compress);
            }
            auto name = nlohmann::json::object();
            content_blob_encode(input,encoding,true,name);
            check(!name.contains("c"), "Incompressible data must fall back to raw");
            reject([&] { content_blob_encode(input+'x',encoding,false,name); });
            const auto packed = content_blob_encode(std::string(capacity,'A'),encoding,true,name);
            check(name.contains("c") == content_zstd_available(), "Missing zstd tag");
            if (content_zstd_available())
            {
                auto damaged = name;
                damaged["r"] = capacity - 1;
                reject([&] { content_blob_decode(blob(damaged,packed)); });
                auto corrupt = packed;
                corrupt[0] ^= 1;
                reject([&] { content_blob_decode(blob(name,corrupt)); });
            }
        }
        // Every alphabet value, including both endpoints, must survive UTF-8.
        for (unsigned value{}; value < 16384; ++value)
        {
            std::string input(7,'\0');
            input[0] = static_cast<char>(value >> 6);
            input[1] = static_cast<char>((value & 63) << 2);
            check(cjk14_decode(cjk14_encode(input),7) == input, "Alphabet mismatch");
        }
        reject([] { cjk14_decode("ascii",1); });
        reject([] { cjk14_decode("\xe4\xb7\xbf",1); }); // U+4DFF
        reject([] { cjk14_decode("\xe8\xb8\x80",1); }); // U+8E00
        reject([] { cjk14_decode("\xe4\x38\x80",1); }); // Bad continuation
        reject([] { cjk14_decode("\xe4\xb8\x81",1); }); // Nonzero padding
        reject([] { cjk14_decode(cjk14_encode("abc"),2); });
        reject([] { cjk14_decode(cjk14_encode("abc"),4); });
        reject([] { cjk14_decode("",43751); });
        auto name = nlohmann::json{{"e","cjk14"},{"n",1}};
        for (auto value : {nlohmann::json(-1),nlohmann::json(1.0),nlohmann::json("1")})
        {
            name["n"] = value;
            reject([&] { content_blob_decode(blob(name,cjk14_encode("x"))); });
        }
        reject([] { content_blob_decode(blob({{"e","unknown"}},"")); });
        reject([] { content_blob_decode(blob({{"c","unknown"}},"")); });
        reject([] { content_bytes_per_blob("unknown"); });
        reject([] { content_blob_decode(blob({{"e","cjk14"}},cjk14_encode("x"))); });
        reject([] { content_blob_decode(blob({{"e","cjk14"},{"n",43751}},"")); });
        if (!content_zstd_available())
            reject([] { content_blob_decode(blob({{"c","zstd"},{"r",1}},base64url_encode("x"))); });
        check(content_blob_decode({{"description",base64url_encode("legacy")}}) == "legacy",
              "Old description-only response is unreadable");
        std::cout << "PASS: settings/task persistence, lengths 0..200, full blobs, all 16384 characters, UTF-8/escaped JSON, legacy, tags, raw fallback, malformed inputs; zstd="
                  << content_zstd_available() << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
