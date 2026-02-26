#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <cstring>

#define UNICODE

#include "lib/header/Windows.h"
#include "lib/header/wrl_client.h"
#include "lib/header/d3d12.h"
#include "lib/header/d3d12shader.h"
#include "lib/header/DirectXMath.h"
#include "lib/header/dxgi1_6.h"
#include "lib/header/shellapi.h"
#include "lib/header/d3d11on12.h"
#include "lib/header/d2d1_3.h"
#include "lib/header/dwrite_3.h"

Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue{};

#include "lib/.hpp"
#include "out/.hpp"
#include "global/.hpp"
#include "stmt/.hpp"
#include "expr/.hpp"
#include "instr/.hpp"

auto compile(std::istream& source, std::ostream& exe) -> void {
    std::string entrance{ get_string(source) };

    std::vector<fun> funs{};
    try {
        while (true) {
            global get{ source };
            switch (get.type()) {
            case global::fun:
                funs.push_back(get.to_fun());
                break;
            default:
                log_file("global type\r\n");
            }
        }
    }
    catch (error& e) {
        if (e.type() != error::eof) {
            throw e;
        }
    }

    std::uint32_t bss_size{ 0 };
    std::uint32_t entry_point_address{ 0 };
    std::vector<unsigned char> data{}, rdata{}, tls{}, text{ 0x33, 0b11000000, 0xC3 }, idata{}, reloc{};
    auto entrance_fun{ std::find_if(funs.begin(), funs.end(), [&entrance] (const fun& in) {
        return in.name() == entrance;
    }) };
    entrance_fun->compile(text);
    funs.erase(entrance_fun);
    for (std::int8_t i{ 0 }; i < funs.size(); ++i) {
        funs[i].compile(text);
    }

    dos_header(exe);
    signature(exe);
    coff_file_header_missing_field coff_file_header_missing_field_obj{ coff_file_header(exe) };
    const std::streamoff start_of_optional_header{ exe.tellp() };
    optional_header_missing_field optional_header_missing_field_obj{ optional_header(exe) };
    fill_integer(exe, coff_file_header_missing_field_obj.m_size_of_optional_header
    , static_cast<std::uint16_t>(static_cast<std::streamoff>(exe.tellp()) - start_of_optional_header));

    std::vector<section> sections{};
    size_entry size_obj{};
    push_section(sections, ".text", section_flag::CNT_CODE | section_flag::MEM_EXECUTE | section_flag::MEM_READ
    , text, static_cast<std::uint32_t>(text.size()), size_obj);
    push_section(sections, ".bss", section_flag::CNT_UNINITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , {}, bss_size, size_obj);
    push_section(sections, ".data", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , data, static_cast<std::uint32_t>(data.size()), size_obj);
    push_section(sections, ".rdata", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ
    , rdata, static_cast<std::uint32_t>(rdata.size()), size_obj);
    push_section(sections, ".tls", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , tls, static_cast<std::uint32_t>(tls.size()), size_obj);
    push_section(sections, ".idata", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_WRITE
    , idata, static_cast<std::uint32_t>(idata.size()), size_obj);
    push_section(sections, ".reloc", section_flag::CNT_INITIALIZED_DATA | section_flag::MEM_READ | section_flag::MEM_DISCARDABLE
    , reloc, static_cast<std::uint32_t>(reloc.size()), size_obj);

    std::vector<section_header_missing_field> section_header_missing_fields{};
    for (std::uint8_t i{ 0 }; i < sections.size(); ++i) {
        section_header_missing_fields.push_back(pad_section_header(exe, sections[i]));
    }
    std::uint32_t virtual_address{ static_cast<std::uint32_t>(exe.tellp()) };
    align_file(exe, virtual_address);
    fill_integer(exe, coff_file_header_missing_field_obj.m_number_of_sections, static_cast<std::uint16_t>(sections.size()));
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_code, size_obj.m_code);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_initialized_data, size_obj.m_initialized_data);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_uninitialized_data, size_obj.m_uninitialized_data);
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_headers, static_cast<std::uint32_t>(exe.tellp()));
    fill_integer(exe, optional_header_missing_field_obj.m_address_of_entry_point, virtual_address + entry_point_address);
    fill_integer(exe, optional_header_missing_field_obj.m_base_of_code, virtual_address);

    for (std::uint8_t i{ 0 }; i < sections.size(); ++i) {
        pad_section(exe, sections[i], section_header_missing_fields[i], virtual_address);
    }
    fill_integer(exe, optional_header_missing_field_obj.m_size_of_image, virtual_address);
    return;
}

constexpr inline LONG window_width{ 0x400 };
constexpr inline LONG window_height{ 0x400 };
constexpr inline UINT buffer_count{ 0x2 };
const inline std::wstring window_name{ L"compiler" };

