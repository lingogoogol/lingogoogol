#ifndef LGO_BASIC_TYPE_INT_P_BASIC_CVT
#define LGO_BASIC_TYPE_INT_P_BASIC_CVT

#include "basic.hpp"

#include "../../literal_str/basic.hpp"

namespace lgo::inline literal {
    template<is_basic_intp_cpt t_out, is_basic_literal_str_cpt t_in>
    constexpr auto make_basic_intp(t_in in) -> t_out {
        t_out out{};
		for (basic_intpc i{ 0 }; i < in.len(); ++i) {
			out <<= 4;
			out += to_basic_intp<t_out>(in.m_data[i]);
		}
        return out;
    }
}

#endif