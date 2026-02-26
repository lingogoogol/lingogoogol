#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_LEN
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_LEN

#include "../../../../type/fun/pack_size.hpp"

namespace lgo {
    template<typename t_disambiguater, typename... t_type>
    class arrmc_len_private {
    protected:
        constexpr static auto len_private() -> intpc;
    };

    template<typename t_disambiguater, typename... t_type>
    constexpr auto arrmc_len_private<t_disambiguater, t_type...>::len_private() -> intpc {
        return pack_size<t_type...>;
    }

    template<typename t_disambiguater, typename... t_type>
    class arrmc_len: private arrmc_len_private<arrmc_len<t_disambiguater, t_type...>, t_type...> {
    private:
        using base = arrmc_len_private<arrmc_len, t_type...>;
    public:
        constexpr static auto len() -> intpc;
    };

    template<typename t_disambiguater, typename... t_type>
    constexpr auto arrmc_len<t_disambiguater, t_type...>::len() -> intpc {
        return base::len_private();
    }
}

#endif