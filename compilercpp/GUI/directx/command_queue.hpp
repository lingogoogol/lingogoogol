#ifndef COMPILERCPP_GUI_DIRECTX_COMMAND_QUEUE
#define COMPILERCPP_GUI_DIRECTX_COMMAND_QUEUE

#include <queue>
#include <map>
#include <utility>
#include <mutex>

#include "info_queue.hpp"

#include "../directx/init.hpp"
#include "../../lib/header.hpp"
#include "../../lib/file.hpp"

class command_queue_t {
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
    std::recursive_mutex m_mutex{};
public:
    command_queue_t() = default;

    auto init(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) -> void {
        std::unique_lock lock{ m_mutex };
        m_device = device;
        m_type = type;
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;
        hresult(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue)));
        hresult(m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_event = CreateEventW(nullptr, false, false, nullptr);
        return;
    }

    auto get() -> Microsoft::WRL::ComPtr<ID3D12CommandQueue> {
        std::unique_lock lock{ m_mutex };
        return m_queue;
    }

    auto set_fence() -> UINT64 {
        std::unique_lock lock{ m_mutex };
        ++m_fence_value;
        hresult(m_queue->Signal(m_fence.Get(), m_fence_value));
        return m_fence_value;
    }

    auto check_fence(UINT64 fence_value) -> bool {
        std::unique_lock lock{ m_mutex };
        return m_fence->GetCompletedValue() >= fence_value;
    }

    auto wait_fence(UINT64 fence_value) -> void {
        std::unique_lock lock{ m_mutex };
        hresult(m_fence->SetEventOnCompletion(fence_value, m_event));
        WaitForSingleObject(m_event, INFINITE);
        return;
    }

    auto flush() -> void {
        wait_fence(set_fence());
        return;
    }

    auto create_list() -> Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> {
        std::unique_lock lock{ m_mutex };
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator{};
        if (!m_allocator_executing.empty() && check_fence(m_allocator_executing.front().second)) {
            allocator = m_allocator_executing.front().first;
            m_allocator_executing.pop();
            hresult(allocator->Reset());
        }
        else {
            hresult(m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&allocator)));
        }
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list{};
        if (m_list_unused.size()) {
            list = m_list_unused.front();
            m_list_unused.pop();
            hresult(list->Reset(allocator.Get(), nullptr));
        }
        else {
            hresult(m_device->CreateCommandList(0, m_type, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        }
        m_allocator_map.emplace(list, allocator);
        return list;
    }

    auto execute_list(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list) -> UINT64 {
        std::unique_lock lock{ m_mutex };
        hresult(list->Close());
        ID3D12CommandList* list_list{ list.Get() };
        m_queue->ExecuteCommandLists(1, &list_list);
        ++m_fence_value;
        hresult(m_queue->Signal(m_fence.Get(), m_fence_value));
        m_list_unused.emplace(list);
        m_allocator_executing.emplace(m_allocator_map[list], m_fence_value);
        m_allocator_map.erase(list);
        return m_fence_value;
    }

    auto create_swap_chain(Microsoft::WRL::ComPtr<IDXGIFactory5> factory, HWND window
    , UINT width, UINT height, DXGI_FORMAT format, DXGI_USAGE usage, UINT count
    , bool tearing_supported) -> Microsoft::WRL::ComPtr<IDXGISwapChain4> {
        std::unique_lock lock{ m_mutex };
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
        //Specify blending in D3D12_RENDER_TARGET_BLEND_DESC. Alpha blended swapchains must not be created with CreateSwapChainForHwnd.
        swap_chain_description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_description.Flags = tearing_supported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1{};
        hresult(factory->CreateSwapChainForHwnd(m_queue.Get(), window
        , &swap_chain_description, nullptr, nullptr, &swap_chain1));
        hresult(swap_chain1.As(&out));
        hresult(factory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES));
        return out;
    }
};

#endif