#ifndef COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_TABLE
#define COMPILERCPP_GUI_DIRECTX_DESCRIPTOR_TABLE

#include <vector>

#include "../../lib/header.hpp"

class descriptor_table_t {
public:
    descriptor_table_t() = default;
    template<typename t_descriptor_table>
    descriptor_table_t(t_descriptor_table&& descriptor_table);
    
    //offset: The offset in descriptors from the start of the descriptor table. Can be D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND.
    //count: Use UINT_MAX to specify unbounded size.
    auto range_add(D3D12_DESCRIPTOR_RANGE_TYPE type
    , std::uint32_t offset, std::uint32_t reg_space, std::uint32_t reg_base, std::uint32_t count) -> void;
    auto desc_get() const -> D3D12_ROOT_DESCRIPTOR_TABLE1;
private:
    std::vector<D3D12_DESCRIPTOR_RANGE1> m_range{};
};

template<typename t_descriptor_table>
descriptor_table_t::descriptor_table_t(t_descriptor_table&& descriptor_table)
: m_range{ std::forward<t_descriptor_table&&>(descriptor_table).m_range } {}

auto descriptor_table_t::range_add(D3D12_DESCRIPTOR_RANGE_TYPE type
, std::uint32_t offset, std::uint32_t reg_space, std::uint32_t reg_base, std::uint32_t count) -> void {
    D3D12_DESCRIPTOR_RANGE1 range{};
    range.RangeType = type;
    range.OffsetInDescriptorsFromTableStart = offset;
    range.RegisterSpace = reg_space;
    range.BaseShaderRegister = reg_base;
    range.NumDescriptors = count;
    range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    m_range.push_back(range);
    return;
}

auto descriptor_table_t::desc_get() const -> D3D12_ROOT_DESCRIPTOR_TABLE1 {
    D3D12_ROOT_DESCRIPTOR_TABLE1 desc{};
    desc.pDescriptorRanges = m_range.data();
    desc.NumDescriptorRanges = static_cast<UINT>(m_range.size());
    return desc;
}

#endif