#ifndef COMPILERCPP_SETTING_MAIN_DEF
#define COMPILERCPP_SETTING_MAIN_DEF

#include <string>
#include <fstream>
#include <map>

#include "member.hpp"

class setting_t {
private:
    static const std::wstring default_path;

    std::wstring m_path{};
    std::fstream m_file{};
public:
    class type {
    public:
        using type_t = std::uint8_t;
        enum: type_t {
            logfile_path,
            source_path,
            lib_path,
            exe_path,
            entrance_fun
        };
    };

    std::map<type::type_t, setting_member_t*> m_data{};

    setting_t() = default;
    ~setting_t();

    auto parse(std::wstring path) -> void;
    template<typename t_member>
    auto get_member() -> decltype(t_member::get_data_default());
    template<typename t_member, typename t_in>
    auto set_member(t_in&& in) -> void;

    auto save() -> void;
};

const std::wstring setting_t::default_path{ L"setting" };

#endif