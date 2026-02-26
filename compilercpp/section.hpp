#ifndef COMPILERCPP_SECTION
#define COMPILERCPP_SECTION

#include <string>
#include <cstdint>
#include <iostream>
#include <vector>

#include "common.hpp"
#include "setting.hpp"
#include "constant.hpp"
#include "error.hpp"
#include "io.hpp"

struct section {
    std::string name{};
    const std::vector<unsigned char>* segment{};
    std::uint32_t raw_size{};
    std::uint32_t virtual_size{};
    std::uint32_t characteristics{};
};

struct section_header_missing_field {
    std::streampos virtual_address{};
    std::streampos pointer_to_raw_data{};
};

struct size_entry {
    std::uint32_t code{ 0 };
    std::uint32_t initialized_data{ 0 };
    std::uint32_t uninitialized_data{ 0 };
};

auto push_section(std::vector<section>& sections, const std::string& name
, std::uint32_t characteristics, const std::vector<unsigned char>& segment, std::uint32_t size, size_entry& section_size) -> void {
    if (!size) {
        return;
    }
    std::uint32_t raw_size{ roundup(static_cast<std::uint32_t>(segment.size()), setting::file_alignment) };
    if (characteristics & section_flag::CNT_CODE) {
        section_size.code += raw_size;
    }
    else if (characteristics & section_flag::CNT_INITIALIZED_DATA) {
        section_size.initialized_data += raw_size;
    }
    else if (characteristics & section_flag::CNT_UNINITIALIZED_DATA) {
        section_size.uninitialized_data += raw_size;
    }
    else {
        throw internal_error{ "section flag" };
    }
    sections.push_back(section{ name, &segment, raw_size, size, characteristics });
    return;
}

[[nodiscard]]
auto pad_section_header(std::ostream& ostream, const section& in) -> section_header_missing_field {
    if (in.name.size() > 8) {
        throw internal_error{ "section name length" };
    }
    ostream.write(in.name.data(), 8);
    pad_integer(ostream, in.virtual_size);
    std::streampos virtual_address{ ostream.tellp() };
    pad_integer(ostream, std::uint32_t{});
    pad_integer(ostream, in.raw_size);
    std::streampos pointer_to_raw_data{ ostream.tellp() };
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, std::uint16_t{ 0 });
    pad_integer(ostream, in.characteristics);
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
    fill_integer(ostream, missing_field.pointer_to_raw_data, static_cast<std::uint32_t>(ostream.tellp()));
    fill_integer(ostream, missing_field.virtual_address, virtual_address);
    for (std::uint32_t i{ 0 }; i < in.segment->size(); ++i) {
        ostream.put((*in.segment)[i]);
    }
    virtual_address += static_cast<std::uint32_t>(in.segment->size());
    align_file(ostream, virtual_address);
    return;
}

#endif