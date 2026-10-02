export module lgo.GUI.texture:pixmap;

import std;

import lgo.math.vec;

export namespace lgo
{
    class pixmap_t
    {
    public:
        using size_t = std::uint64_t;

        pixmap_t();
        pixmap_t(size_2D size);
        pixmap_t(size_t x, size_t y);
        pixmap_t(pixmap_t&& in);

        auto dim_get() const -> size_2D;
        auto dim_set(size_2D size) -> void;
        auto dim_set(size_t x, size_t y) -> void;
        auto pixel_count() const -> size_t;

        auto operator[](size_t xy) -> std::byte&;
        auto operator[](size_t xy) const -> const std::byte&;
        auto operator[](size_t x, size_t y) -> std::byte&;
        auto operator[](size_t x, size_t y) const -> const std::byte&;
    private:
        //Set buffer size without changing m_size.
        auto dim_set1(size_2D size) -> void;

        size_2D m_size{};
        std::vector<std::byte> m_buffer{};
    };
}
