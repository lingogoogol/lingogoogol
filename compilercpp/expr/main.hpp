#ifndef COMPILERCPP_EXPR_MAIN
#define COMPILERCPP_EXPR_MAIN

#include <cstdint>
#include <iostream>

#include "../stmt/pv.hpp"

#include "pv.hpp"

class expr: public stmt_pv {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        init
    };
private:
    type_t m_type{};
    expr_pv* m_data{};
public:
    expr(std::istream& src) {}

    auto type() -> type_t {
        return m_type;
    }
};

#endif