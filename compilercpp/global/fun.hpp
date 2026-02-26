#ifndef COMPILERCPP_GLOBAL_FUN
#define COMPILERCPP_GLOBAL_FUN

#include <string>
#include <vector>
#include <iostream>

#include "pv.hpp"
#include "../stmt/.hpp"
#include "../lib/.hpp"

class fun: public global_pv {
private:
    std::string m_name{};
    std::vector<stmt> m_stmt{};
public:
    fun(std::istream& src) {
        m_name = get_string(src);
        std::uint64_t stmt_count{ get_integer<std::uint64_t>(src) };
        for (int i{ 0 }; i < stmt_count; ++i) {
            m_stmt.push_back(stmt{ src });
        }
        return;
    }

    virtual ~fun() = default;

    auto name() const -> const std::string& {
        return m_name;
    }

    auto compile(std::vector<unsigned char>& dest) const -> void {
        for (std::uint64_t i{ 0 }; i < m_stmt.size(); ++i) {
            m_stmt[i].compile(dest);
        }
        return;
    }
};

#endif