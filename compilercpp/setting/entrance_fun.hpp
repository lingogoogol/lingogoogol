#ifndef COMPILERCPP_SETTING_ENTRANCE_FUN
#define COMPILERCPP_SETTING_ENTRANCE_FUN

#include "../lib/.hpp"

#include "main_def.hpp"
#include "member.hpp"

class setting_entrance_fun_t: public setting_member_t {
public:
    constexpr static setting_t::type::type_t type{ setting_t::type::entrance_fun };
    std::wstring m_data{};

    static auto get(std::istream& src) -> setting_entrance_fun_t* {
        setting_entrance_fun_t* out{ new setting_entrance_fun_t{} };
        out->m_data = get_stringw(src);
        return out;
    }

    auto pad(std::ostream& dest) -> void override {
        pad_stringw(dest, m_data);
        return;
    }

    auto get_data_ptr() -> std::wstring* {
        return &m_data;
    }

    static auto get_data_default() -> std::wstring {
        return L"";
    }

    auto set_data(std::wstring in) -> void {
        m_data = in;
        return;
    }
};

#endif