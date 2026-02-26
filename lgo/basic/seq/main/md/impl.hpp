#ifndef LGO_BASIC_SEQ_MAIN_MD_IMPL
#define LGO_BASIC_SEQ_MAIN_MD_IMPL

#include "def.hpp"
#include "opt/len/.hpp"
#include "opt/reserve/.hpp"

#include "../d/impl.hpp"
#include "../../arr/mc/pack/.hpp"
#include "../../../type/ptr/s/def.hpp"
#include "../../../type/int/p/impl.hpp"
#include "../../../type/int/p/literal.hpp"
#include "../../../type/int/main/algo_impl.hpp"

namespace lgo {
    template<typename t_type>
    template<is_seqmd_opt_cpt... t_in>
    seqmd<t_type>::seqmd(t_in&&... in): m_len{}, m_reserve{}, m_data{} {
        using type_arg_list = arrmc<rm_qr<t_in>...>;
        pack opt{ seq_data{ ::lgo::f<t_in>(in)... } };
        if constexpr (type_arg_list::template tcontain<is_seqmd_len_bind>.basic()) {
            m_len = opt.ind<find_tem<is_seqmd_len_bind, t_in...>>().len();
        }
        else if constexpr (type_arg_list::template tcontain<is_seq_data_bind>.basic()) {
            m_len = type_arg_list::template tfilter<is_seq_data_bind>::type<intpc::val_v<U"0">>::len();
        }
        else {
            m_len = U"0"_intpc;
        }
        if constexpr (type_arg_list::template tcontain<is_seqmd_reserve_bind>.basic()) {
            m_reserve = opt.ind<find_tem<is_seqmd_reserve_bind, t_in...>>().reserve();
        }
        else {
            m_reserve = m_len;
        }
        if constexpr (type_arg_list::template tcontain<is_seq_data_bind>.basic()) {
            while (!m_data) {
                m_data = opt.ind<find_tem<is_seq_data_bind, t_in...>>().r().apply(fun{
                    [this] <typename... u_data> (u_data&&... opt_data) -> type* {
                        return new(std::nothrow) type[this->m_reserve.basic()]{ ::lgo::f<u_data>(opt_data)... };
                    }
                });
            }
        }
        else {
            while (!m_data) {
                m_data = new(std::nothrow) type[m_reserve.basic()]{};
            }
        }
        return;
    }
    
    template<typename t_type>
    seqmd<t_type>::seqmd(const std::vector<type>& in): seqmd{ seqmd_len{ in.size() } } {
        for (intpc i{ U"0"_intpc }; (i < in.size().sc<intpc>()).basic(); ++i) {
            (*this)[i] = in[i];
        }
        return;
    }

    template<typename t_type>
    constexpr auto seqmd<t_type>::size(size_stack) -> intpc {
        return intpc::size(size_stack{})
        + intpc::size(size_stack{}) + intpc{ sizeof(type*) };
    }
    
    template<typename t_type>
    constexpr auto seqmd<t_type>::size(size_heap) const -> intpc
    requires requires { type::size(size_stack{}); } {
        return type::size(size_stack{}) * len();
    }

    template<typename t_type>
    constexpr auto seqmd<t_type>::size(size_content) const -> intpc
    requires requires (const type element) { element.size(size_content{}); } {
        intpc out{ U"0"_intpc };
        for (intpc i{ U"0"_intpc }; (i < len()).basic(); ++i) {
            out += (*this)[i].size(size_content{});
        }
        return out;
    }

    template<typename t_type>
    seqmd<t_type>::seqmd(const seqmd& in): seqmd{ seqmd_len{ in.len() } } {}

    template<typename t_type>
    template<is_seq_impl_cpt t_in>
    seqmd<t_type>::seqmd(const t_in& in)
    requires is_same_cpt<typename t_in::type, type>: seqmd{ seqmd_len{ in.len() } } {}

