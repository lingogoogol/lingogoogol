#ifndef COMPILERCPP_GUI_DIRECTX_INFO_QUEUE
#define COMPILERCPP_GUI_DIRECTX_INFO_QUEUE

#include <string>
#include <cstddef>

#include "../../lib/header.hpp"

#include "../../lib/file.hpp"

auto log_info_queue(Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue) -> void {
    if (!info_queue) {
        return;
    }
    if (!info_queue->GetNumStoredMessagesAllowedByRetrievalFilter()) {
        return;
    }
    for (UINT64 i{ 0 }; i < info_queue->GetNumStoredMessagesAllowedByRetrievalFilter(); ++i) {
        SIZE_T message_len{};
        info_queue->GetMessage(i, nullptr, &message_len);
        D3D12_MESSAGE* info{ reinterpret_cast<D3D12_MESSAGE*>(new std::byte[message_len]{}) };
        info_queue->GetMessage(i, info, &message_len);
        std::string error_message{ info->pDescription };
        log_file(error_message + "\n");
    }
    info_queue->ClearStoredMessages();
    return;
}

#endif
