#ifndef COMPILERCPP_GUI_DIRECTX_SRV_DEF
#define COMPILERCPP_GUI_DIRECTX_SRV_DEF

#include <cstdint>

#include "descriptor_heap_decl.hpp"
#include "texture.hpp"

class SRV_t {
public:
    SRV_t() = default;

    auto descriptor_heap_index_get() const -> std::uint64_t;
    auto handle_CPU_get() const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto handle_GPU_get() const -> D3D12_GPU_DESCRIPTOR_HANDLE;
private:
    const descriptor_heap_t* m_descriptor_heap{};
    std::uint64_t m_descriptor_heap_index{};
    const texture_t* m_texture{};

    friend class descriptor_heap_t;
};

#endif