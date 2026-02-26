#ifndef LGO_MATH_NUM_Z_D_IMPL
#define LGO_MATH_NUM_Z_D_IMPL

#include "def.hpp"

	template<is_dnum T1>
	T1 resize(const T1& in, intf64 highest_bit, intf64 lowest_bit) {
		T1 out{};
		intf64 type_bits{ size<typename T1::type>() * 8 };
		intf64 moded_size{ div_rup(highest_bit, type_bits) -
			div_rdown(lowest_bit, type_bits) };
		out.m_begin = -(highest_bit + 1) % type_bits;
		out.m_begin += (out.m_begin < 0 ? type_bits : 0);
		out.m_dot = out.m_begin + highest_bit + 1;
		out.m_end = out.m_dot - lowest_bit;
		intf64 shift_bits{ out.m_dot - in.m_dot };
		if (moded_size >= in.m_data.len()) {
			out.m_data = resize(in.m_data, moded_size);
			impl::num_shift<dir::right>(out, shift_bits, out);
		}
		else {
			impl::num_shift<dir::right>(in, shift_bits, out);
			out.m_data.resize(moded_size);
		}
		return out;
	}

namespace lgo {
	template<is_uintegral t_type>
	dnum<t_type>::dnum() :m_positive{ true }, m_data{}, m_dot{ 0 }, m_begin{ 0 }, m_end{ 0 } {}

	template<is_uintegral t_type>
	template<is_integral t_in>
	dnum<t_type>::dnum(t_in in) : m_positive{ in >= 0 }, m_data{ lgo::abs(in) },
		m_dot{ lgo::size<type>() * 8 }, m_begin{ 0 }, m_end{ lgo::size<type>() * 8 } {}

	template<is_uintegral t_type>
	dnum<t_type>::dnum(const strd& in) : dnum{} {
		intf64 find_res{ in.find('.') };
		intf64 dot_i{ find_res == -1 ? in.len() : find_res };
		*this = impl::str_num<dnum<type>>(in);
		resize(dot_i * 4 - 1, max((in.len() - dot_i - 1) * 4, 0));
	}

	template<is_uintegral t_type>
	dnum<t_type>::dnum(const dnum<t_type>& in) : m_positive{ in.m_positive }, m_data { in.m_data },
		m_dot{ in.m_dot }, m_begin{ in.m_begin }, m_end{ in.m_end } {}

