#include "AST.h"

Instruction::Instruction(Opcode op) : opcode(op), operand(0) {}
Instruction::Instruction(Opcode op, int32_t val) : opcode(op), operand(val) {}
Instruction::Instruction(Opcode op, int64_t val) : opcode(op), operand(val) {}
Instruction::Instruction(Opcode op, float val) : opcode(op), operand(val) {}
Instruction::Instruction(Opcode op, double val) : opcode(op), operand(val) {}
Instruction::Instruction(Opcode op, std::string val) : opcode(op), operand(val) {}
