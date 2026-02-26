#ifndef COMPILERCPP_DATA_TYPE_REF_BI
#define COMPILERCPP_DATA_TYPE_REF_BI

#include <cstdint>
#include <iostream>

#include "../lib/.hpp"

#include "ref_pv.hpp"

class data_type_ref_bi: public data_type_ref_pv {
public:
    using size_t = std::uint8_t;

    enum: size_t {
        byte = 1,
        word = 2,
        dword = 3,
        qword = 4,
        oword = 5
    };
private:
    size_t m_size{};
public:
    data_type_ref_bi(std::istream& src) {
        m_size = get_integer<size_t>(src);
    }

    virtual ~data_type_ref_bi() = default;

    auto type() const -> size_t {
        return m_size;
    }
};

#endif