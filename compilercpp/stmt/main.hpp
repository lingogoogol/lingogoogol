#ifndef COMPILERCPP_STMT_MAIN
#define COMPILERCPP_STMT_MAIN

#include <cstdint>
#include <iostream>
#include <vector>

#include "pv.hpp"

class stmt {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        expr,
        ret
    };
private:
    type_t m_type{};
    stmt_pv* m_data{};
public:
    stmt(std::istream& src) {}

    auto type() -> type_t {
        return m_type;
    }

    auto compile(std::vector<unsigned char>& dest) const -> void {}
};

#endif