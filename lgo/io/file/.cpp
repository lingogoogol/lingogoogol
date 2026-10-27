module lgo.io.file;

import std;

import external.Windows;

import lgo.dev.error;
import lgo.container.str;

namespace lgo
{
    auto create_ofstream
    (
        const std::string& path,
        std::ios_base::openmode mode
    )
    -> std::ofstream
    {
        std::ofstream out{};
        out.open(path, mode);
        if (!out.fail())
        {
            out.exceptions(file_exception_mask);
        }
        return out;
    }

    auto create_ofstream_destroy(const std::string& path) -> std::ofstream
    {
        return create_ofstream(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    }

    auto create_ofstream_preserve(const std::string& path) -> std::ofstream
    {
        std::ofstream temp{ create_ofstream(path, std::ios_base::out | std::ios_base::app) };
        temp.close();
        return create_ofstream(path, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
    }

    auto create_ifstream
    (
        const std::string& path,
        std::ios_base::openmode mode
    )
    -> std::ifstream
    {
        std::ifstream out{};
        out.open(path, mode);
        if (!out.fail())
        {
            out.exceptions(file_exception_mask);
        }
        return out;
    }

    auto create_fstream
    (
        const std::string& path,
        std::ios_base::openmode mode
    )
    -> std::fstream
    {
        std::fstream out{};
        out.open(path, std::ios_base::out | std::ios_base::app);
        out.close();
        out.open(path, mode);
        if (out.rdstate() != std::ios_base::goodbit)
        {
            throw internal_error_t{ "create_fstream" };
        }
        out.exceptions(file_exception_mask);
        return out;
    }

    auto get_file(const std::string& path) -> std::vector<unsigned char>
    {
        std::vector<unsigned char> out{};
        std::ifstream file{ create_ifstream(path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate) };
        out.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
        return out;
    }

    auto get_file_str(const std::string& path) -> std::string
    {
        std::string out{};
        std::ifstream file{ create_ifstream(path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate) };
        out.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(out.data(), static_cast<std::streamsize>(out.size()));
        return out;
    }

    auto get_logfile_path() -> std::string&
    {
        static std::string logfile_path{};
        return logfile_path;
    }

    auto get_logfile_string() -> std::string&
    {
        static std::string logfile_string{};
        return logfile_string;
    }

    namespace
    {
        struct logfile_t
        {
            std::mutex mutex;
            std::string path;
            std::ofstream stream;
        };

        auto logfile() -> logfile_t&
        {
            static logfile_t state;
            return state;
        }
    }

    auto log_file(const std::string& in) -> void
    {
        auto& state{ logfile() };
        std::unique_lock lock{ state.mutex };
        if (get_logfile_path().empty())
        {
            get_logfile_string() += in;
            return;
        }
        try
        {
            // Reuse an append handle: object lifecycle traces can contain many
            // thousands of entries. Flush each message to retain crash diagnostics.
            if (state.path != get_logfile_path() || !state.stream.is_open())
            {
                state.stream.close();
                state.stream.clear();
                state.path = get_logfile_path();
                state.stream.open(state.path, std::ios::binary | std::ios::app);
            }
            state.stream.write(in.data(), static_cast<std::streamsize>(in.size()));
            state.stream.flush();
            if (!state.stream) throw std::runtime_error{ "Cannot write the log file: " + state.path };
        }
        catch (const std::exception& error)
        {
            // A full disk or invalid path must not trap rendering/native callbacks
            // in an infinite retry loop. The next message may retry opening once.
            state.stream.close();
            state.stream.clear();
            std::cerr << "log_file: " << error.what() << '\n';
        }
    }

    auto init_logfile(const std::string& path) -> bool
    {
        const auto parent{ std::filesystem::path{ path }.parent_path() };
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::ofstream next{path, std::ios::binary | std::ios::app};
        if (!next)
            throw std::runtime_error{"Cannot open log file: " + path};
        auto &state{logfile()};
        std::lock_guard lock{state.mutex};
        get_logfile_path() = path;
        state.path = path;
        state.stream = std::move(next);
        state.stream << get_logfile_string();
        state.stream.flush();
        get_logfile_string().clear();
        return true;
    }

    auto tab(std::uint32_t count) -> std::string
    {
        return std::string( 4 * count, ' ' );
    }

    auto log_console(const std::string& in) -> void
    {
        OutputDebugStringW(to_wstring(in).data());
        return;
    }
}
