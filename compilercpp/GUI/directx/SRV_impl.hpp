#ifndef COMPILERCPP_GUI_DIRECTX_SRV_IMPL
#define COMPILERCPP_GUI_DIRECTX_SRV_IMPL

#include "SRV_def.hpp"
#include "descriptor_heap_def.hpp"

auto SRV_t::descriptor_heap_get() const -> const descriptor_heap_t& {
    return *m_descriptor_heap;
}

auto SRV_t::descriptor_heap_index_get() const -> std::uint64_t {
    return m_descriptor_heap_index;
}

auto SRV_t::handle_CPU_get() const -> D3D12_CPU_DESCRIPTOR_HANDLE {
    return m_descriptor_heap->descriptor_handle_CPU_get(m_descriptor_heap_index);
}

auto SRV_t::handle_GPU_get() const -> D3D12_GPU_DESCRIPTOR_HANDLE {
    return m_descriptor_heap->descriptor_handle_GPU_get(m_descriptor_heap_index);
}

#endif