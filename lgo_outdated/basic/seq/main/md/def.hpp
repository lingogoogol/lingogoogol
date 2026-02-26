#ifndef LGO_BASIC_SEQ_MAIN_MD_DEF
#define LGO_BASIC_SEQ_MAIN_MD_DEF

#include "opt/cpt.hpp"

#include "../cpt/.hpp"
#include "../d/def.hpp"
#include "../../../type/deduce/qualifier_rm.hpp"
#include "../../../type/constraint/.hpp"
#include "../../../type/fun/cast_impl.hpp"
#include "../../../type/int/p/def.hpp"
#include "../../../type/tem/.hpp"
#include "../../../type/ptr/s/decl.hpp"
#include "../../../bind/iden.hpp"
#include "../../../size/heap/.hpp"

namespace lgo {
    template<typename t_type>
    class seqmd: public seqd {
    private:
        template<is_basic_literal_str_v_cpt t_in>
        struct intpc_val_stu {
            using type = intpc::val_v<t_in::val>;
        };
        using intpc_val_bind = bind<tem<intpc_val_stu>>;
        constexpr static basic_literal_str basic_literal_str_0{ U"0" };

        template<is_bind_cpt t_bind, is_seqmd_opt_cpt... t_in>
        using find_tem = arrmc<t_in...>::template tfind<t_bind>::template type
        <typename suspend_bind<intpc_val_bind, v<basic_literal_str_0>>::template type<t_bind>>;
    public:
        using type = t_type;
        using type_t = t<type>;

        template<is_seqmd_opt_cpt... t_in>
        seqmd(t_in&&... in);
		seqmd(const std::vector<type>& in);
    protected:
        constexpr static auto size(size_stack) -> intpc;
        constexpr auto size(size_heap) const -> intpc
        requires requires { type::size(size_stack{}); };
        constexpr auto size(size_content) const -> intpc
        requires requires (const type element) { element.size(size_content{}); };

        seqmd(const seqmd& in);
        template<is_seq_impl_cpt t_in>
        seqmd(const t_in& in)
        requires is_same_cpt<typename t_in::type, type>;
        seqmd(seqmd&& in);
        template<is_seq_impl_cpt t_self, is_seq_impl_cpt t_in>
        auto ctor(this t_self& self, const t_in& in) -> void;
    public:
        template<is_seqd_cpt t_self, is_seq_cpt t_in>
        auto operator=(this t_self& self, const t_in& in) -> t_self&;
        template<is_seqd_cpt t_self>
        auto operator=(this t_self& self, t_self&& in) -> t_self&;

        auto len() const -> intpc;
        template<is_seqd_cpt t_self>
        auto resize(this t_self& self, intpc len) -> t_self&;
        template<is_seqd_cpt t_self>
        auto reserve(this t_self& self, intpc len) -> t_self&;
        template<is_seqd_cpt t_self>
        auto clear(this t_self& self) -> t_self&;
        template<is_seqd_cpt t_self>
        auto operator+(this const t_self& self, const t_self& in) -> t_self;
        template<is_seqd_cpt t_self>
        auto operator+=(this t_self& self, const t_self& in) -> t_self&;

        auto operator[](intpc pos) -> type&;
        auto operator[](intpc pos) const -> const type&;
        auto data() -> ptrs<type>;
        auto data() const -> ptrs<const type>;
    private:
        auto discard() -> seqmd&;

        intpc m_len{};
        intpc m_reserve{};
        type* m_data{ nullptr };
    };
}

#endif