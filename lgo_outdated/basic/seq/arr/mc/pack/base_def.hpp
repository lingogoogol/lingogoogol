#ifndef LGO_BASIC_SEQ_ARR_MC_PACK_BASE_DEF
#define LGO_BASIC_SEQ_ARR_MC_PACK_BASE_DEF

#include "base_decl.hpp"
#include "base_cpt.hpp"

namespace lgo {
    template<typename>
    struct arrmc_pack_to_virtual_stu {};

    template<typename... t_type>
    struct arrmc_pack_to_virtual_stu<arrmc_base<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename>
    struct arrmc_pack_from_virtual_stu {};

    template<typename... t_type>
    struct arrmc_pack_from_virtual_stu<arrmc_base<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_cvt>
    struct arrmc_pack_cvt_virtual {
        using is_kind_bind = t_cvt::is_kind_bind;
        using to_base = then_bind<typename t_cvt::to_base, bind<tem<arrmc_pack_to_virtual_stu>>>;
        using from_base = then_bind<bind<tem<arrmc_pack_from_virtual_stu>>, typename t_cvt::from_base>;
    };

    template<typename>
    struct arrmc_pack_to_storage_stu {};

    template<typename... t_type>
    struct arrmc_pack_to_storage_stu<arrmc_base<t_type...>> {
        using type = arrmc_base<ptrs<t_type>...>;
    };

    template<typename>
    struct arrmc_pack_from_storage_stu {};

    template<typename... t_type>
    struct arrmc_pack_from_storage_stu<arrmc_base<t_type...>> {
        using type = arrmc_base<typename t_type::type...>;
    };

    template<typename t_cvt>
    struct arrmc_pack_cvt_storage {
        using is_kind_bind = t_cvt::is_kind_bind;
        using to_base = then_bind<typename t_cvt::to_base, bind<tem<arrmc_pack_to_storage_stu>>>;
        using from_base = then_bind<bind<tem<arrmc_pack_from_storage_stu>>, typename t_cvt::from_base>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    class arrmc_pack
    : public arrmc_virtual<arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>, arrmc_pack_cvt_virtual<t_cvt>, t_type...>
    , public arrmc_storage<arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>
    , arrmc_pack_cvt_storage<t_cvt>, t_opt, ptrs<t_type>...> {
    private:
        using base_virtual = arrmc_virtual<arrmc_pack, arrmc_pack_cvt_virtual<t_cvt>, t_type...>;
        using base_storage = arrmc_storage<arrmc_pack, arrmc_pack_cvt_storage<t_cvt>, t_opt, ptrs<t_type>...>;

        template<typename u_pack, is_ptr_cpt... u_ptr>
        constexpr arrmc_pack(placeholder<intpc::val_v<U"0">>, u_pack&& pack, u_ptr... ptr)
        requires (!(rm_qr<u_pack>::len() == pack_size<u_ptr...>).basic());
        template<typename u_pack, is_ptr_cpt... u_ptr>
        constexpr arrmc_pack(placeholder<intpc::val_v<U"0">>, u_pack&&, u_ptr... ptr)
        requires ((rm_qr<u_pack>::len() == pack_size<u_ptr...>).basic());
    public:
        template<typename... u_in>
        constexpr arrmc_pack(u_in&&... in)
        requires is_same_cpt<t_opt, arrmc_data_raw>;
        template<typename u_in>
        constexpr arrmc_pack(u_in&& in)
        requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>;

        template<is_intp_v_cpt t_pos, typename t_self>
        constexpr auto ind(this t_self&& self) -> apply_qcv<typename base_virtual::template type<t_pos>, t_self>&;
        template<is_intp_v_cpt t_pos, typename t_self>
        constexpr auto indr(this t_self&& self) -> apply_qcv<typename base_virtual::template type<t_pos>, t_self>&&;
        template<is_intp_v_cpt t_pos, typename t_in, typename t_self>
        constexpr auto indf(this t_self&& self) -> decltype(auto);
    };
}

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<typename u_pack, is_ptr_cpt... u_ptr>
    constexpr arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::
    arrmc_pack(placeholder<intpc::val_v<U"0">>, u_pack&& pack, u_ptr... ptr)
    requires (!(rm_qr<u_pack>::len() == pack_size<u_ptr...>).basic())
    : arrmc_pack{ placeholder<intpc::val_v<U"0">>{}, ::lgo::f<u_pack>(pack), ptr.r()...
    , &pack.template indf<v<pack_size<u_ptr...>>, u_pack>() } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<typename u_pack, is_ptr_cpt... u_ptr>
    constexpr arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::
    arrmc_pack(placeholder<intpc::val_v<U"0">>, u_pack&&, u_ptr... ptr)
    requires ((rm_qr<u_pack>::len() == pack_size<u_ptr...>).basic()): base_storage{ seq_data{ ptr.r()... } } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<typename... u_in>
    constexpr arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::arrmc_pack(u_in&&... in)
    requires is_same_cpt<t_opt, arrmc_data_raw>: base_storage{ &in.f<u_in>()... } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<typename u_in>
    constexpr arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::arrmc_pack(u_in&& in)
    requires is_same_cpt<t_opt, arrmc_data_copyable> && is_seq_data_cpt<rm_qcv<rm_qr<u_in>>>
    : arrmc_pack{ placeholder<intpc::val_v<U"0">>{}, ::lgo::f<u_in>(in) } {}

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<is_intp_v_cpt t_pos, typename t_self>
    constexpr auto arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::ind(this t_self&& self)
    -> apply_qcv<typename base_virtual::template type<t_pos>, t_self>& {
        return *self.base_storage::ind<t_pos>();
    }

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<is_intp_v_cpt t_pos, typename t_self>
    constexpr auto arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::indr(this t_self&& self)
    -> apply_qcv<typename base_virtual::template type<t_pos>, t_self>&& {
        return ::lgo::r(self.ind<t_pos>());
    }

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    template<is_intp_v_cpt t_pos, typename t_in, typename t_self>
    constexpr auto arrmc_pack<t_disambiguater, t_cvt, t_opt, t_type...>::indf(this t_self&& self) -> decltype(auto) {
        return ::lgo::f<apply_qr<t_in, typename base_virtual::template type<t_pos>>>(self.ind<t_pos>());
    }
}

#endif