    template<typename t_type>
    seqmd<t_type>::seqmd(seqmd&& in): m_len{ in.len() }, m_reserve{ in.m_reserve }, m_data{ in.m_data } {
        in.discard();
        return;
    }
    
    template<typename t_type>
    template<is_seq_impl_cpt t_self, is_seq_impl_cpt t_in>
    auto seqmd<t_type>::ctor(this t_self& self, const t_in& in) -> void {
        self.subms() = in;
        return;
    }

    template<typename t_type>
    template<is_seqd_cpt t_self, is_seq_cpt t_in>
    auto seqmd<t_type>::operator=(this t_self& self, const t_in& in) -> t_self& {
        fun reallocate{
            [&self]
            <is_seq_cpt u_in>
            (const u_in& in) -> void {
                delete[] self.m_data;
                self.m_data = nullptr;
                self.m_len = in.len();
                self.m_reserve = self.m_len;
                while (!self.m_data) {
                    self.m_data = new(std::nothrow) type[self.m_len.basic()]{};
                }
                self.subms() = in;
                return;
            }
        };
        if constexpr (is_seqm_boo<t_in>.basic()) {
            if (self.m_data == in.m_data) {
                return self;
            }
            reallocate(in);
        }
        else {
            if ((self.data() == in.m_data.ref().data()).basic()) {
                reallocate(in);
            }
            else {
                reallocate(t_self{ in });
            }
        }
        return self;
    }

    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::operator=(this t_self& self, t_self&& in) -> t_self& {
        if (self.m_data != in.m_data) {
            delete[] self.m_data;
            self.m_len = in.m_len;
            self.m_reserve = in.m_reserve;
            self.m_data = in.m_data;
            in.discard();
        }
        return self;
    }

    template<typename t_type>
    auto seqmd<t_type>::len() const -> intpc {
        return m_len;
    }
    
    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::resize(this t_self& self, intpc len) -> t_self& {
        if ((len <= self.m_reserve.divdw(U"2"_intpc) || len > self.m_reserve).basic()) {
            self.reserve(len);
        }
        self.m_len = len;
        return self;
    }

    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::reserve(this t_self& self, intpc in) -> t_self& {
        intpc len{ min(self.len(), in) };
        t_self temp{ seqmd_len{ len }, seqmd_reserve{ in } };
        temp.subms(U"0"_intpc, len) = self.subcs(U"0"_intpc, len);
        return self = temp.r();
    }

    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::clear(this t_self& self) -> t_self& {
        return self = t_self{};
    }

    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::operator+(this const t_self& self, const t_self& in) -> t_self {
        t_self out{ seqmd_len{ self.len() + in.len() } };
        out.subms(U"0"_intpc, self.len()) = self;
        out.subms(self.len(), in.len()) = in;
        return out;
    }

    template<typename t_type>
    template<is_seqd_cpt t_self>
    auto seqmd<t_type>::operator+=(this t_self& self, const t_self& in) -> t_self& {
        intpc orig_len{ self.len() };
        self.resize(self.len() + in.len());
        self.subms(orig_len, in.len()) = in;
        return self;
    }

    template<typename t_type>
    auto seqmd<t_type>::operator[](intpc pos) -> type& {
        if ((pos >= len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        return m_data[pos.basic()];
    }

    template<typename t_type>
    auto seqmd<t_type>::operator[](intpc pos) const -> const type& {
        if ((pos >= len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        return m_data[pos.basic()];
    }

    template<typename t_type>
    auto seqmd<t_type>::data() -> ptrs<type> {
        return ptrs<type>{ m_data };
    }

    template<typename t_type>
    auto seqmd<t_type>::data() const -> ptrs<const type> {
        return ptrs<const type>{ m_data };
    }
    
    template<typename t_type>
    auto seqmd<t_type>::discard() -> seqmd& {
        m_len = U"0"_intpc;
        m_reserve = U"0"_intpc;
        m_data = nullptr;
        return *this;
    }
}

#endif