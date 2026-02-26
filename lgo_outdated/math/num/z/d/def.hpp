#ifndef LGO_MATH_NUM_Z_D_DEF
#define LGO_MATH_NUM_Z_D_DEF

#include "decl.hpp"

namespace lgo {
	template<is_uintegral t_type>
	class numd {
	public:
		using type = t_type;
		dnum();
		template<is_integral t_in>
		dnum(t_in in);
		dnum(const strd& in);
		dnum(const dnum<type>& in);
		dnum(dnum<type>&& in) noexcept;
		auto operator-() const -> dnum<type>;
		template<typename t_in>
		auto operator+(t_in&& in) const -> dnum<type>
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator-(t_in&& in) const -> dnum<type>
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator*(t_in&& in) const -> dnum<type>
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator/(t_in&& in) const -> dnum<type>
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator%(t_in&& in) const -> dnum<type>
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		auto operator<<(intf64 bits) const -> dnum<type>;
		auto operator>>(intf64 bits) const -> dnum<type>;
		template<typename t_in>
		auto operator>(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator<(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator>=(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator<=(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator==(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator!=(t_in&& in) const -> bool
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator<=>(t_in&& in) const -> dir
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		auto operator=(const dnum<type>& in) & -> numd&;
		auto operator=(dnum<type>&& in) & noexcept -> numd&;
		template<typename t_in>
		auto operator+=(t_in&& in) -> numd&
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator-=(t_in&& in) -> numd&
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator*=(t_in&& in) -> numd&
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator/=(t_in&& in) -> numd&
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		template<typename t_in>
		auto operator%=(t_in&& in) -> numd&
			requires is_same<rm_cvr<t_in>, dnum<type>>;
		auto operator>>=(intf64 bits) -> numd&;
		auto operator<<=(intf64 bits) -> numd&;
		template<is_integral t_out>
		operator t_out() const;
		template<is_snum t_out>
		operator t_out() const;
		operator strd() const;
		auto len() const -> intf64;
		auto highest_bit() const -> intf64;
		auto lowest_bit() const -> intf64;
		auto size() const -> intf64;
		auto positive() ->bool&;
		auto positive()const ->const bool&;
		auto bit(intf64 pos)->type;
		auto data() const -> const type*;
		auto move()->arrd<type>&&;
		auto abs() const->numd&;
		auto resize(intf64 new_high, intf64 new_low) -> numd&;
		auto shrink()->numd&;
		template<is_dnum T1>
		friend T1 resize(const T1& in, intf64 int_size, intf64 dec_size);
	private:
		auto operator[](intf64 pos) -> type&;
		auto operator[](intf64 pos) const -> type;
		auto getval(intf64 pos) const -> type;
		auto pos_to_index(intf64 pos) const -> intf64;
		auto low_pos() const -> intf64;
		auto high_pos() const -> intf64;
		bool m_positive{};
		arrd<type> m_data{};
		intf64 m_dot{};
		intf64 m_begin{};
		intf64 m_end{};
		template<intf64, intf64, is_uintegral>
		friend class snum;
		template<is_uintegral>
		friend class dnum;
		template<is_num t_out>
		friend auto impl::str_num(const strd& in) -> t_out;
		template<typename t_in1, typename t_in2, is_num t_dest>
		friend void impl::num_add(t_in1&& in1, t_in2&& in2, t_dest& dest)
			requires is_same<typename rm_cvr<t_in1>::type,
			typename rm_cvr<t_in2>::type, typename t_dest::type>;
		template<typename t_in1, typename t_in2, is_num t_dest>
		friend void impl::num_sub(t_in1&& in1, t_in2&& in2, t_dest& dest)
			requires is_same<typename rm_cvr<t_in1>::type,
			typename rm_cvr<t_in2>::type, typename t_dest::type>;
		template<typename t_in1, typename t_in2, is_num t_dest>
		friend void impl::num_mul(t_in1&& in1, t_in2&& in2, t_dest& dest)
			requires is_same<typename rm_cvr<t_in1>::type,
			typename rm_cvr<t_in2>::type, typename t_dest::type>;
		template<typename t_in1, typename t_in2, is_num t_dest>
		friend void impl::num_div(t_in1&& in1, t_in2&& in2, t_dest& dest)
			requires is_same<typename rm_cvr<t_in1>::type,
			typename rm_cvr<t_in2>::type, typename t_dest::type>;
		template<typename t_in1, typename t_in2, is_num t_dest>
		friend void impl::num_mod(t_in1&& in1, t_in2&& in2, t_dest& dest)
			requires is_same<typename rm_cvr<t_in1>::type,
			typename rm_cvr<t_in2>::type, typename t_dest::type>;
		template<dir t_dir, is_num t_in, is_num t_dest>
		friend void impl::num_shift(const t_in& in, intf64 bits, t_dest& dest)
			requires is_same<typename rm_cvr<t_in>::type, typename t_dest::type>;
		template<typename t_in1, typename t_in2>
		friend auto impl::num_compare(t_in1&& in1, t_in2&& in2) -> dir requires
			is_same<typename rm_cvr<t_in1>::type, typename rm_cvr<t_in2>::type>;
		template<typename t_in>
		friend auto impl::num_strd(t_in&& in) -> strd;
	};
}

#endif