#ifndef COMPILERCPP_OUT_HEADER
#define COMPILERCPP_OUT_HEADER

#include <iostream>
#include <cstdint>
#include <chrono>

#include "../lib/.hpp"

#include "constant.hpp"
#include "setting.hpp"

auto dos_header(std::ostream& ostream) -> void {
    pad_integer(ostream, std::uint16_t{ 0x5A4D });
    for (std::uint8_t i{ 0 }; i < 29; ++i) {
        pad_integer(ostream, std::uint16_t{ 0x0 });
    }
    std::streampos LFA_new{ ostream.tellp() };
    pad_integer(ostream, std::uint32_t{});
    fill_integer(ostream, LFA_new, static_cast<std::uint32_t>(ostream.tellp()));
    return;
}

auto signature(std::ostream& ostream) -> void {
    pad_integer(ostream, std::uint32_t{ 0x00004550 });
    return;
}

struct coff_file_header_missing_field {
    std::streampos m_number_of_sections{};
    std::streampos m_size_of_optional_header{};
};

[[nodiscard]]
auto coff_file_header(std::ostream& ostream) -> coff_file_header_missing_field {
    pad_integer(ostream, machine_type::AMD64);
    const std::streampos number_of_sections{ ostream.tellp() };
    pad_integer(ostream, std::uint16_t{});
    pad_integer(ostream, static_cast<std::uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0 });
    const std::streampos size_of_optional_header{ ostream.tellp() };
    pad_integer(ostream, std::uint16_t{});
    pad_integer(ostream, setting::characteristics);
    return coff_file_header_missing_field{ number_of_sections, size_of_optional_header };
}

struct optional_header_missing_field {
    std::streampos m_size_of_code{};
    std::streampos m_size_of_initialized_data{};
    std::streampos m_size_of_uninitialized_data{};
    std::streampos m_address_of_entry_point{};
    std::streampos m_base_of_code{};
    
    std::streampos m_size_of_image{};
    std::streampos m_size_of_headers{};

    std::streampos m_export_table{};
    std::streampos m_import_table{};
    std::streampos m_resource_table{};
    std::streampos m_exception_table{};
    std::streampos m_certificate_table{};
    std::streampos m_base_relocation_table{};
    std::streampos m_tls_table{};
    std::streampos m_load_config_table{};
    std::streampos m_IAT{};
    std::streampos m_delay_import_descriptor{};
};

auto optional_header_standard(std::ostream& ostream, optional_header_missing_field& missing_field) -> void {
    pad_integer(ostream, std::uint16_t{ 0x20B });
    pad_integer(ostream, setting::linker_version_major);
    pad_integer(ostream, setting::linker_version_minor);
    missing_field.m_size_of_code = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    missing_field.m_size_of_initialized_data = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    missing_field.m_size_of_uninitialized_data = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    missing_field.m_address_of_entry_point = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    missing_field.m_base_of_code = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    return;
}

auto optional_header_windows(std::ostream& ostream, optional_header_missing_field& missing_field) -> void {
    pad_integer(ostream, setting::image_base);
    pad_integer(ostream, setting::virtual_alignment);
    pad_integer(ostream, setting::file_alignment);
    pad_integer(ostream, setting::required_operating_system_version_major);
    pad_integer(ostream, setting::required_operating_system_version_minor);
    pad_integer(ostream, setting::image_version_major);
    pad_integer(ostream, setting::image_version_minor);
    pad_integer(ostream, setting::subsystem_version_major);
    pad_integer(ostream, setting::subsystem_version_minor);
    pad_integer(ostream, std::uint32_t{ 0 });
    missing_field.m_size_of_image = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    missing_field.m_size_of_headers = ostream.tellp();
    pad_integer(ostream, std::uint32_t{});
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, windows_subsystem::WINDOWS_CUI);
    pad_integer(ostream, setting::dll_characteristics);
    pad_integer(ostream, setting::size_stack_reserve);
    pad_integer(ostream, setting::size_stack_commit);
    pad_integer(ostream, setting::size_heap_reserve);
    pad_integer(ostream, setting::size_heap_commit);
    pad_integer(ostream, std::uint32_t{ 0 });
    pad_integer(ostream, std::uint32_t{ 0x10 });
    return;
}

struct data_directory {
    std::uint32_t m_virtual_address{ 0 };
    std::uint32_t m_size{ 0 };
};

auto pad_data_directory(std::ostream& ostream, const data_directory& in) -> void {
    pad_integer(ostream, in.m_virtual_address);
    pad_integer(ostream, in.m_size);
    return;
}

auto optional_header_data_directory(std::ostream& ostream, optional_header_missing_field& missing_field) -> void {
    missing_field.m_export_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_import_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_resource_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_exception_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_certificate_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_base_relocation_table = ostream.tellp();
    pad_data_directory(ostream, {});
    pad_data_directory(ostream, { 0, 0 });
    pad_data_directory(ostream, { 0, 0 });
    pad_data_directory(ostream, { 0, 0 });
    missing_field.m_tls_table = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_load_config_table = ostream.tellp();
    pad_data_directory(ostream, {});
    pad_data_directory(ostream, { 0, 0 });
    missing_field.m_IAT = ostream.tellp();
    pad_data_directory(ostream, {});
    missing_field.m_delay_import_descriptor = ostream.tellp();
    pad_data_directory(ostream, {});
    pad_data_directory(ostream, { 0, 0 });
    pad_data_directory(ostream, { 0, 0 });
    return;
}

[[nodiscard]]
auto optional_header(std::ostream& ostream) -> optional_header_missing_field {
    optional_header_missing_field missing_field{};
    optional_header_standard(ostream, missing_field);
    optional_header_windows(ostream, missing_field);
    optional_header_data_directory(ostream, missing_field);
    return missing_field;
}

#endif