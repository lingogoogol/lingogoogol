#ifndef LGO_BASIC_TYPE_INT_MAIN_IMPL
#define LGO_BASIC_TYPE_INT_MAIN_IMPL

#include "life.hpp"

#include "../p/life.hpp"
#include "../s/life.hpp"
#include "../../boo/.hpp"
#include "../../../e/.hpp"

namespace lgo {
	template<is_basic_intp_cpt t_type>
	template<typename t_in>
	constexpr auto int_class<t_type>::size(t_in) -> intpc
	requires is_one_of_cpt<t_in, size_stack, size_content> {
		return intpc{ sizeof(type) };
	}

	template<is_basic_intp_cpt t_type>
    constexpr auto int_class<t_type>::bits() -> intpc {
        return intpc{ sizeof(type) * 8 };
    }

	template<is_basic_intp_cpt t_type>
    constexpr auto int_class<t_type>::bytes() -> intpc {
        return intpc{ sizeof(type) };
    }
	
	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator<=>(this t_self self, t_in in) -> intsc {
        if ((self.sign() && !in.sign()).basic()) {
            return (self.m_data | in.m_data) == 0 ? intsc{ 0 } : intsc{ 1 };
        }
        else if ((!self.sign() && in.sign()).basic()) {
            return (self.m_data | in.m_data) == 0 ? intsc{ 0 } : intsc{ -1 };
        }
        else if ((self.sign() && in.sign()).basic()) {
		    return self.m_data < in.m_data ? intsc{ -1 } : self.m_data == in.m_data ? intsc{ 0 } : intsc{ 1 };
        }
        else {
		    return self.m_data < in.m_data ? intsc{ 1 } : self.m_data == in.m_data ? intsc{ 0 } : intsc{ -1 };
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator+(this t_self self, t_in in) -> add_ret<t_self, t_in> {
		if ((self.sign() != in.sign()).basic()) {
			return self - (-in);
		}

		using ret = add_ret<t_self, t_in>;
		using ret_type = ret::int_class::type;
		ret abs{ ret_type{ self.m_data } + ret_type{ in.m_data } };
		if constexpr (is_ints_cpt<ret>) {
			if ((!self.sign()).basic()) {
				return -abs;
			}
		}
		return abs;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator+=(this t_self& self, t_in in) -> t_self& {
		if ((self.sign() != in.sign()).basic()) {
			return self -= (-in);
		}

		self.m_data += in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator-(this t_self self, t_in in) -> sub_ret<t_self, t_in> {
		if ((self.sign() != in.sign()).basic()) {
			return self + (-in);
		}
		else if (self.m_data < in.m_data) {
			return -(in - self);
		}

		using ret = sub_ret<t_self, t_in>;
		using ret_type = ret::int_class::type;
		ret abs{ ret_type{ self.m_data } - ret_type{ in.m_data } };
        if constexpr (is_ints_cpt<ret>) {
			if ((!self.sign()).basic()) {
				return -abs;
			}
        }
		return abs;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator-=(this t_self& self, t_in in) -> t_self& {
		if ((self.sign() != in.sign()).basic()) {
			return self += (-in);
		}

		if (self.m_data >= in.m_data) {
			self.m_data -= in.m_data;
		}
		else if constexpr (is_ints_cpt<t_self>) {
			self.m_data = in.m_data - self.m_data;
			self = -self;
		}
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator*(this t_self self, t_in in) -> mul_ret<t_self, t_in> {
		using ret = mul_ret<t_self, t_in>;
		using ret_type = ret::int_class::type;
		ret abs{ ret_type{ self.m_data } * ret_type{ in.m_data } };
        if constexpr (is_ints_cpt<ret>) {
			if ((self.sign() != in.sign()).basic()) {
				return -abs;
			}
        }
		return abs;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator*=(this t_self& self, t_in in) -> t_self& {
		self.m_data *= in.m_data;
        if constexpr (is_ints_cpt<t_self>) {
			if ((!in.sign()).basic()) {
				self = -self;
			}
        }
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator/(this t_self self, t_in in) -> div_ret<t_self, t_in> {
		if ((!(self % in)).basic()) {
			throw e_other{ U""_strmdc };
		}
		return self.divtz(in);
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator/=(this t_self& self, t_in in) -> t_self& {
		if ((!(self % in)).basic()) {
			throw e_other{ U""_strmdc };
		}
		return self.divtz_a(in);
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divup(this t_self self, t_in in) -> div_ret<t_self, t_in> {
        if (self.sign().basic()) {
            return self.divaz(in);
        }
        else {
            return self.divtz(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divup_a(this t_self& self, t_in in) -> t_self& {
        if (self.sign().basic()) {
            return self.divaz_a(in);
        }
        else {
            return self.divtz_a(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divdw(this t_self self, t_in in) -> div_ret<t_self, t_in> {
        if (self.sign().basic()) {
            return self.divtz(in);
        }
        else {
            return self.divaz(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divdw_a(this t_self& self, t_in in) -> t_self& {
        if (self.sign().basic()) {
            return self.divtz_a(in);
        }
        else {
            return self.divaz_a(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divtz(this t_self self, t_in in) -> div_ret<t_self, t_in> {
		using ret = div_ret<t_self, t_in>;
		using ret_type = ret::int_class::type;
		ret abs{ ret_type{ self.m_data } / ret_type{ in.m_data } };
        if constexpr (is_ints_cpt<ret>) {
			if ((self.sign() != in.sign()).basic()) {
				return -abs;
			}
        }
		return abs;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divtz_a(this t_self& self, t_in in) -> t_self& {
		self.m_data /= in.m_data;
        if constexpr (is_ints_cpt<self>) {
			if ((!in.sign()).basic()) {
				self = -self;
			}
        }
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divaz(this t_self self, t_in in) -> div_ret<t_self, t_in> {
		auto out{ self.divtz(in) };
		if ((!(self % in)).basic()) {
			out.m_data += 1;
		}
		return out;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::divaz_a(this t_self& self, t_in in) -> t_self& {
		self.divtz(in);
		if ((!(self % in)).basic()) {
			self.m_data += 1;
		}
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator%(this t_self self, t_in in) -> boo {
		return self.modtz(in) == intpc{ 0 };
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modup(this t_self self, t_in in) -> modup_ret<t_self, t_in> {
        if (self.sign().basic()) {
            return self.modaz(in);
        }
        else {
            return self.modtz(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modup_a(this t_self& self, t_in in) -> t_self& {
        if (self.sign().basic()) {
            return self.modaz_a(in);
        }
        else {
            return self.modtz_a(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::moddw(this t_self self, t_in in) -> moddw_ret<t_self, t_in> {
        if (self.sign().basic()) {
            return self.modtz(in);
        }
        else {
            return self.modaz(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::moddw_a(this t_self& self, t_in in) -> t_self& {
        if (self.sign().basic()) {
            return self.modtz_a(in);
        }
        else {
            return self.modaz_a(in);
        }
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modtz(this t_self self, t_in in) -> modtz_ret<t_self, t_in> {
		using ret = modtz_ret<t_self, t_in>;
		using ret_type = ret::int_class::type;
		ret abs{ ret_type{ self.m_data } % ret_type{ in.m_data } };
        if constexpr (is_ints_cpt<ret>) {
			if ((!self.sign()).basic()) {
				return -abs;
			}
        }
		return abs;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modtz_a(this t_self& self, t_in in) -> t_self& {
		self.m_data %= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modaz(this t_self self, t_in in) -> modaz_ret<t_self, t_in> {
		auto out{ self.modtz(in) };
        if ((out != U"0"_intpc).basic()) {
            out.m_data = in.m_data - out.m_data;
        }
        return out;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::modaz_a(this t_self& self, t_in in) -> t_self& {
		self.modtz(in);
        if ((self != U"0"_intpc).basic()) {
            self.m_data = in.m_data - self.m_data;
        }
        return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator<<(this t_self self, t_in in) -> t_self {
		if ((!in.sign()).basic()) {
			return self >> -in;
		}
		self.m_data <<= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator<<=(this t_self& self, t_in in) -> t_self& {
		if ((!in.sign()).basic()) {
			return self >>= -in;
		}
		self.m_data <<= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator>>(this t_self self, t_in in) -> t_self {
		if ((!in.sign()).basic()) {
			return self << -in;
		}
		self.m_data >>= in.m_data;
		return self;
	}

	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::operator>>=(this t_self& self, t_in in) -> t_self& {
		if ((!in.sign()).basic()) {
			return self <<= -in;
		}
		self.m_data >>= in.m_data;
		return self;
	}
	
	template<is_basic_intp_cpt t_type>
	template<is_int_cpt t_self, is_int_cpt t_in>
	constexpr auto int_class<t_type>::pow(this t_self self, t_in in) -> t_self {
		t_self out{ U"1"_intpc };
		for (intpc i{ U"0"_intpc }; (i < in).basic(); ++i) {
			out *= self;
		}
		return out;
	}
}

#endif