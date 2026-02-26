#ifndef LGO_BASIC_SEQ_STR_MAIN_DEF
#define LGO_BASIC_SEQ_STR_MAIN_DEF

#include "decl.hpp"

#include "../cpt/s_cpt.hpp"
#include "../../main/main/opt/plain/.hpp"
#include "../../../type/literal_str/def.hpp"
#include "../../../type/literal_str/cpt.hpp"
#include "../../../io/opt/.hpp"

namespace lgo {
    class str {
    protected:
        template<typename t_self, is_literal_str_cpt t_in>
        constexpr auto ctor(this t_self& self, const t_in& in) -> void;
    public:
		template<is_str_cpt t_self>
        constexpr auto size(this const t_self& self, const size_mem<seq_plain>&) -> intpc;

        template<is_str_cpt t_in, is_str_cpt t_out>
        constexpr static auto str_pad(const t_in& in, t_out& out) -> void;
        template<is_str_cpt t_out, is_mem_cpt t_in>
        constexpr static auto mem_get(const t_in& in) -> t_out;
        template<is_str_cpt t_out, is_mem_cpt t_in>
        constexpr static auto mem_get(const t_in& in, const io_len& opt) -> t_out;
        template<is_str_cpt t_out, is_mem_cpt t_in, is_io_until_ignore_cpt t_opt>
        constexpr static auto mem_get(const t_in& in, const t_opt& opt) -> t_out;
        template<is_str_cpt t_in, is_mem_cpt t_out>
        constexpr static auto mem_pad(const t_in& in, t_out& out) -> void;
        template<is_str_cpt t_in, is_mem_cpt t_out>
        constexpr static auto mem_pad(const t_in& in, t_out& out, seq_plain) -> void;

        template<is_basic_cha_cpt t_type, typename t_self>
        operator std::basic_string<t_type>(this const t_self& self);
    };
}

#endif