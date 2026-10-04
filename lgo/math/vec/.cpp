module lgo.math.vec;

namespace lgo {
    auto pos_2D::operator==(const pos_2D& in) const -> bool
    {
        return x == in.x && y == in.y;
    }

    auto pos_2D::operator+(size_2D offset) -> pos_2D
    {
        return pos_2D{ x + offset.x, y + offset.y };
    }

    auto pos_2D::operator-(size_2D offset) -> pos_2D
    {
        return pos_2D{ x - offset.x, y - offset.y };
    }

    auto pos_2D::operator-(pos_2D pos) -> size_2D
    {
        return size_2D{ x - pos.x, y - pos.y };
    }

    auto size_2D::operator==(const size_2D& in) const -> bool
    {
        return x == in.x && y == in.y;
    }

    auto size_2D::operator+(size_2D in) -> size_2D
    {
        return size_2D{ x + in.x, y + in.y };
    }

    auto size_2D::operator-(size_2D in) -> size_2D
    {
        return size_2D{ x - in.x, y - in.y };
    }

    auto to_string(pos_2D pos) -> std::string
    {
        return "(" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")";
    }

    auto to_string(size_1D size) -> std::string
    {
        return "(" + std::to_string(size.x) + ")";
    }

    auto to_string(size_2D size) -> std::string
    {
        return "(" + std::to_string(size.x) + ", " + std::to_string(size.y) + ")";
    }
}
