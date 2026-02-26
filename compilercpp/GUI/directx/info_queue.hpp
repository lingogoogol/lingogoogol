#ifndef COMPILERCPP_GUI_DIRECTX_INFO_QUEUE
#define COMPILERCPP_GUI_DIRECTX_INFO_QUEUE

#include <string>
#include <cstddef>

#include "../../lib/header.hpp"

#include "../../lib/file.hpp"

auto log_info_queue(Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue) -> void {
    for (UINT64 i{ 0 }; i < info_queue->GetNumStoredMessagesAllowedByRetrievalFilter(); ++i) {
        SIZE_T message_len{};
        info_queue->GetMessage(i, nullptr, &message_len);
        D3D12_MESSAGE* info{ reinterpret_cast<D3D12_MESSAGE*>(new std::byte[message_len]{}) };
        info_queue->GetMessage(i, info, &message_len);
        std::string error_message{ info->pDescription };
        if (error_message != "ID3D12Device::CreateCommittedResource: "
        "Ignoring InitialState D3D12_RESOURCE_STATE_GENERIC_READ. Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON.") {
            log_file(error_message + "\n");
        }
    }
    return;
}

#endif