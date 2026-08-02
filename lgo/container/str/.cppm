export module lgo.container.str;

import std;

export namespace lgo {
    auto to_wstring(std::string in) -> std::wstring;

    template<std::integral t_in>
    auto to_wstring(t_in in) -> std::wstring {
        return to_wstring(in, sizeof in);
    }

    template<std::integral t_in>
    auto to_wstring(t_in in, std::uint8_t size) -> std::wstring {
        std::wstring temp{}, out{};
        for (std::uint8_t i{ 0 }; i < size; ++i) {
            for (std::uint8_t j{ 0 }; j < 2; ++j) {
                if (in % 0x10 < 0xA) {
                    temp.push_back(L'0' + in % 0x10);
                }
                else {
                    temp.push_back(L'A' + in % 0x10 - 0xA);
                }
                in >>= 4;
            }
        }
        out.insert(out.begin(), temp.rbegin(), temp.rend());
        return out;
    }

    template<std::unsigned_integral t_out>
    auto to_int(std::wstring in) -> t_out {
        return static_cast<t_out>(std::stoull(in, nullptr, 0x10));
    }

    auto to_string(std::wstring in) -> std::string;
}
