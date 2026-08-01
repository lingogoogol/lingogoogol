module lgo.math.ver;

import std;

namespace lgo {
    ver_t::ver_t(std::uint64_t _1, std::uint64_t _2, std::uint64_t _3, std::uint64_t _4): m_data{ _1, _2, _3, _4 } {}

    ver_t::ver_t(std::istream& istream) {
        for (std::size_t i{ 0 }; i < m_data.size(); ++i) {
            m_data[i] = get_integer<std::uint64_t>(istream);
        }
        return;
    }

    auto ver_t::operator<=>(const ver_t& in) const -> std::strong_ordering {
        for (std::size_t i{ 0 }; i < m_data.size(); ++i) {
            if (m_data[i] > in.m_data[i]) {
                return std::strong_ordering::greater;
            }
            else if (m_data[i] < in.m_data[i]) {
                return std::strong_ordering::less;
            }
        }
        return std::strong_ordering::equal;
    }

    auto ver_t::pad(std::ostream& ostream) const -> void {
        for (std::size_t i{ 0 }; i < m_data.size(); ++i) {
            pad_integer<std::uint64_t>(ostream, m_data[i]);
        }
        return;
    }
}
