#ifndef COMPILERCPP_GUI_DIRECTX_SHADER_BYTECODE
#define COMPILERCPP_GUI_DIRECTX_SHADER_BYTECODE

#include <string>

#include "../../lib/io.hpp"

class shader_bytecode_t {
public:
    shader_bytecode_t(std::uint16_t type, std::uint16_t name);

    auto desc_get() const -> D3D12_SHADER_BYTECODE;
private:
    std::string m_bytecode{};
};

shader_bytecode_t::shader_bytecode_t(std::uint16_t type, std::uint16_t name): m_bytecode{ get_resource(type, name) } {}

auto shader_bytecode_t::desc_get() const -> D3D12_SHADER_BYTECODE {
    D3D12_SHADER_BYTECODE out{};
    out.pShaderBytecode = m_bytecode.data();
    out.BytecodeLength = m_bytecode.size();
    return out;
}

#endif