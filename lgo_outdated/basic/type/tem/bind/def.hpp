#ifndef LGO_BASIC_TYPE_TEM_BIND_DEF
#define LGO_BASIC_TYPE_TEM_BIND_DEF

#include "cpt.hpp"
#include "arg.hpp"

#include "../tem/cpt.hpp"
#include "../t/def.hpp"
#include "../../fun/common.hpp"
#include "../../int/p/basic.hpp"
#include "../../int/p/basic_cvt.hpp"
#include "../../lgo/def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_tem, typename... t_arg>
    class bind: public lgo {
    public:
        static_assert(is_tem_cpt<t_tem> || is_bind_cpt<t_tem>);
    private:
        template<typename... u_data>
        struct list_stu;

        template<typename... u_data>
        struct list_decompose_stu {
            template<typename v_first, typename... v_other>
            struct decompose_stu {
                using first = v_first;
                using other = list_stu<v_other...>;
            };

            using first = decompose_stu<u_data...>::first;
            using other = decompose_stu<u_data...>::other;
        };

        template<>
        struct list_decompose_stu<> {};

        template<typename... u_data>
        struct list_stu: public list_decompose_stu<u_data...> {
            using base = list_decompose_stu<u_data...>;

            template<basic_intpc v_pos>
            struct type_stu {
                using type = base::other::template type<v_pos - 1>;
            };

            template<basic_intpc v_pos>
            requires (v_pos == 0)
            struct type_stu<v_pos> {
                using type = base::first;
            };

            template<basic_intpc v_pos>
            using type = type_stu<v_pos>::type;
            template<typename v_in>
            using combine = list_stu<u_data..., v_in>;
            template<typename v_tem>
            using apply = v_tem::template type<u_data...>;
        };

        template<typename... u_in>
        struct type_stu {
            template<typename w_in>
            struct process_stu {
                using type = w_in;
            };

            template<basic_literal_str w_pos>
            struct process_stu<bind_arg<w_pos>> {
                using type = list_stu<u_in...>::template type<make_basic_intp<basic_intpc>(w_pos)>;
            };

            template<typename v_processed, typename v_processing, typename v_other>
            struct replace_stu {
                using type = replace_stu<typename v_processed::template combine<typename process_stu<v_processing>::type>
                , typename v_other::first, typename v_other::other>::type;
            };

            template<typename v_processed, typename v_processing>
            struct replace_stu<v_processed, v_processing, list_stu<>> {
                using type = v_processed::template combine<typename process_stu<v_processing>::type>;
            };

            using arg_list = list_stu<t_arg...>;
            using type = replace_stu<list_stu<>, typename arg_list::first
            , typename arg_list::other>::type::template apply<t_tem>;
        };
    public:
        bind() = delete;
        ~bind() = delete;

        template<typename... t_in>
        using type = type_stu<t_in...>::type;
        template<typename... t_in>
        using type_t = t<type<t_in...>>;
    };

    template<typename t_tem>
    class bind<t_tem> {
    public:
        static_assert(is_tem_cpt<t_tem> || is_bind_cpt<t_tem>);

        bind() = delete;
        ~bind() = delete;
        
        template<typename... t_in>
        using type = t_tem::template type<t_in...>;
        template<typename... t_in>
        using type_t = t<type<t_in...>>;
    };
}

#endif