struct window_state {
    bool m_initialized{ false };
    bool m_tearing_supported{};
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue{};
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain{};
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_RT{};
    command_queue_smart m_command_queue{};
    std::vector<UINT64> m_buffer_fence_value{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTV_heap{};
    UINT m_RTV_size{};
    UINT m_frame_index{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature{};
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state{};
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view{};
    text m_text{};
};

auto CALLBACK WndProc(HWND window, UINT message, WPARAM param_1, LPARAM param_2) -> LRESULT {
    window_state* state{ reinterpret_cast<window_state*>(GetWindowLongPtrW(window, GWLP_USERDATA)) };
    if (!state || !state->m_initialized) {
        return DefWindowProcW(window, message, param_1, param_2);
    }
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint_info{};
        BeginPaint(window, &paint_info);
        log_console("paint received\r\n");
        Microsoft::WRL::ComPtr<ID3D12Resource> RT_current{ state->m_RT[state->m_frame_index] };
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list{ state->m_command_queue.create_list() };
        push_transition_barrier(command_list, RT_current, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE RTV_handle{ create_V_handle(state->m_RTV_heap->GetCPUDescriptorHandleForHeapStart()
        , state->m_RTV_size, state->m_frame_index) };
        clear_RT(command_list, RTV_handle, { 0.0f, 0.0f, 0.0f, 1.0f });
        command_list->SetGraphicsRootSignature(state->m_root_signature.Get());
        command_list->SetPipelineState(state->m_pipeline_state.Get());
        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = window_width;
        viewport.Height = window_height;
        viewport.MinDepth = D3D12_MIN_DEPTH;
        viewport.MaxDepth = D3D12_MAX_DEPTH;
        command_list->RSSetViewports(1, &viewport);
        D3D12_RECT scissor_rect{};
        scissor_rect.left = 0;
        scissor_rect.top = 0;
        scissor_rect.right = window_width;
        scissor_rect.bottom = window_height;
        command_list->RSSetScissorRects(1, &scissor_rect);
        command_list->OMSetRenderTargets(1, &RTV_handle, false, nullptr);
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        command_list->IASetVertexBuffers(0, 1, &state->m_vertex_buffer_view);
        command_list->DrawInstanced(3, 1, 0, 0);
        state->m_command_queue.execute_list(command_list);
        state->m_text.render(state->m_frame_index);

        state->m_buffer_fence_value[state->m_frame_index] = state->m_command_queue.set_fence();
        state->m_swap_chain->Present(0, state->m_tearing_supported ? DXGI_PRESENT_ALLOW_TEARING : 0);
        state->m_frame_index = state->m_swap_chain->GetCurrentBackBufferIndex();
        state->m_command_queue.wait_fence(state->m_buffer_fence_value[state->m_frame_index]);
        EndPaint(window, &paint_info);
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    default: {
        return DefWindowProcW(window, message, param_1, param_2);
    }
    }
}

struct vertex_data {
    DirectX::XMFLOAT4 pos{};
    DirectX::XMFLOAT4 color{};
};

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arg, int) -> int {
    int argc{};
    LPWSTR* argv{ CommandLineToArgvW(arg, &argc) };
    if (argc != 3) {
        log_file("command_line_argument_count");
    }

    init_logfile(argv[2]);
    init_directx();

    Microsoft::WRL::ComPtr<IDXGIFactory5> factory{ create_factory(true) };
    window_state state{};
    state.m_tearing_supported = check_tearing_support(factory);
    ATOM window_class{ create_window_class(L"window_class", &WndProc, instance
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED))
    , reinterpret_cast<HICON>(LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED))
    , reinterpret_cast<HCURSOR>(LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_SHARED))) };
    HWND window{ create_window(factory, window_class, L"compilercpp", WS_POPUP, window_width, window_height, instance) };
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{ create_adapter(factory) };
    Microsoft::WRL::ComPtr<ID3D12Device2> device{ create_device(adapter) };
    state.m_info_queue = create_info_queue(device);
    info_queue = state.m_info_queue;
    state.m_command_queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    state.m_swap_chain = state.m_command_queue.create_swap_chain(factory, window, window_width
    , window_height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_USAGE_RENDER_TARGET_OUTPUT, buffer_count, state.m_tearing_supported);
    state.m_RTV_heap = create_V_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count);
    state.m_RTV_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    state.m_RT = create_RT(state.m_swap_chain, buffer_count);
    create_RTV(device, state.m_RT, state.m_RTV_size, state.m_RTV_heap);
    text::init(device, state.m_command_queue.get(), window, state.m_RT);
    state.m_text = text{ L"compilercpp", 0x0, 0x0, 0x200, 0x200, 1.0f, 1.0f, 1.0f };
    state.m_frame_index = state.m_swap_chain->GetCurrentBackBufferIndex();
    state.m_buffer_fence_value.resize(buffer_count);
    state.m_initialized = true;
    D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_version{};
    root_signature_version.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &root_signature_version, sizeof root_signature_version);
    if (root_signature_version.HighestVersion != D3D_ROOT_SIGNATURE_VERSION_1_1) {
        log_file("root_signature_version\r\n");
        return -1;
    }
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description{};
    root_signature_description.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_signature_description.Desc_1_1.NumParameters = 0;
    root_signature_description.Desc_1_1.pParameters = nullptr;
    root_signature_description.Desc_1_1.NumStaticSamplers = 0;
    root_signature_description.Desc_1_1.pStaticSamplers = nullptr;
    root_signature_description.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    Microsoft::WRL::ComPtr<ID3DBlob> root_signature_blob{};
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob{};
    D3D12SerializeVersionedRootSignature(&root_signature_description, &root_signature_blob, &error_blob);
    device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&state.m_root_signature));
    std::array<std::string, 2> shader_code{};
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_element{};
    std::array<std::string, 2> input_element_name{ "POS", "COLOR" };
    input_element.push_back(create_input_element(input_element_name[0], DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0));
    input_element.push_back(create_input_element(input_element_name[1], DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_description{};
    pipeline_state_description.pRootSignature = state.m_root_signature.Get();
    pipeline_state_description.VS = create_shader_bytecode("vertex.cso", shader_code[0]);
    pipeline_state_description.PS = create_shader_bytecode("pixel.cso", shader_code[1]);
    pipeline_state_description.BlendState.AlphaToCoverageEnable = false;
    pipeline_state_description.BlendState.IndependentBlendEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].BlendEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].LogicOpEnable = false;
    pipeline_state_description.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pipeline_state_description.SampleMask = -1;
    pipeline_state_description.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipeline_state_description.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pipeline_state_description.RasterizerState.FrontCounterClockwise = false;
    pipeline_state_description.RasterizerState.DepthClipEnable = true;
    pipeline_state_description.RasterizerState.DepthClipEnable = false;
    pipeline_state_description.RasterizerState.AntialiasedLineEnable = false;
    pipeline_state_description.RasterizerState.ForcedSampleCount = 0;
    pipeline_state_description.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    pipeline_state_description.DepthStencilState.DepthEnable = false;
    pipeline_state_description.DepthStencilState.StencilEnable = false;
    pipeline_state_description.InputLayout.pInputElementDescs = input_element.data();
    pipeline_state_description.InputLayout.NumElements = input_element.size();
    pipeline_state_description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_description.NumRenderTargets = 1;
    pipeline_state_description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipeline_state_description.SampleDesc.Count = 1;
    pipeline_state_description.SampleDesc.Quality = 0;
    pipeline_state_description.NodeMask = 0;
    pipeline_state_description.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    device->CreateGraphicsPipelineState(&pipeline_state_description, IID_PPV_ARGS(&state.m_pipeline_state));
    std::array vertex{
        vertex_data{ { 0.0f, 0.25f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        vertex_data{ { 0.25f, -0.25f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        vertex_data{ { -0.25f, -0.25f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
    };
    const UINT64 vertex_size{ vertex.size() * sizeof(vertex_data) };
    D3D12_HEAP_PROPERTIES upload_heap_property{};
    upload_heap_property.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap_property.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_heap_property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_heap_property.CreationNodeMask = 0;
    upload_heap_property.VisibleNodeMask = 0;
    D3D12_RESOURCE_DESC upload_resource_description{};
    upload_resource_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_resource_description.Alignment = 0;
    upload_resource_description.Width = vertex_size;
    upload_resource_description.Height = 1;
    upload_resource_description.DepthOrArraySize = 1;
    upload_resource_description.MipLevels = 1;
    upload_resource_description.Format = DXGI_FORMAT_UNKNOWN;
    upload_resource_description.SampleDesc.Count = 1;
    upload_resource_description.SampleDesc.Quality = 0;
    upload_resource_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    upload_resource_description.Flags = D3D12_RESOURCE_FLAG_NONE;
    Microsoft::WRL::ComPtr<ID3D12Resource> upload_resource{};
    device->CreateCommittedResource(&upload_heap_property, D3D12_HEAP_FLAG_NONE, &upload_resource_description
    , D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload_resource));
    void* upload_resource_cpu_address{};
    upload_resource->Map(0, nullptr, &upload_resource_cpu_address);
    std::memcpy(upload_resource_cpu_address, vertex.data(), vertex_size);
    upload_resource->Unmap(0, nullptr);
    state.m_vertex_buffer_view.BufferLocation = upload_resource->GetGPUVirtualAddress();
    state.m_vertex_buffer_view.SizeInBytes = vertex_size;
    state.m_vertex_buffer_view.StrideInBytes = sizeof(vertex_data);
    ShowWindow(window, SW_SHOW);

    MSG message{};
    BOOL message_get_result{};
    while ((message_get_result = GetMessageW(&message, NULL, 0, 0))) {
        if (message_get_result == -1) {
            log_file("message_get\r\n");
            return -1;
        }
        DispatchMessageW(&message);
    }
    state.m_command_queue.flush();

    std::ifstream source{ create_ifstream(argv[0], std::ios_base::in | std::ios_base::binary) };
    std::ofstream exe{ create_ofstream(argv[1], std::ios_base::out
    | std::ios_base::binary | std::ios_base::trunc) };
    compile(source, exe);

    source.close();
    exe.close();
    log_info_queue(info_queue);
    log_file("complete\r\n");
    return 0;
}