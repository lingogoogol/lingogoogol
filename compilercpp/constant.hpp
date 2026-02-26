#ifndef COMPILERCPP_CONSTANT
#define COMPILERCPP_CONSTANT

#include <cstdint>

namespace machine_type {
    constexpr inline std::uint16_t AMD64{ 0x8664 };
}

namespace characteristic {
    constexpr inline std::uint16_t EXECUTABLE_IMAGE{ 0x0002 };
    constexpr inline std::uint16_t LARGE_ADDRESS_AWARE{ 0x0020 };
}

namespace windows_subsystem {
    constexpr inline std::uint16_t WINDOWS_CUI{ 3 };
}

namespace dll_characteristic {
    constexpr inline std::uint16_t HIGH_ENTROPY_VA{ 0x0020 };
    constexpr inline std::uint16_t DYNAMIC_BASE{ 0x0040 };
    constexpr inline std::uint16_t NX_COMPAT{ 0x0100 };
    constexpr inline std::uint16_t TERMINAL_SERVER_AWARE{ 0x8000 };
};

namespace section_flag {
    constexpr inline std::uint32_t CNT_CODE{ 0x00000020 };
    constexpr inline std::uint32_t CNT_INITIALIZED_DATA{ 0x00000040 };
    constexpr inline std::uint32_t CNT_UNINITIALIZED_DATA{ 0x00000080 };
    constexpr inline std::uint32_t GPREL{ 0x00008000 };
    constexpr inline std::uint32_t LNK_NRELOC_OVFL{ 0x01000000 };
    constexpr inline std::uint32_t MEM_DISCARDABLE{ 0x02000000 };
    constexpr inline std::uint32_t MEM_NOT_CACHED{ 0x04000000 };
    constexpr inline std::uint32_t MEM_NOT_PAGED{ 0x08000000 };
    constexpr inline std::uint32_t MEM_SHARED{ 0x10000000 };
    constexpr inline std::uint32_t MEM_EXECUTE{ 0x20000000 };
    constexpr inline std::uint32_t MEM_READ{ 0x40000000 };
    constexpr inline std::uint32_t MEM_WRITE{ 0x80000000 };
}

#endif