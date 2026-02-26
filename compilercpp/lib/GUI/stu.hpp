#ifndef COMPILERCPP_LIB_GUI_STU
#define COMPILERCPP_LIB_GUI_STU

#include <cstdint>

struct pos_2D {
    std::int64_t x{}, y{};
};

struct depth_range_t {
    float near{}, far{};
};

struct size_1D {
    std::uint64_t x{};
};

struct size_2D {
    std::uint64_t x{}, y{};
};

struct color_t {
    float R{}, G{}, B{};
};

enum class alignment_x {
    left,
    center,
    right,
    count
};

enum class alignment_y {
    top,
    center,
    bottom,
    count
};

struct alignment_2D {
    alignment_x x{};
    alignment_y y{};
};

#endif