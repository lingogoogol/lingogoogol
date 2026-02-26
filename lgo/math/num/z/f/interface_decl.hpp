#ifndef LGO_MATH_NUM_Z_F_INTERFACE_DECL
#define LGO_MATH_NUM_Z_F_INTERFACE_DECL

LGO_EXPORT namespace lgo {
	template<is_intp_v_cpt t_len>
    class numzf8;
	template<is_intp_v_cpt t_len>
    class numzf16;
	template<is_intp_v_cpt t_len>
    class numzf32;
	template<is_intp_v_cpt t_len>
    class numzf64;

    template<is_intp_v_cpt t_len>
    using numzfc = numzf64<t_len>;
}

#endif