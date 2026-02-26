#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_EACH
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_EACH

#include "replace.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_teach_private: private arrmc_cvt_private<arrmc_teach_private<t_disambiguater, t_cvt, t_type...>, t_cvt>
    , private arrmc_treduce_private<arrmc_teach_private<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_teach_private, t_cvt>;
        using base_treduce = arrmc_treduce_private<arrmc_teach_private, t_cvt, t_type...>;

        template<is_bind_cpt u_tem>
        struct teach_stu {
            template<typename v_previous, typename v_current, is_intp_v_cpt v_pos, typename>
            struct bind_stu {
                struct type {
                    using processed = v_previous::template type_merge<typename base_cvt::template from_base_private
                    <arrmc_base<typename u_tem::template type<v_current>>>>;
                    using state = void;
                };
            };
        };
    public:
        template<is_bind_cpt u_bind>
        using teach_private = base_treduce::template treduce_private<bind<tem<teach_stu<u_bind>::template bind_stu>>
        , typename base_cvt::template from_base_private<arrmc_base<>>, void>;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_teach: private arrmc_teach_private<arrmc_teach<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_teach_private<arrmc_teach, t_cvt, t_type...>;
    public:
        template<is_bind_cpt u_bind>
        using teach = base::template teach_private<u_bind>;
    };
}

#endif