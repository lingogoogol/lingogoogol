#ifndef LGO_MATH_NUM_Z_S_INTERFACE_DECL
#define LGO_MATH_NUM_Z_S_INTERFACE_DECL

LGO_EXPORT namespace lgo {
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs8;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs16;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs32;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    class numzs64;

    template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzsc = numzs64<t_lit, t_len>;
}

LGO_EXPORT namespace lgo {
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzs8_t = t<numzs8<t_lit, t_len>>;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzs16_t = t<numzs16<t_lit, t_len>>;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzs32_t = t<numzs32<t_lit, t_len>>;
	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzs64_t = t<numzs64<t_lit, t_len>>;

    template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
    using numzsc_t = t<numzsc<t_lit, t_len>>;
}

#endif