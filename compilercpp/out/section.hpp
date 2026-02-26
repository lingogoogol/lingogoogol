#ifndef COMPILERCPP_OUT_SECTION
#define COMPILERCPP_OUT_SECTION

#include <string>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../lib/.hpp"

#include "setting.hpp"
#include "constant.hpp"

struct section {
    std::string m_name{};
    const std::vector<unsigned char>* m_segment{};
    std::uint32_t m_raw_size{};
    std::uint32_t m_virtual_size{};
    std::uint32_t m_characteristics{};
};

struct section_header_missing_field {
    std::streampos m_virtual_address{};
    std::streampos m_pointer_to_raw_data{};
};

struct size_entry {
    std::uint32_t m_code{ 0 };
    std::uint32_t m_initialized_data{ 0 };
    std::uint32_t m_uninitialized_data{ 0 };
};

auto push_section(std::vector<section>& sections, const std::string& name
, std::uint32_t characteristics, const std::vector<unsigned char>& segment, std::uint32_t size, size_entry& section_size) -> void {
    if (!size) {
        return;
    }
    std::uint32_t raw_size{ roundup(static_cast<std::uint32_t>(segment.size()), setting::file_alignment) };
    if (characteristics & section_flag::CNT_CODE) {
        section_size.m_code += raw_size;
    }
    else if (characteristics & section_flag::CNT_INITIALIZED_DATA) {
        section_size.m_initialized_data += raw_size;
    }
    else if (characteristics & section_flag::CNT_UNINITIALIZED_DATA) {
        section_size.m_uninitialized_data += raw_size;
    }
    else {
        throw internal_error{ "section flag" };
    }
    sections.push_back(section{ name, &segment, raw_size, size, characteristics });
    return;
}

[[nodiscard]]
auto pad_section_header(std::ostream& ostream, const section& in) -> section_header_missing_field {
    if (in.m_name.size() > 8) {
        throw internal_error{ "section name length" };
    }
    ostream.write(in.m_name.data(), 8);
    pad_integer(ostream, in.m_virtual_size);
    std::streampos virtual_address{ ostream.tellp() };
    pad_integer(ostream, std::uint32_t{});
    pad_integer(ostream, in.m_raw_size);
    std::streampos pointer_to_raw_data{ ostream.tellp() };
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, in.m_characteristics);
    return section_header_missing_field{ virtual_address, pointer_to_raw_data };
}

auto align_file(std::ostream& ostream, std::uint32_t& virtual_address) -> void {
    std::uint32_t file_address_new{ roundup(static_cast<std::uint32_t>(ostream.tellp()), setting::file_alignment) };
    std::uint32_t count{ file_address_new - static_cast<std::uint32_t>(ostream.tellp()) };
    for (std::uint32_t i{ 0 }; i < count; ++i) {
        ostream.put(0);
    }
    virtual_address = roundup(virtual_address, setting::virtual_alignment);
    return;
}

auto pad_section(std::ostream& ostream, const section& in, section_header_missing_field& missing_field, std::uint32_t& virtual_address) -> void {
    fill_integer(ostream, missing_field.m_pointer_to_raw_data, static_cast<std::uint32_t>(ostream.tellp()));
    fill_integer(ostream, missing_field.m_virtual_address, virtual_address);
    for (std::uint32_t i{ 0 }; i < in.m_segment->size(); ++i) {
        ostream.put((*in.m_segment)[i]);
    }
    virtual_address += static_cast<std::uint32_t>(in.m_segment->size());
    align_file(ostream, virtual_address);
    return;
}

#endif