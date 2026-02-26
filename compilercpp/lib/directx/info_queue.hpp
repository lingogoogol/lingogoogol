#ifndef COMPILERCPP_LIB_DIRECTX_INFO_QUEUE
#define COMPILERCPP_LIB_DIRECTX_INFO_QUEUE

#include <string>
#include <cstddef>

#include "../header.hpp"

#include "../file.hpp"

auto log_info_queue(Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue) -> void {
    for (UINT64 i{ 0 }; i < info_queue->GetNumStoredMessagesAllowedByRetrievalFilter(); ++i) {
        SIZE_T message_len{};
        info_queue->GetMessage(i, nullptr, &message_len);
        D3D12_MESSAGE* info{ reinterpret_cast<D3D12_MESSAGE*>(new std::byte[message_len]{}) };
        info_queue->GetMessage(i, info, &message_len);
        log_file(std::string{ info->pDescription } + "\r\n");
    }
    return;
}

#endif