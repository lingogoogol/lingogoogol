export module lgo.GUI.misc;

export namespace lgo {
    struct depth_range_t {
        float near{}, far{};
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
}
