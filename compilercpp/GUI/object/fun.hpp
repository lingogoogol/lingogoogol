#ifndef COMPILERCPP_GUI_OBJECT_FUN
#define COMPILERCPP_GUI_OBJECT_FUN

#include <cstdint>

#include "state.hpp"

auto restore_state(state_t* state, std::uint64_t previous_state) -> void {
    state->restore_state(previous_state);
    state->remove_state(previous_state);
    return;
}

auto inform_restore(state_t* state, std::wstring info) -> void {
    std::uint64_t previous_state{ state->save_state() };
    state->clear_state();
    state->add_object<text_block_t>(pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, info
    , pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, std_white, alignment_2D{ alignment_x::left, alignment_y::center });
    state->add_object<button_classic_std_t>(pos_2D{ 0x1C0, 0x340 }, std::bind(restore_state, state, previous_state), L"確定", depth_range_t{ 0.0f, 1.0f });
    return;
}

auto store_input(state_t* state, std::wstring* out, std::uint64_t input_id, std::function<void(void)>& callback) -> void {
    *out = state->get_object_weak<text_block_scroll_input_t>(input_id).lock()->get_text();
    callback();
    return;
}

auto get_input(state_t* state, std::wstring prompt, std::wstring* out, std::function<void(void)> callback) -> void {
    state->clear_state();
    state->add_object<text_block_t>(pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, prompt
    , pos_2D{ 0x80, 0x80 }, size_2D{ 0x300, 0x40 }, size_1D{ 0x10 }, std_white, alignment_2D{ alignment_x::left, alignment_y::center });
    std::uint64_t input_id{ state->add_object<text_block_scroll_input_t>(pos_2D{ 0x80, 0xC0 }, 0.0f, L"", nullptr, size_2D{ 0x300, 0x270 }
    , size_1D{ 0x20 }, std_white, alignment_2D{ alignment_x::left, alignment_y::top }) };
    state->add_object<button_classic_std_t>(pos_2D{ 0x1C0, 0x340 }, std::bind(store_input, state, out, input_id, callback), L"確定", depth_range_t{ 0.0f, 1.0f });
    return;
}

auto get_input_restore(state_t* state, std::wstring prompt, std::wstring* out) -> void {
    std::uint64_t previous_state{ state->save_state() };
    state->clear_state();
    get_input(state, prompt, out, std::bind(restore_state, state, previous_state));
    return;
}

#endif