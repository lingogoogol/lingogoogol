export module lgo.io.file;

import std;

export namespace lgo {
    constexpr inline std::ios_base::iostate file_exception_mask
    { std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit };

    auto create_ofstream(const std::string& path, std::ios_base::openmode mode) -> std::ofstream;
    auto create_ofstream_destroy(const std::string& path) -> std::ofstream;
    auto create_ofstream_preserve(const std::string& path) -> std::ofstream;
    auto create_ifstream(const std::string& path
    , std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary) -> std::ifstream;
    auto create_fstream(const std::string& path
    , std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary) -> std::fstream;
    
    auto get_file(const std::string& path) -> std::vector<unsigned char>;
    auto get_logfile_path() -> std::string&;
    auto get_logfile_string() -> std::string&;
    auto log_file(const std::string& in) -> void;
    auto init_logfile(const std::string& path) -> bool;
    auto log_console(const std::string& in) -> void;

    template<typename t_in>
    auto log_ref_count(t_in in) -> void {
        in->AddRef();
        log_console(std::to_string(in->Release()));
        return;
    }
}
