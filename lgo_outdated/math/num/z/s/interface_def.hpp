#ifndef LGO_MATH_NUM_Z_S_INTERFACE_DEF
#define LGO_MATH_NUM_Z_S_INTERFACE_DEF

#include "interface_decl.hpp"
#include "base_def.hpp"

LGO_EXPORT namespace lgo {
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs8: public lgo, public numzs<intp8, t_lit, t_len> {
    private:
        using base = numzs<intp8, t_lit, t_len>;
    public:
        using base::numzs;

        using base::operator=;
		constexpr auto operator=(const numzs8& in) & -> numzs8& = default;
    };
    
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs16: public lgo, public numzs<intp16, t_lit, t_len> {
    private:
        using base = numzs<intp16, t_lit, t_len>;
    public:
        using base::numzs;

        using base::operator=;
		constexpr auto operator=(const numzs16& in) & -> numzs16& = default;
    };

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs32: public lgo, public numzs<intp32, t_lit, t_len> {
    private:
        using base = numzs<intp32, t_lit, t_len>;
    public:
        using base::numzs;

        using base::operator=;
		constexpr auto operator=(const numzs32& in) & -> numzs32& = default;
    };

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs64: public lgo, public numzs<intp64, t_lit, t_len> {
    private:
        using base = numzs<intp64, t_lit, t_len>;
    public:
        using base::numzs;

        using base::operator=;
		constexpr auto operator=(const numzs64& in) & -> numzs64& = default;
    };
}

#endif