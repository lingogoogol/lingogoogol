#ifndef COMPILERCPP_INSTR_TABLE
#define COMPILERCPP_INSTR_TABLE

#include <vector>
#include <map>

#include "entry.hpp"
#include "mnemonic.hpp"

std::map<mnemonic::mnemonic_t, std::vector<instr_entry>> instr_table{
    {
        mnemonic::ADD,
        {
            {
                { { operand_entry::AL, operand_entry::discard }, { operand_entry::imm8, operand_entry::imm } },
                { { opcode::constant, 0x04 } }
            }
        }
    },
    {
        mnemonic::CALL,
        {
            {
                { { operand_entry::rel32, operand_entry::imm } },
                { { opcode::constant, 0xE8 } }
            }
        }
    },
    {
        mnemonic::XOR,
        {
            {
                { { operand_entry::r32, operand_entry::ModR_M_reg }, { operand_entry::r_m32, operand_entry::ModR_M_r_m } },
                { { opcode::operand_size_32 }, { opcode::constant, 0x33 } }
            }
        }
    },
    {
        mnemonic::RET,
        {
            {
                {},
                { { opcode::constant, 0xC3 } }
            }
        }
    }
};

#endif