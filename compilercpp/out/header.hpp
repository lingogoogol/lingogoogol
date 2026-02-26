#ifndef COMPILERCPP_OUT_HEADER
#define COMPILERCPP_OUT_HEADER

#include <iostream>
#include <cstdint>
#include <chrono>

#include "../lib/.hpp"

#include "constant.hpp"
#include "setting.hpp"

auto dos_header(code_stream& stream) -> void {
    stream.pad_int(std::uint16_t{ 0x5A4D });
    for (std::uint8_t i{ 0 }; i < 29; ++i) {
        stream.pad_int(std::uint16_t{ 0x0 });
    }
    stream.pad_address_ref("PE_signature", code_stream::resolver_overwrite_address(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_def("PE_signature");
    return;
}

auto signature(code_stream& stream) -> void {
    stream.pad_int(std::uint32_t{ 0x00004550 });
    return;
}

auto coff_file_header(code_stream& stream, std::uint16_t section_count) -> void {
    stream.pad_int(machine_type::AMD64);
    stream.pad_int(section_count);
    stream.pad_int(static_cast<std::uint32_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
    stream.pad_int(std::uint32_t{ 0x0 });
    stream.pad_int(std::uint32_t{ 0x0 });
    stream.pad_address_ref("optional_header", code_stream::resolver_overwrite_size(code_stream::raw_ptr, 2));
    stream.pad_int(std::uint16_t{});
    stream.pad_int(setting::characteristics);
    return;
}

auto optional_header_standard(code_stream& stream) -> void {
    stream.pad_int(std::uint16_t{ 0x20B });
    stream.pad_int(setting::linker_version_major);
    stream.pad_int(setting::linker_version_minor);
    stream.pad_address_ref("code", code_stream::resolver_overwrite_size_sum(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref("initialized_data", code_stream::resolver_overwrite_size_sum(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref("uninitialized_data", code_stream::resolver_overwrite_size_sum(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref("entry_point", code_stream::resolver_overwrite_address(code_stream::RVA, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref("code_begin0", code_stream::resolver_overwrite_address(code_stream::RVA, 4));
    stream.pad_int(std::uint32_t{});
    return;
}

auto optional_header_windows(code_stream& stream) -> void {
    stream.pad_int(setting::image_base);
    stream.pad_int(setting::virtual_alignment);
    stream.pad_int(setting::file_alignment);
    stream.pad_int(setting::required_operating_system_version_major);
    stream.pad_int(setting::required_operating_system_version_minor);
    stream.pad_int(setting::image_version_major);
    stream.pad_int(setting::image_version_minor);
    stream.pad_int(setting::subsystem_version_major);
    stream.pad_int(setting::subsystem_version_minor);
    stream.pad_int(std::uint32_t{ 0 });
    stream.pad_address_ref("image", code_stream::resolver_overwrite_size(code_stream::RVA, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref("header", code_stream::resolver_overwrite_size(code_stream::raw_ptr, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_int(std::uint32_t{ 0 });
    stream.pad_int(windows_subsystem::WINDOWS_CUI);
    stream.pad_int(setting::dll_characteristics);
    stream.pad_int(setting::size_stack_reserve);
    stream.pad_int(setting::size_stack_commit);
    stream.pad_int(setting::size_heap_reserve);
    stream.pad_int(setting::size_heap_commit);
    stream.pad_int(std::uint32_t{ 0 });
    stream.pad_int(std::uint32_t{ 0x10 });
    return;
}

struct data_directory {
    std::uint32_t m_virtual_address{ 0 };
    std::uint32_t m_size{ 0 };
};

auto pad_data_directory(code_stream& stream, const data_directory& in) -> void {
    stream.pad_int(in.m_virtual_address);
    stream.pad_int(in.m_size);
    return;
}

auto pad_data_directory_empty(code_stream& stream, std::string name) -> void {
    stream.pad_address_ref(name + "_begin", code_stream::resolver_overwrite_address_if_exist(code_stream::RVA, 4));
    stream.pad_int(std::uint32_t{});
    stream.pad_address_ref(name, code_stream::resolver_overwrite_size_if_exist(code_stream::RVA, 4));
    stream.pad_int(std::uint32_t{});
    return;
}

auto optional_header_data_directory(code_stream& stream) -> void {
    pad_data_directory_empty(stream, "export_table");
    pad_data_directory_empty(stream, "import_table");
    pad_data_directory_empty(stream, "resource_table");
    pad_data_directory_empty(stream, "exception_table");
    pad_data_directory_empty(stream, "certificate_table");
    pad_data_directory_empty(stream, "base_relocation_table");
    pad_data_directory(stream, { 0, 0 });
    pad_data_directory(stream, { 0, 0 });
    pad_data_directory(stream, { 0, 0 });
    pad_data_directory_empty(stream, "tls_table");
    pad_data_directory_empty(stream, "load_config_table");
    pad_data_directory(stream, { 0, 0 });
    pad_data_directory_empty(stream, "IAT");
    pad_data_directory_empty(stream, "delay_import_descriptor");
    pad_data_directory(stream, { 0, 0 });
    pad_data_directory(stream, { 0, 0 });
    return;
}

auto optional_header(code_stream& stream) -> void {
    optional_header_standard(stream);
    optional_header_windows(stream);
    optional_header_data_directory(stream);
    return;
}

#endif