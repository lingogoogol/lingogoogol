#ifndef LGO_BASIC_TYPE_CONSTRAINT_QUALIFIER
#define LGO_BASIC_TYPE_CONSTRAINT_QUALIFIER

#include "../deduce/qualifier_stu.hpp"
#include "../deduce/qualifier_rm.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_qc_cpt = qc_stu<t_in>::val;
    template<typename t_in>
    concept is_qv_cpt = qv_stu<t_in>::val;

    template<typename t_in>
    concept is_qp_cpt = qp_stu<t_in>::val;
    template<typename t_in>
    concept is_qlr_cpt = qlr_stu<t_in>::val;
    template<typename t_in>
    concept is_qrr_cpt = qrr_stu<t_in>::val;

    template<typename t_in>
    concept is_qr_cpt = is_qlr_cpt<t_in> || is_qrr_cpt<t_in>;
    template<typename t_in>
    concept is_qcv_cpt = is_qc_cpt<t_in> && is_qv_cpt<t_in>;
    template<typename t_in>
    concept is_qcr_cpt = is_qr_cpt<t_in> && is_qc_cpt<rm_qr<t_in>>;
    template<typename t_in>
    concept is_qvr_cpt = is_qr_cpt<t_in> && is_qv_cpt<rm_qr<t_in>>;
    template<typename t_in>
    concept is_qcvr_cpt = is_qr_cpt<t_in> && is_qcv_cpt<rm_qr<t_in>>;
}

#endif