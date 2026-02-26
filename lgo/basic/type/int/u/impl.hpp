#ifndef LGO_BASIC_TYPE_INT_U_IMPL
#define LGO_BASIC_TYPE_INT_U_IMPL

#include "def.hpp"

#include "../main/impl.hpp"
#include "../p/basic_cvt.hpp"
#include "../../../seq/io/.hpp"

namespace lgo {
	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_out, is_mem_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::mem_get(const u_in& in) -> u_out {
		if ((in.len() < u_out::size(size_content{})).basic()) {
			throw get_not_enough{ u_out::size(size_content{}) - in.len() };
		}

		u_out out{ U"0"_intpc };
		for (intpc i{ U"0"_intpc }; (i < u_out::size(size_content{})).basic(); ++i) {
			out <<= U"8"_intpc;
			out += in[i];
		}
		in.getpos(u_out::size(size_content{}));
		return out;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_in, is_mem_cpt u_out>
	constexpr auto intu_class<t_type, t_self>::mem_pad(u_in in, u_out& out) -> void {
		mem_pad(in, out, int_little_endian{});
		return;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_in, is_mem_cpt u_out, is_int_endian_cpt u_endian>
	constexpr auto intu_class<t_type, t_self>::mem_pad(u_in in, u_out& out, u_endian) -> void {
		if ((out.len() < u_in::size(size_content{})).basic()) {
			throw pad_not_enough{ u_in::size(size_content{}) - out.len() };
		}
		
		for (intpc i{ U"0"_intpc }; (i < u_in::size(size_content{})).basic(); ++i) {
			if constexpr (is_same_cpt<u_endian, int_big_endian>) {
				out[i] = (in << (i * U"8"_intpc)) >> ((u_in::size(size_content{}) - U"1"_intpc) * U"8"_intpc);
			}
			else if constexpr (is_same_cpt<u_endian, int_little_endian>) {
				out[i] = in.modtz(U"100"_intpc);
				in >>= U"8"_intpc;
			}
			else {
				throw e_other{ U""_strmdc };
			}
		}
		out.padpos(u_in::size(size_content{}));
		return;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<typename t_in>
	constexpr auto intu_class<t_type, t_self>::size(const t_in&) -> intpc
	requires is_size_mem_cpt<t_in> && ((t_in::len() == U"0"_intpc).cpt()
	|| ((t_in::len() == U"1"_intpc).cpt() && is_int_endian_cpt<typename t_in::template type
	<typename suspend_bind<intpc_val_bind, intpc, v<basic_literal_str_0>>::template type<t_in>>>)) {
		return base::size(size_content{});
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in, is_intu_cpt u_out>
	constexpr auto intu_class<t_type, t_self>::sub_overflow_o(this u_self self, u_in in, u_out& out) -> void {
		out.m_data = ::lgo::sc<typename u_out::type>(self.m_data) - ::lgo::sc<typename u_out::type>(in.m_data);
		return;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self>
	constexpr auto intu_class<t_type, t_self>::operator~(this u_self self) -> u_self {
		return u_self{ ~self.m_data };
	}
	
	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self>
	constexpr auto intu_class<t_type, t_self>::bnot_a(this u_self& self) -> u_self& {
		self.m_data = ~self.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator&(this u_self self, u_in in) -> band_ret<u_self, u_in> {
		using ret_type = band_ret<u_self, u_in>::intu_class::type;
		return band_ret<u_self, u_in>{ ::lgo::sc<ret_type>(self.m_data) & ::lgo::sc<ret_type>(in.m_data) };
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator&=(this u_self& self, u_in in) -> u_self& {
		self.m_data &= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator|(this u_self self, u_in in) -> bor_ret<u_self, u_in> {
		using ret_type = bor_ret<u_self, u_in>::intu_class::type;
		return bor_ret<u_self, u_in>{ ::lgo::sc<ret_type>(self.m_data) | ::lgo::sc<ret_type>(in.m_data) };
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator|=(this u_self& self, u_in in) -> u_self& {
		self.m_data |= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator^(this u_self self, u_in in) -> bxor_ret<u_self, u_in> {
		using ret_type = bxor_ret<u_self, u_in>::intu_class::type;
		return bxor_ret<u_self, u_in>{ ::lgo::sc<ret_type>(self.m_data) ^ ::lgo::sc<ret_type>(in.m_data) };
	}

	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
	template<is_intu_cpt u_self, is_intu_cpt u_in>
	constexpr auto intu_class<t_type, t_self>::operator^=(this u_self& self, u_in in) -> u_self& {
		self.m_data ^= in.m_data;
		return self;
	}
	
	template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
    template<basic_literal_str u_in>
	constexpr auto intu_class<t_type, t_self>::make() -> t_self {
		return t_self{ make_basic_intp<t_self::intu_class::type>(u_in) };
	}
}

#endif