#ifndef LGO_IO_CMD_IMPL
#define LGO_IO_CMD_IMPL

#include "def.hpp"

namespace lgo {
    cmd_type::cmd_type(std::istream& is, std::ostream& os): m_is{ &is }, m_os{ &os } {}

    template<typename u_self, typename u_in>
    auto cmd_type::pad(this u_self&& self, const u_in& in) -> u_self&& {
        (*self.m_os) << strmdc{}.pad_resize(in).sc<std::string>();
        return self.f<u_self>();
    }
}

#endif