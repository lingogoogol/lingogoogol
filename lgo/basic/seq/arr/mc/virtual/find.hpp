#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_FIND
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_FIND

#include "reduce.hpp"

#include "../main/decl.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tfind_private
    : private arrmc_treduce_private<arrmc_tfind_private<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base_treduce = arrmc_treduce_private<arrmc_tfind_private, t_cvt, t_type...>;

        template<is_bind_cpt u_tem>
        struct tfind_stu {
            template<typename v_previous, typename v_current, is_intp_v_cpt v_pos, typename>
            struct bind_stu {
                struct type {
                    using processed = tcond_basic<u_tem::template type<v_current>::val.basic()
                    , typename v_previous::template tmerge<arrmc<v_pos>>, v_previous>;
                    using state = void;
                };
            };
        };
    protected:
        template<is_bind_cpt u_bind>
        using tfind_private = base_treduce::template treduce_private
        <bind<tem<tfind_stu<u_bind>::template bind_stu>>, arrmc<>, void>;
    private:
        template<is_bind_cpt u_tem>
        struct tfind_one_stu {
            using ind = tfind_private<u_tem>;
            static_assert((ind::len() == U"1"_intpc).basic());
            using type = ind::template type<intpc::val_v<U"0">>;
        };
    protected:
        using tfind_one_bind_private = bind<tem<tfind_one_stu>>;
        template<is_bind_cpt u_tem>
        using tfind_one_private = tfind_one_stu<u_tem>::type;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tfind
    : private arrmc_tfind_private<arrmc_tfind<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_tfind_private<arrmc_tfind, t_cvt, t_type...>;
    public:
        template<is_bind_cpt u_bind>
        using tfind = base::template tfind_private<u_bind>;
        using tfind_one_bind = base::tfind_one_bind_private;
        template<is_bind_cpt u_tem>
        using tfind_one = base::template tfind_one_private<u_tem>;
    };
}

#endif