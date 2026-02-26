#ifndef LGO_BASIC_TYPE_INT_N_DEF
#define LGO_BASIC_TYPE_INT_N_DEF

#include "decl.hpp"

#include "../s/basic.hpp"
#include "../u/def.hpp"
#include "../p/alias.hpp"
#include "../main/cpt.hpp"
#include "../../lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<is_basic_intp_cpt t_type>
    class intn_class: public lgo, public intu_class<t_type, intn_class<t_type>> {
    private:
        using base = intu_class<t_type, intn_class<t_type>>;
    public:
        using base::size;
        template<typename t_in>
        constexpr static auto size(t_in in) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content>;
        constexpr static auto min() -> intn_class;
        constexpr static auto max() -> intn_class;
        
		constexpr intn_class() = default;
		template<is_basic_intp_cpt t_in>
        constexpr explicit intn_class(t_in in);
		template<is_basic_ints_cpt t_in>
        constexpr explicit intn_class(t_in in);
        template<is_int_cpt t_in>
        constexpr intn_class(const t_in& in);
        constexpr intn_class(const intn_class& in);

		template<is_int_cpt t_in>
		constexpr auto operator=(t_in in) & -> intn_class&;
		constexpr auto operator=(const intn_class& in) & -> intn_class&;
        
        constexpr auto basic() const -> typename base::type;
        
		using base::operator+;
		using base::operator-;
        constexpr auto sign() const -> boo;
        constexpr auto absp() const -> intp_class<typename base::type>;
        constexpr auto operator+() const -> intn_class;
        constexpr auto operator-() const -> intp_class<typename base::type>;

		template<typename t_self>
		constexpr auto operator++(this t_self&& self) -> t_self&&;
		constexpr auto operator++(int) -> intn_class;
		template<typename t_self>
		constexpr auto operator--(this t_self&& self) -> t_self&&;
		constexpr auto operator--(int) -> intn_class;
    };
}

#endif