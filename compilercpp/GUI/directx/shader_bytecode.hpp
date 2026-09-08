#ifndef COMPILERCPP_GUI_DIRECTX_SHADER_BYTECODE
#define COMPILERCPP_GUI_DIRECTX_SHADER_BYTECODE

#include <string>
#include <fstream>
#include <stdexcept>

class shader_bytecode_t {
public:
    shader_bytecode_t(std::uint16_t type, std::uint16_t name, std::string bytecode);

    auto desc_get() const -> D3D12_SHADER_BYTECODE;
private:
    std::string m_bytecode{};
};

shader_bytecode_t::shader_bytecode_t(std::uint16_t, std::uint16_t name, std::string bytecode): m_bytecode{ std::move(bytecode) } {
	if (!m_bytecode.empty()) {
		return;
	}
	const char* filename{};
	switch (name) {
	case 1: filename = "shader/solid_vertex.cso"; break;
	case 2: filename = "shader/solid_pixel.cso"; break;
	case 3: filename = "shader/texture_vertex.cso"; break;
	case 4: filename = "shader/texture_pixel.cso"; break;
	default: throw std::runtime_error{ "unknown shader resource" };
	}
	std::ifstream stream{ filename, std::ios::binary };
	if (!stream) {
		throw std::runtime_error{ std::string{ "failed to open " } + filename };
	}
	m_bytecode.assign(std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{});
	if (m_bytecode.empty()) {
		throw std::runtime_error{ std::string{ "empty shader bytecode in " } + filename };
	}
}

auto shader_bytecode_t::desc_get() const -> D3D12_SHADER_BYTECODE {
    D3D12_SHADER_BYTECODE out{};
    out.pShaderBytecode = m_bytecode.data();
    out.BytecodeLength = m_bytecode.size();
    return out;
}

#endif
