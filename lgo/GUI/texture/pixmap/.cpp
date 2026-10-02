module lgo.GUI.texture;

import std;

import lgo.math.vec;

namespace lgo
{
    pixmap_t::pixmap_t(): pixmap_t{ 0, 0 } {}

    pixmap_t::pixmap_t(size_2D size): m_size{ size }
    {
        dim_set1(size);
        return;
    }

    pixmap_t::pixmap_t(size_t x, size_t y)
    : pixmap_t{ size_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) } }
    {}

    pixmap_t::pixmap_t(pixmap_t&& in): m_size{ in.m_size }, m_buffer{ std::move(in.m_buffer) } {}

    auto pixmap_t::dim_get() const -> size_2D
    {
        return m_size;
    }

    auto pixmap_t::dim_set(size_2D size) -> void
    {
        m_size.x = size.x;
        m_size.y = size.y;
        dim_set1(size);
        return;
    }

    auto pixmap_t::dim_set(size_t x, size_t y) -> void
    {
        dim_set(size_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) });
        return;
    }

    auto pixmap_t::pixel_count() const -> size_t
    {
        return m_size.x * m_size.y;
    }

    auto pixmap_t::operator[](size_t xy) -> std::byte&
    {
        return m_buffer[xy];
    }

    auto pixmap_t::operator[](size_t xy) const -> const std::byte&
    {
        return m_buffer[xy];
    }

    auto pixmap_t::operator[](size_t x, size_t y) -> std::byte&
    {
        return m_buffer[m_size.x * y + x];
    }

    auto pixmap_t::operator[](size_t x, size_t y) const -> const std::byte&
    {
        return m_buffer[m_size.x * y + x];
    }

    auto pixmap_t::dim_set1(size_2D size) -> void
    {
        m_buffer.resize(size.x * size.y);
        return;
    }
}
