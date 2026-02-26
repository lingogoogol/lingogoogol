#ifndef LGO_MATH_NUM_Z_F_INTERFACE_DEF
#define LGO_MATH_NUM_Z_F_INTERFACE_DEF

#include "interface_decl.hpp"
#include "base_def.hpp"

LGO_EXPORT namespace lgo {
	template<is_intp_v_cpt t_len>
    class numzf8: public lgo, public numzf<intp8, t_len> {
    private:
        using base = numzf<intp8, t_len>;
    public:
        using base::numzf;

        using base::operator=;
		constexpr auto operator=(const numzf8& in) & -> numzf8& = default;
    };
    
	template<is_intp_v_cpt t_len>
    class numzf16: public lgo, public numzf<intp16, t_len> {
    private:
        using base = numzf<intp16, t_len>;
    public:
        using base::numzf;

        using base::operator=;
		constexpr auto operator=(const numzf16& in) & -> numzf16& = default;
    };

	template<is_intp_v_cpt t_len>
    class numzf32: public lgo, public numzf<intp32, t_len> {
    private:
        using base = numzf<intp32, t_len>;
    public:
        using base::numzf;

        using base::operator=;
		constexpr auto operator=(const numzf32& in) & -> numzf32& = default;
    };

	template<is_intp_v_cpt t_len>
    class numzf64: public lgo, public numzf<intp64, t_len> {
    private:
        using base = numzf<intp64, t_len>;
    public:
        using base::numzf;

        using base::operator=;
		constexpr auto operator=(const numzf64& in) & -> numzf64& = default;
    };
}

#endif