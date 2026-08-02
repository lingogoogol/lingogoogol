module lgo.math.op;

import lgo.math.vec;

namespace lgo {
    auto inside(pos_2D rect_pos, size_2D rect_size, pos_2D point_pos) -> bool {
        return point_pos.x >= rect_pos.x && point_pos.x < rect_pos.x + rect_size.x
        && point_pos.y >= rect_pos.y && point_pos.y < rect_pos.y + rect_size.y;
    }
}
