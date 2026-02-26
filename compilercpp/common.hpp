#ifndef COMPILERCPP_COMMON
#define COMPILERCPP_COMMON

template<typename t_in>
auto divup(t_in in1, t_in in2) -> t_in {
    return 1 + (in1 - 1) / in2;
}

template<typename t_in>
auto roundup(t_in in1, t_in in2) -> t_in {
    return divup(in1, in2) * in2;
}

#endif