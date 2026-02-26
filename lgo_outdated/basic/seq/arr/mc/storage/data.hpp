#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_DATA
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_DATA

#include "data_opt.hpp"

#include "../../../main/main/opt/data/cpt.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    class arrmc_data {
    private:
        template<is_intp_v_cpt u_pos, typename u_in>
        constexpr arrmc_data(placeholder<u_pos>, u_in&&)
        requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>;

        template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
        friend class arrmc_data;
    public:
        constexpr arrmc_data() = default;
        constexpr arrmc_data(const seq_data<>&)
        requires is_same_cpt<t_opt, arrmc_data_copyable>;
    };

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<is_intp_v_cpt u_pos, typename u_in>
    constexpr arrmc_data<t_disambiguater, t_cvt, t_opt, t_type...>::arrmc_data(placeholder<u_pos>, u_in&&)
    requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>> {}
    
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    constexpr arrmc_data<t_disambiguater, t_cvt, t_opt, t_type...>::arrmc_data(const seq_data<>&)
    requires is_same_cpt<t_opt, arrmc_data_copyable> {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    class arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>
    : private arrmc_type_private<arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>, t_cvt, t_first, t_other...> {
    private:
        using base_type = arrmc_type_private<arrmc_data, t_cvt, t_first, t_other...>;
        
        template<is_intp_v_cpt u_pos>
        constexpr static auto incrementer(placeholder<u_pos>) -> auto;

        template<is_intp_v_cpt u_pos, typename u_in>
        constexpr arrmc_data(placeholder<u_pos>, u_in&& in)
        requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>;

        template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
        friend class arrmc_data;
    public:
        constexpr arrmc_data() = default;
        template<typename u_first, typename... u_other>
        constexpr arrmc_data(u_first&& in, u_other&&... other)
        requires is_same_cpt<t_opt, arrmc_data_raw>;
        template<typename u_in>
        constexpr arrmc_data(u_in&& in)
        requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>;
        template<typename u_in>
        constexpr arrmc_data(u_in&&)
        requires is_same_cpt<t_opt, arrmc_data_raw> && is_same_cpt<rm_qcv<rm_qr<u_in>>, arrmc_data> = delete;

        constexpr auto operator=(const arrmc_data& in) & -> arrmc_data& = default;

        template<is_intp_v_cpt t_pos, typename t_self>
        constexpr auto ind(this t_self&& self)
        -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>&;
        template<is_intp_v_cpt t_pos, typename t_self>
        constexpr auto indr(this t_self&& self)
        -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>&&;
        template<is_intp_v_cpt t_pos, typename t_in, typename t_self>
        constexpr auto indf(this t_self&& self) -> decltype(auto);
    private:
        template<is_intp_v_cpt t_pos, typename t_self>
        constexpr auto ind_internal(this t_self&& self)
        -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>&;

        template<typename t_disambiguater, typename t_cvt>
        friend class arrmc_merge_private;

        t_first m_data{};
        arrmc_data<t_disambiguater, t_cvt, t_opt, t_other...> m_next{};
    };

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt u_pos>
    constexpr auto arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::incrementer(placeholder<u_pos>) -> auto {
        constexpr static intpc out{ u_pos::val + U"1"_intpc };
        return placeholder<v<out>>{};
    }
    
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt u_pos, typename u_in>
    constexpr arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>
    ::arrmc_data(placeholder<u_pos>, u_in&& in)
    requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>
    : m_data{ in.indf<u_pos, u_in>() }, m_next{ incrementer(placeholder<u_pos>{}), in } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<typename u_first, typename... u_other>
    constexpr arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::arrmc_data(u_first&& in, u_other&&... other)
    requires is_same_cpt<t_opt, arrmc_data_raw>: m_data{ ::lgo::f<u_first>(in) }, m_next{ ::lgo::f<u_other>(other)... } {}
    
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<typename u_in>
    constexpr arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::arrmc_data(u_in&& in)
    requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>
    : arrmc_data{ placeholder<intpc::val_v<U"0">>{}, ::lgo::f<u_in>(in) } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt t_pos, typename t_self>
    constexpr auto arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::ind(this t_self&& self)
    -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>& {
        return self.ind_internal<t_pos>();
    }

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt t_pos, typename t_self>
    constexpr auto arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::indr(this t_self&& self)
    -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>&& {
        return ::lgo::r(self.ind<t_pos>());
    }

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt t_pos, typename t_in, typename t_self>
    constexpr auto arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>
    ::indf(this t_self&& self) -> decltype(auto) {
        return ::lgo::f<t_in>(self.ind<t_pos>());
    }
    
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename t_first, typename... t_other>
    template<is_intp_v_cpt t_pos, typename t_self>
    constexpr auto arrmc_data<t_disambiguater, t_cvt, t_opt, t_first, t_other...>::ind_internal(this t_self&& self)
    -> apply_qcv<typename base_type::template type_private<t_pos>, rm_qr<t_self>>& {
        if constexpr ((t_pos::val == U"0"_intpc).basic()) {
            return self.m_data;
        }
        else {
            constexpr static intpc next_pos{ t_pos::val - U"1"_intpc };
            return self.m_next.ind_internal<v<next_pos>>();
        }
    }
}

#endif