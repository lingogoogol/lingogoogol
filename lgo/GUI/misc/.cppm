export module lgo.GUI.misc;

import std;

export namespace lgo
{
    struct depth_range_t
    {
        float near{}, far{};
    };

    struct color_t
    {
        float R{}, G{}, B{};
    };

    enum class alignment_x
    {
        left,
        center,
        right,
        count
    };

    enum class alignment_y
    {
        top,
        center,
        bottom,
        count
    };

    struct alignment_2D
    {
        alignment_x x{};
        alignment_y y{};
    };

    auto to_string(depth_range_t depth_range) -> std::string;
    auto to_string(color_t color) -> std::string;
}
