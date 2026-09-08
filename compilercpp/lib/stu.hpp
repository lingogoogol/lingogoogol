#ifndef COMPILERCPP_LIB_STU
#define COMPILERCPP_LIB_STU

#include <cstdint>

class size_2D;

class pos_2D {
public:
    std::int64_t x{}, y{};

    auto operator==(const pos_2D& in) const -> bool;
    auto operator+(size_2D offset) -> pos_2D;
    auto operator-(size_2D offset) -> pos_2D;
    auto operator-(pos_2D pos) -> size_2D;
};

struct depth_range_t {
    float near{}, far{};
};

struct size_1D {
    std::int64_t x{};
};

class size_2D {
public:
    std::int64_t x{}, y{};

    auto operator==(const size_2D& in) const -> bool;
    auto operator+(size_2D in) -> size_2D;
    auto operator-(size_2D in) -> size_2D;
};

auto pos_2D::operator==(const pos_2D& in) const -> bool {
    return x == in.x && y == in.y;
}

auto pos_2D::operator+(size_2D offset) -> pos_2D {
    return pos_2D{ x + offset.x, y + offset.y };
}

auto pos_2D::operator-(size_2D offset) -> pos_2D {
    return pos_2D{ x - offset.x, y - offset.y };
}

auto pos_2D::operator-(pos_2D pos) -> size_2D {
    return size_2D{ x - pos.x, y - pos.y };
}

auto size_2D::operator==(const size_2D& in) const -> bool {
    return x == in.x && y == in.y;
}

auto size_2D::operator+(size_2D in) -> size_2D {
    return size_2D{ x + in.x, y + in.y };
}

auto size_2D::operator-(size_2D in) -> size_2D {
    return size_2D{ x - in.x, y - in.y };
}

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
