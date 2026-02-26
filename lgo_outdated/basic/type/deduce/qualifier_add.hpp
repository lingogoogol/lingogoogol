#ifndef LGO_BASIC_TYPE_DEDUCE_QUALIFIER_ADD
#define LGO_BASIC_TYPE_DEDUCE_QUALIFIER_ADD

LGO_EXPORT namespace lgo {
    template<typename t_in>
    using add_qc = const t_in;
    template<typename t_in>
    using add_qv = volatile t_in;

    template<typename t_in>
    using add_qp = t_in*;
    template<typename t_in>
    using add_qlr = t_in&;
    template<typename t_in>
    using add_qrr = t_in&&;

    template<typename t_in>
    using add_qcv = add_qv<add_qc<t_in>>;
}

#endif