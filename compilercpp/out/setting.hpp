#ifndef COMPILERCPP_OUT_SETTING
#define COMPILERCPP_OUT_SETTING

#include <cstdint>

#include "constant.hpp"

namespace setting {
    constexpr inline std::uint16_t characteristics{ characteristic::EXECUTABLE_IMAGE | characteristic::LARGE_ADDRESS_AWARE };
    constexpr inline std::uint8_t linker_version_major{ 0 };
    constexpr inline std::uint8_t linker_version_minor{ 0 };
    constexpr inline std::uint64_t image_base{ 0x400000 };
    constexpr inline std::uint32_t virtual_alignment{ 0x1000 };
    constexpr inline std::uint32_t file_alignment{ 0x200 };
    constexpr inline std::uint16_t required_operating_system_version_major{ 10 };
    constexpr inline std::uint16_t required_operating_system_version_minor{ 0 };
    constexpr inline std::uint16_t image_version_major{ 0 };
    constexpr inline std::uint16_t image_version_minor{ 0 };
    constexpr inline std::uint16_t subsystem_version_major{ 6 };
    constexpr inline std::uint16_t subsystem_version_minor{ 0 };
    constexpr inline std::uint16_t dll_characteristics{ dll_characteristic::HIGH_ENTROPY_VA
    | dll_characteristic::DYNAMIC_BASE | dll_characteristic::NX_COMPAT | dll_characteristic::TERMINAL_SERVER_AWARE };
    constexpr inline std::uint64_t size_stack_reserve{ 0x100000 };
    constexpr inline std::uint64_t size_stack_commit{ 0x1000 };
    constexpr inline std::uint64_t size_heap_reserve{ 0x100000 };
    constexpr inline std::uint64_t size_heap_commit{ 0x1000 };
}

#endif