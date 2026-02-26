#ifndef COMPILERCPP_INSTR_ENTRY
#define COMPILERCPP_INSTR_ENTRY

#include <vector>

#include "opcode.hpp"
#include "operand_entry.hpp"

struct instr_entry {
    std::vector<operand_entry> m_operand{};
    std::vector<opcode> m_opcode{};
};

#endif