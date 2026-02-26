#ifndef LGO_BASIC_TYPE_INT_S_DEF
#define LGO_BASIC_TYPE_INT_S_DEF

#include "decl.hpp"
#include "basic.hpp"
#include "basic_cvt.hpp"
#include "cpt.hpp"

#include "../p/def.hpp"
#include "../../boo/def.hpp"
#include "../../lgo/.hpp"
#include "../../tem/v/.hpp"
#include "../../deduce/cond_basic.hpp"
#include "../../../seq/arr/mc/main/decl.hpp"

LGO_EXPORT namespace lgo {
    template<is_basic_intp_cpt t_type>
    class ints_class: public lgo, public int_class<t_type> {
    private:
        using base = int_class<t_type>;

        template<is_int_cpt t_in>
        constexpr ints_class(const t_in& in, void*);
    public:
        template<typename t_in>
        constexpr static auto size(const t_in& in) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_mem<>>;
        constexpr static auto size(size_content) -> intpc;
        
        constexpr ints_class();
        template<is_basic_intp_cpt t_in>
        constexpr explicit ints_class(t_in in);
        template<is_basic_ints_cpt t_in>
        constexpr explicit ints_class(t_in in);
        template<is_int_cpt t_in>
        constexpr ints_class(const t_in& in);
        constexpr ints_class(const ints_class& in);

        template<is_int_cpt t_in>
        constexpr auto operator=(t_in in) & -> ints_class&;
		constexpr auto operator=(const ints_class& in) & -> ints_class&;

        constexpr auto basic() const -> basic_intp_to_basic_ints<typename base::type>;

		using base::operator+;
		using base::operator-;
        constexpr auto sign() const -> boo;
        constexpr auto absp() const -> intp_class<typename base::type>;
        constexpr auto operator+() const -> ints_class;
        constexpr auto operator-() const -> ints_class;

		template<typename t_self>
		constexpr auto operator++(this t_self&& self) -> t_self&&;
		constexpr auto operator++(int) -> ints_class;
		template<typename t_self>
		constexpr auto operator--(this t_self&& self) -> t_self&&;
		constexpr auto operator--(int) -> ints_class;
    private:
        boo m_sign{};
    };
}

#endif