	template<is_uintegral t_type>
	dnum<t_type>::dnum(dnum<t_type>&& in) noexcept :
		m_positive{ in.m_positive }, m_data { std::move(in.m_data) },
		m_dot{ in.m_dot }, m_begin{ in.m_begin }, m_end{ in.m_end } {
		in.m_dot = 0;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator-() const->dnum<type> {
		dnum<type> out{ *this };
		out.m_positive = !m_positive;
		return out;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator+(t_in&& in) const -> dnum<type>
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		dnum<t_type> out{ *this };
		return out += std::forward<t_in>(in);
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator-(t_in&& in) const -> dnum<type>
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		dnum<t_type> out{ *this };
		return out -= std::forward<t_in>(in);
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator*(t_in&& in) const -> dnum<type>
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		dnum<type> out{};
		impl::num_mul(*this, std::forward<t_in>(in),
			out.resize(highest_bit() + in.highest_bit(),
				lowest_bit() + in.lowest_bit()));
		return out;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator/(t_in&& in) const -> dnum<type>
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		dnum<type> out{};
		impl::num_div(*this, std::forward<t_in>(in), out.resize(
			highest_bit() + in.lowest_bit(), lowest_bit() + in.highest_bit()));
		return out;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator%(t_in&& in) const -> dnum<type>
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		dnum<type> out{};
		impl::num_mod(*this, std::forward<t_in>(in), out);
		return out;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator<<(intf64 bits) const -> dnum<type> {
		dnum<t_type> out{ *this };
		return out <<= bits;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator>>(intf64 bits) const -> dnum<type> {
		dnum<t_type> out{ *this };
		return out >>= bits;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator>(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) == dir::greater;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator<(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) == dir::less;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator>=(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) != dir::less;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator<=(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) != dir::greater;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator==(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) == dir::equal;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator!=(t_in&& in) const -> bool
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return (*this <=> std::forward<t_in>(in)) != dir::equal;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator<=>(t_in&& in) const -> dir
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return impl::num_compare(*this, std::forward<t_in>(in));
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator=(const dnum<type>& in) & -> numd& {
		m_positive = in.m_positive;
		m_data = in.m_data;
		m_dot = in.m_dot;
		m_begin = in.m_begin;
		m_end = in.m_end;
		return *this;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator=(dnum<type>&& in) & noexcept -> numd& {
		m_positive = in.m_positive;
		m_data = std::move(in.m_data);
		m_dot = in.m_dot;
		m_begin = in.m_begin;
		m_end = in.m_end;
		return *this;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator+=(t_in&& in) -> numd&
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		resize(max(highest_bit(), in.highest_bit()), min(lowest_bit(), in.lowest_bit()));
		impl::num_add(*this, std::forward<t_in>(in), *this);
		return *this;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator-=(t_in&& in) -> numd&
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		resize(max(highest_bit(), in.highest_bit()), min(lowest_bit(), in.lowest_bit()));
		impl::num_sub(*this, std::forward<t_in>(in), *this);
		return *this;
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator*=(t_in&& in) -> numd&
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return *this = *this * std::forward<t_in>(in);
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator/=(t_in&& in) -> numd&
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return *this = *this / std::forward<t_in>(in);
	}

	template<is_uintegral t_type>
	template<typename t_in>
	auto dnum<t_type>::operator%=(t_in&& in) -> numd&
		requires is_same<rm_cvr<t_in>, dnum<type>> {
		return *this = *this % std::forward<t_in>(in);
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator>>=(intf64 bits) -> numd& {
		m_dot -= bits;
		return *this;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator<<=(intf64 bits) -> numd& {
		m_dot += bits;
		return *this;
	}

	template<is_uintegral t_type>
	template<is_integral t_out>
	dnum<t_type>::operator t_out() const {
		dnum<type> in{ impl::num_align_dot(*this) };
		t_out out{};
		for (intf64 i{ 0 }; i < lgo::size<t_out>() / lgo::size<type>(); ++i) {
			out *= sc<t_out>(pow(sc<intf64>(2), lgo::size<type>()));
			out += sc<t_out>(in.getval(i));
		}
		return out;
	}

	template<is_uintegral t_type>
	template<is_snum t_out>
	dnum<t_type>::operator t_out() const {
		dnum<type> in{ impl::num_align_dot(*this) };
		t_out out{};
		static_cast<lgo::strd>(out);
		io{}.pad(out);
		for (intf64 i{ -out.dec_len() }; i < out.int_len(); ++i) {
			out[i] = sc<typename t_out::type>(in.getval(i));
		}
		return out;
	}

	template<is_uintegral t_type>
	dnum<t_type>::operator strd() const {
		return impl::num_strd(*this);
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::len() const -> intf64 {
		return m_data.len();
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::highest_bit() const -> intf64 {
		return m_dot - m_begin - 1;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::lowest_bit() const -> intf64 {
		return m_dot - m_end;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::size() const -> intf64 {
		return m_data.size();
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::positive() ->bool& {
		return m_positive;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::positive()const ->const bool& {
		return m_positive;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::bit(intf64 pos)->type {
		return ((*this)[div_rdown(pos + revpmod(m_dot, lgo::size<type>() * 8),
			lgo::size<type>() * 8)] >> ((pos + revpmod(m_dot,
				lgo::size<type>() * 8)) % (lgo::size<type>() * 8))) % 0b10;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::data() const -> const type* {
		return m_data.data();
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::move() -> arrd<type>&& {
		return m_data.move();
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::abs() const->numd& {
		positive() = true;
		return *this;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::resize(intf64 new_high, intf64 new_low) -> numd& {
		return *this = lgo::resize(*this, new_high, new_low);
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::shrink()->numd& {
		for (intf64 i{ highest_bit() }; bit(i) == 0; --i) {
			m_begin++;
		}
		for (intf64 i{ lowest_bit() }; bit(i) == 0; ++i) {
			m_end--;
		}
		return *this;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator[](intf64 pos) -> type& {
		return m_data[pos_to_index(pos)];
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::operator[](intf64 pos) const -> type {
		return m_data[pos_to_index(pos)];
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::getval(intf64 pos) const -> type {
		try {
			return (*this)[pos];
		}
		catch (e) {
			return sc<type>(0);
		}
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::pos_to_index(intf64 pos) const -> intf64 {
		return div_rup(m_dot, lgo::size<type>() * 8) - pos - 1;
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::low_pos() const -> intf64 {
		return div_rup(m_dot, lgo::size<type>() * 8) - div_rup(m_end, lgo::size<type>() * 8);
	}

	template<is_uintegral t_type>
	auto dnum<t_type>::high_pos() const -> intf64 {
		return div_rup(m_dot, lgo::size<type>() * 8) - div_rdown(m_begin, lgo::size<type>() * 8) - 1;
	}
}

#endif