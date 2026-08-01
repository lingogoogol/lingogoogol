module lgo.math.vec;

namespace lgo {
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
}
