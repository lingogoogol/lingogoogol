#ifndef COMPILERCPP_DATA_TYPE_REF_BI
#define COMPILERCPP_DATA_TYPE_REF_BI

#include <cstdint>
#include <iostream>

#include "../lib/.hpp"
#include "../data/bi.hpp"

#include "ref_pv.hpp"

class data_type_ref_bi: public data_type_ref_pv {
private:
    data_bi::size_t m_size{};
public:
    data_type_ref_bi(std::istream& src) {
        m_size = get_integer<data_bi::size_t>(src);
    }

    virtual ~data_type_ref_bi() = default;

    auto type() const -> data_bi::size_t {
        return m_size;
    }
};

#endif