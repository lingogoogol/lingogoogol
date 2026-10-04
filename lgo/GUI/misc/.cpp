module lgo.GUI.misc;

import std;

namespace lgo
{
    auto to_string(depth_range_t depth_range) -> std::string
    {
        return "(" + std::to_string(depth_range.near) + ", " + std::to_string(depth_range.far) + ")";
    }

    auto to_string(color_t color) -> std::string
    {
        return "(" + std::to_string(color.R) + ", " +
            std::to_string(color.B) + ", " +
            std::to_string(color.B) + ")";
    }
}
