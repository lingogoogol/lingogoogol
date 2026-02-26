#ifndef COMPILERCPP_LIB_COMMON
#define COMPILERCPP_LIB_COMMON

#include "stu.hpp"

template<typename t_in>
auto divup(t_in in1, t_in in2) -> t_in {
    return 1 + (in1 - 1) / in2;
}

template<typename t_in>
auto roundup(t_in in1, t_in in2) -> t_in {
    return divup(in1, in2) * in2;
}

auto inside(pos_2D rect_pos, size_2D rect_size, pos_2D point_pos) -> bool {
    return point_pos.x >= rect_pos.x && point_pos.x < rect_pos.x + rect_size.x && point_pos.y >= rect_pos.y && point_pos.y < rect_pos.y + rect_size.y;
}

#endif