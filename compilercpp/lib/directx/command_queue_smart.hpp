#ifndef COMPILERCPP_LIB_DIRECTX_COMMAND_QUEUE_SMART
#define COMPILERCPP_LIB_DIRECTX_COMMAND_QUEUE_SMART

#include <queue>
#include <map>
#include <utility>
#include "../file.hpp"
#include "info_queue.hpp"

#include "../header/Windows.h"
#include "../header/wrl_client.h"
#include "../header/d3d12.h"
#include "../header/dxgi1_6.h"

class command_queue_smart {
private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_device{};
    D3D12_COMMAND_LIST_TYPE m_type{};
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue{};
    std::map<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
    , Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocator_map{};
    std::queue<std::pair<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
    , UINT64>> m_allocator_executing{};
    std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> m_list_unused{};
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence{};
    HANDLE m_event{};
    UINT64 m_fence_value{ 0 };
public:
    command_queue_smart() {}

    auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) -> void {
        m_device = device;
        m_type = type;
        D3D12_COMMAND_QUEUE_DESC command_queue_description{};
        command_queue_description.Type = type;
        command_queue_description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        command_queue_description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        command_queue_description.NodeMask = 0;
        m_device->CreateCommandQueue(&command_queue_description, IID_PPV_ARGS(&m_queue));
        m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        m_event = CreateEventW(nullptr, false, false, nullptr);
        return;
    }

    auto set_fence() -> UINT64 {
        ++m_fence_value;
        m_queue->Signal(m_fence.Get(), m_fence_value);
        return m_fence_value;
    }

    auto check_fence(UINT64 fence_value) -> bool {
        return m_fence->GetCompletedValue() >= fence_value;
    }

    auto wait_fence(UINT64 fence_value) -> void {
        m_fence->SetEventOnCompletion(fence_value, m_event);
        WaitForSingleObject(m_event, INFINITE);
        return;
    }

    auto flush() -> void {
        wait_fence(set_fence());
        return;
    }

    auto create_list() -> Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator{};
        if (!m_allocator_executing.empty() && check_fence(m_allocator_executing.front().second)) {
            allocator = m_allocator_executing.front().first;
            m_allocator_executing.pop();
            allocator->Reset();
        }
        else {
            m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&allocator));
        }
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list{};
        if (m_list_unused.size()) {
            list = m_list_unused.front();
            m_list_unused.pop();
            list->Reset(allocator.Get(), nullptr);
        }
        else {
            m_device->CreateCommandList(0, m_type, allocator.Get(), nullptr, IID_PPV_ARGS(&list));
        }
        m_allocator_map.emplace(list, allocator);
        return list;
    }

    auto execute_list(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list) -> UINT64 {
        list->Close();
        ID3D12CommandList* list_list[]{ list.Get() };
        m_queue->ExecuteCommandLists(_countof(list_list), list_list);
        ++m_fence_value;
        m_queue->Signal(m_fence.Get(), m_fence_value);
        m_list_unused.emplace(list);
        m_allocator_executing.emplace(m_allocator_map[list], m_fence_value);
        m_allocator_map.erase(list);
        return m_fence_value;
    }

    auto create_swap_chain(Microsoft::WRL::ComPtr<IDXGIFactory5> factory, HWND window
    , UINT width, UINT height, DXGI_FORMAT format, DXGI_USAGE usage, UINT count
    , bool tearing_supported) -> Microsoft::WRL::ComPtr<IDXGISwapChain4> {
        Microsoft::WRL::ComPtr<IDXGISwapChain4> out{};
        DXGI_SWAP_CHAIN_DESC1 swap_chain_description{};
        swap_chain_description.Width = width;
        swap_chain_description.Height = height;
        swap_chain_description.Format = format;
        swap_chain_description.Stereo = false;
        swap_chain_description.SampleDesc = { 1, 0 };
        swap_chain_description.BufferUsage = usage;
        swap_chain_description.BufferCount = count;
        swap_chain_description.Scaling = DXGI_SCALING_NONE;
        swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_description.Flags = tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1{};
        factory->CreateSwapChainForHwnd(m_queue.Get(), window
        , &swap_chain_description, nullptr, nullptr, &swap_chain1);
        swap_chain1.As(&out);
        factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);
        return out;
    }
};

#endif