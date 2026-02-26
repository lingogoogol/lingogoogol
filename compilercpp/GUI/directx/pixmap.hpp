#ifndef COMPILERCPP_GUI_DIRECTX_PIXMAP
#define COMPILERCPP_GUI_DIRECTX_PIXMAP

#include <vector>
#include <cstddef>

#include "../../lib/common.hpp"

class pixmap_t {
public:
    using size_t = std::uint64_t;

    pixmap_t();
    pixmap_t(size_2D size);
    pixmap_t(size_t x, size_t y);
    pixmap_t(pixmap_t&& in);

    auto size_get() const -> size_2D;
    auto size_set(size_2D size) -> void;
    auto size_set(size_t x, size_t y) -> void;
    auto size_1D_get() const -> size_t;

    auto operator[](size_t xy) -> std::byte&;
    auto operator[](size_t xy) const -> const std::byte&;
    auto operator[](size_t x, size_t y) -> std::byte&;
    auto operator[](size_t x, size_t y) const -> const std::byte&;
private:
    //Set buffer size without changing m_size.
    auto size_set1(size_2D size) -> void;

    size_2D m_size{};
    std::vector<std::byte> m_buffer{};
};

pixmap_t::pixmap_t(): pixmap_t{ 0, 0 } {}

pixmap_t::pixmap_t(size_2D size): m_size{ size } {
    size_set1(size);
    return;
}

pixmap_t::pixmap_t(size_t x, size_t y): pixmap_t{ size_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) } } {}

pixmap_t::pixmap_t(pixmap_t&& in): m_size{ in.m_size }, m_buffer{ std::move(in.m_buffer) } {}

auto pixmap_t::size_get() const -> size_2D {
    return m_size;
}

auto pixmap_t::size_set(size_2D size) -> void {
    m_size.x = size.x;
    m_size.y = size.y;
    size_set1(size);
    return;
}

auto pixmap_t::size_set(size_t x, size_t y) -> void {
    size_set(size_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) });
    return;
}

auto pixmap_t::size_1D_get() const -> size_t {
    return m_size.x * m_size.y;
}

auto pixmap_t::operator[](size_t xy) -> std::byte& {
    return m_buffer[xy];
}

auto pixmap_t::operator[](size_t xy) const -> const std::byte& {
    return m_buffer[xy];
}

auto pixmap_t::operator[](size_t x, size_t y) -> std::byte& {
    return m_buffer[m_size.x * y + x];
}

auto pixmap_t::operator[](size_t x, size_t y) const -> const std::byte& {
    return m_buffer[m_size.x * y + x];
}

auto pixmap_t::size_set1(size_2D size) -> void {
    m_buffer.resize(size.x * size.y);
    return;
}

#endif