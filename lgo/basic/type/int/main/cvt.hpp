#ifndef LGO_BASIC_TYPE_INT_MAIN_CVT
#define LGO_BASIC_TYPE_INT_MAIN_CVT

#include "def.hpp"

#include "../p/impl.hpp"
#include "../../cha/basic_decl.hpp"
#include "../../cha/literal_decl.hpp"
#include "../../fun/cmp.hpp"
#include "../../../seq/str/cpt/d_cpt.hpp"
#include "../../../seq/io/.hpp"

namespace lgo {
	template<is_basic_intp_cpt t_type>
	template<is_int_cpt u_out, is_str_cpt u_in>
	constexpr auto int_class<t_type>::str_get(const u_in& in) -> u_out {
		return str_get<u_out>(in, int_base<intpc::val_v<U"10">>{});
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt u_out, is_str_cpt u_in, is_int_base_cpt u_base>
	constexpr auto int_class<t_type>::str_get(const u_in& in, u_base) -> u_out {
		u_out out{};
		for (intpc i{ U"0"_intpc }; (i < in.len()).basic(); ++i) {
			if constexpr ((u_base::base() == U"10"_intpc).basic()) {
				out <<= U"4"_intpc;
			}
			else {
				out *= u_base::base();
			}
			out.m_data += to_basic_intp<type>(in[i].basic());
		}
		return out;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt u_in, is_str_cpt u_out>
	constexpr auto int_class<t_type>::str_pad(u_in in, u_out& out) -> void {
		str_pad(in, out, int_base<intpc::val_v<U"10">>{});
		return;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt u_in, is_str_cpt u_out, is_int_base_cpt u_base>
	constexpr auto int_class<t_type>::str_pad(u_in in, u_out& out, u_base) -> void {
		if constexpr ((u_base::base() == U"10"_intpc).basic()) {
			intpc digit_len{ u_in::size(size_content{}) * U"2"_intpc };
			intpc pad_len{ U"1"_intpc + digit_len };
			if ((out.len() < pad_len).basic()) {
				throw pad_not_enough{ pad_len - out.len() };
			}
			
			out[U"0"_intpc] = (in.sign().basic() ? U"+"_chac : U"-"_chac);
			for (intpc i{ U"0"_intpc }; (i < digit_len).basic(); ++i) {
				out[i + U"1"_intpc] = chac{ to_basic_cha<basic_chac>(::lgo::sc<u_in::type>((in.m_data
				>> (u_in::bits() - U"4"_intpc * (i + U"1"_intpc)).basic()) % ::lgo::sc<u_in::type>(0x10))) };
			}
			out.padpos(pad_len);
		}
		else {
			intpc digit{ U"1"_intpc };
			while ((u_base::base().pow(digit - U"1"_intpc) == u_base::base().pow(digit) / u_base::base()).basic()) {
				++digit;
			}
			if ((out.len() < U"1"_intpc + digit).basic()) {
				throw pad_not_enough{ U"1"_intpc + digit - out.len() };
			}

			out[U"0"_intpc] = (in.sign().basic() ? U"+"_chac : U"-"_chac);
			out.padpos(U"1"_intpc + digit);
			for (--digit; (digit != intpc::max()).basic(); --digit) {
				out[digit] = chac{ to_basic_cha<basic_chac>(in.m_data % u_base::base().basic()) };
				in.m_data /= u_base::base().basic();
			}
		}
		return;
	}
}

#endif