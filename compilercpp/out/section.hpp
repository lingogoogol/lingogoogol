#ifndef COMPILERCPP_OUT_SECTION
#define COMPILERCPP_OUT_SECTION

#include <string>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../lib/.hpp"

#include "setting.hpp"
#include "constant.hpp"

struct section_t {
    std::string m_name{};
    const code_stream* m_segment{};
    std::uint32_t m_characteristic{};
};

auto pad_section_header(code_stream& stream, const section_t& in) -> void {
    if (in.m_name.size() > 8) {
        throw internal_error_t{ "PE檔段落名稱太長" };
    }
    for (std::size_t i{ 0 }; i < in.m_name.size(); ++i) {
        stream.pad_byte(static_cast<unsigned char>(in.m_name[i]));
    }
    for (std::size_t i{ in.m_name.size() }; i < 8; ++i) {
        stream.pad_byte(0);
    }
    stream.pad_address_ref(in.m_name, code_stream::resolver_overwrite_size(code_stream::VA, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref(in.m_name + "_begin", code_stream::resolver_overwrite_address(code_stream::VA, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref(in.m_name + "_aligned", code_stream::resolver_overwrite_size(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref(in.m_name + "_begin", code_stream::resolver_overwrite_address(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_int(std::uint32_t{ 0 });
    stream.pad_int(std::uint32_t{ 0 });
    stream.pad_int(std::uint16_t{ 0 });
    stream.pad_int(std::uint16_t{ 0 });
    stream.pad_int(in.m_characteristic);
    return;
}

#endif