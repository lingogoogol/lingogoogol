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

    auto log_file(const std::string& in) -> void
    {
        if (get_logfile_path() == "") {
            get_logfile_string() += in;
        }
        else {
            std::ofstream stream{ create_ofstream_preserve(get_logfile_path()) };
            stream.seekp(0, std::ios_base::end);
            for (auto i : in) {
                stream.put(i);
            }
        }
        return;
    }

    auto init_logfile(const std::string& path) -> bool
    {
        get_logfile_path() = path;
        log_file(get_logfile_string());
        return true;
    }

    auto log_console(const std::string& in) -> void
    {
        OutputDebugStringW(to_wstring(in).data());
        return;
    }
}
