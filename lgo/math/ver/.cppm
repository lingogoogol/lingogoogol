export module lgo.math.ver;

import std;

export namespace lgo {
    class ver_t {
    private:
        std::array<std::uint64_t, 4> m_data{};
    public:
        ver_t() = default;
        ver_t(std::uint64_t _1, std::uint64_t _2, std::uint64_t _3, std::uint64_t _4);
        ver_t(std::istream& istream);

        auto operator<=>(const ver_t& in) const -> std::strong_ordering;

        auto pad(std::ostream& ostream) const -> void;
    };
}
