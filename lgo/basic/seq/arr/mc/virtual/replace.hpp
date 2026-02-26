#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_REPLACE
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_REPLACE

#include "reduce.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_treplace_private: private arrmc_cvt_private<arrmc_treplace_private<t_disambiguater, t_cvt, t_type...>, t_cvt>
    , private arrmc_treduce_private<arrmc_treplace_private<t_disambiguater, t_cvt, t_type...>, t_type...> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_treplace_private, t_cvt>;
        using base_treduce = arrmc_treduce_private<arrmc_treplace_private, t_type...>;

        template<is_bind_cpt u_tem, typename u_replacement>
        requires base_cvt::template is_kind_cpt<u_replacement>
        struct treplace_stu {
            template<typename v_previous, typename v_current, is_intp_v_cpt v_pos, typename v_state>
            struct bind_stu {
                constexpr static boo replace{ u_tem::template type<v_current>::val };
                constexpr static intpc next_state{ basic_condval(replace, v_state::val + U"1"_intpc, v_state::val) };
                struct type {
                    using processed = tcond_basic<replace.basic(), typename v_previous::template tmerge
                    <typename base_cvt::template from_base_private<arrmc_base
                    <typename u_replacement::template type<v_state>>>>, typename v_previous::template tmerge
                    <typename base_cvt::template from_base_private<arrmc_base<v_current>>>>;
                    using state = v<next_state>;
                };
            };
        };
    protected:
        template<is_bind_cpt u_bind, typename u_replacement>
        requires base_cvt::template is_kind_cpt_private<u_replacement>
        using treplace_private = base_treduce::template treduce_private<bind<tem<treplace_stu<u_bind, u_replacement>
        ::template bind_stu>>, typename base_cvt::template from_base_private<arrmc_base<>>, intpc::val_v<U"0">>;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_treplace: private arrmc_treplace_private<arrmc_treplace<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_treplace_private<arrmc_treplace, t_cvt, t_type...>;
    public:
        template<is_bind_cpt u_bind, typename u_replacement>
        using treplace = base::template treplace_private<u_bind, u_replacement>;
    };
}

#endif