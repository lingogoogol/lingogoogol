#ifndef LGO_BASIC_SEQ_STR_MAIN_IMPL
#define LGO_BASIC_SEQ_STR_MAIN_IMPL

#include "def.hpp"

#include "../../../type/fun/cmp.hpp"

namespace lgo {
    template<typename t_self, is_literal_str_cpt t_in>
    constexpr auto str::ctor(this t_self& self, const t_in& in) -> void {
        for (intpc i{ U"0"_intpc }; (i < in.len()).basic(); ++i) {
            self[i] = in[i];
        }
        return;
    }

    template<is_str_cpt t_self>
    constexpr auto str::size(this const t_self& self, const size_mem<seq_plain>&) -> intpc {
        return t_self::type::size(size_mem{}) * self.len();
    }

    template<is_str_cpt t_in, is_str_cpt t_out>
    constexpr auto str::str_pad(const t_in& in, t_out& out) -> void {
        if ((out.len() < in.len()).basic()) {
            throw pad_not_enough{ in.len() - out.len() };
        }

        out.subms(U"0"_intpc, in.len()) = in;
        return;
    }

    template<is_str_cpt t_out, is_mem_cpt t_in>
    constexpr auto str::mem_get(const t_in& in) -> t_out {
        return mem_get(in, io_len{ in.len() / t_out::type::size(size_content{}) });
    }

    template<is_str_cpt t_out, is_mem_cpt t_in>
    constexpr auto str::mem_get(const t_in& in, const io_len& opt) -> t_out {
        if ((in.len() < opt.len()).basic()) {
            throw get_not_enough{ opt.len() - in.len() };
        }

        t_out out{};
        for (intpc i{ U"0"_intpc }; (i < opt.len()).basic(); ++i) {
            out.pad_resize(in.get<typename t_out::type>());
        }
        return out;
    }

    template<is_str_cpt t_out, is_mem_cpt t_in, is_io_until_ignore_cpt t_opt>
    constexpr auto str::mem_get(const t_in& in, const t_opt& opt) -> t_out {
        static t_out out{};
        if ((in.getpos() == U"0"_intpc).basic()) {
            out.clear();
        }
        fun enough{
            [] (const t_in& in, const t_opt& opt) {
                if ((in.len() - in.getpos() < opt.delim().len()).basic()) {
                    throw get_not_enough{ U"1"_intpc };
                }
            }
        };
        enough(in, opt);
        while ((in.subcs(in.getpos(), opt.delim().len()) != opt.delim()).basic()) {
            out.push(in.get<typename t_out::type>());
            enough(in, opt);
        }
        in.getpos(in.getpos() + opt.delim().len());
        return out;
    }

    template<is_str_cpt t_in, is_mem_cpt t_out>
    constexpr auto str::mem_pad(const t_in& in, t_out& out) -> void {
        if constexpr (is_strd_cpt<t_in>) {
            throw e_other{ U"not implemented"_strmdc };
        }
        else if constexpr (is_strs_cpt<t_in>) {
            mem_pad(in, out, seq_plain{});
        }
        else {
            throw e_other{ U""_strmdc };
        }
        return;
    }

    template<is_str_cpt t_in, is_mem_cpt t_out>
    constexpr auto str::mem_pad(const t_in& in, t_out& out, seq_plain) -> void {
        if ((out.len() < in.len() * t_in::type::size(size_content{})).basic()) {
            throw pad_not_enough{ in.len() * t_in::type::size(size_content{}) - out.len() };
        }

        for (intpc i{ U"0"_intpc }; (i < in.len()).basic(); ++i) {
            out.pad(in[i]);
        }
        return;
    }

    template<is_basic_cha_cpt t_type, typename t_self>
    str::operator std::basic_string<t_type>(this const t_self& self) {
        std::basic_string<t_type> out{};
        for (intpc i{ U"0"_intpc }; (i < self.len()).basic(); ++i) {
            out.push_back(sc<t_type>(self[i].basic()));
        }
        return out;
    }
}

#endif