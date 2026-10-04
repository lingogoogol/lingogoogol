export module lgo.math.vec;

import std;

export namespace lgo
{
    class size_2D;

    class pos_2D
    {
    public:
        std::int64_t x{}, y{};

        auto operator==(const pos_2D& in) const -> bool;
        auto operator+(size_2D offset) -> pos_2D;
        auto operator-(size_2D offset) -> pos_2D;
        auto operator-(pos_2D pos) -> size_2D;
    };

    struct size_1D
    {
        std::int64_t x{};
    };

    class size_2D
    {
    public:
        std::int64_t x{}, y{};

        auto operator==(const size_2D& in) const -> bool;
        auto operator+(size_2D in) -> size_2D;
        auto operator-(size_2D in) -> size_2D;
    };

    auto to_string(pos_2D pos) -> std::string;
    auto to_string(size_1D size) -> std::string;
    auto to_string(size_2D size) -> std::string;
}
