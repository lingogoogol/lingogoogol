#ifndef COMPILERCPP_STMT_MAIN
#define COMPILERCPP_STMT_MAIN

#include <cstdint>
#include <iostream>
#include <vector>

#include "../lib/.hpp"
#include "../instr/.hpp"

#include "pv.hpp"

class stmt {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        expr,
        instr,
        ret
    };
private:
    type_t m_type{};
    stmt_pv* m_data{};
public:
    stmt(std::istream& src) {
        m_type = get_integer<type_t>(src);
        switch (m_type) {
        case instr:
            m_data = new ::instr{ src };
            break;
        default:
            log_file("stmt type\n");
        }
        return;
    }

    auto type() -> type_t {
        return m_type;
    }

    auto compile(std::vector<unsigned char>& dest) const -> void {}
};

#endif