#ifndef LGO_BASIC_TYPE_PTR_S_DEF
#define LGO_BASIC_TYPE_PTR_S_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../tem/t/.hpp"
#include "../../../type/deduce/qualifier_apply.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class ptrs: public ptr<t_type>, public lgo {
    private:
        using base = ptr<t_type>;
    public:
        using base::ptr;
        explicit constexpr ptrs(t_type&& in);
        explicit constexpr ptrs(rm_qr<t_type>* in);
        constexpr ptrs(const ptrs&) = default;

        constexpr auto operator=(const ptrs&) & -> ptrs& = default;

        constexpr operator ptrs<const rm_qr<t_type>&>() const &;
        constexpr operator ptrs<const rm_qr<t_type>>() const &&;
		template<typename t_out>
		constexpr operator ptrs<t_out&>() const &;
		template<typename t_out>
		constexpr operator ptrs<t_out>() const &&;

        template<typename t_self>
		constexpr auto operator*(this t_self&& self) -> apply_qr<apply_qr<typename base::type, t_type>, t_self>&&;
        template<typename t_self>
		constexpr auto ref(this t_self&& self) -> apply_qr<apply_qr<typename base::type, t_type>, t_self>&&;
    };

    template<typename t_in>
    ptrs(t_in*) -> ptrs<t_in&>;
}

#endif