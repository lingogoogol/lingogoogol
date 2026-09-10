module;

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>

export module external.objbase;

import std;

template<typename t_type>
auto IID_PPV_ARGS_export(t_type* in) -> auto {
    return std::pair{ IID_PPV_ARGS(in) };
}
#undef IID_PPV_ARGS

export
{
    using ::CoCreateInstance;
    
    template<typename t_type>
    auto IID_PPV_ARGS0(t_type* in) -> auto {
        return IID_PPV_ARGS_export(in).first;
    }
    
    template<typename t_type>
    auto IID_PPV_ARGS1(t_type* in) -> auto {
        return IID_PPV_ARGS_export(in).second;
    }
}
