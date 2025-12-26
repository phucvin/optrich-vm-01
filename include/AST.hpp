#pragma once
#include <vector>
#include <string>
#include <memory>
#include <variant>

enum class Opcode {
    UNREACHABLE,
    NOP,
    BLOCK,
    LOOP,
    IF,
    ELSE,
    END,
    BR,
    BR_IF,
    RETURN,
    CALL,

    // Variables
    LOCAL_GET,
    LOCAL_SET,
    LOCAL_TEE,
    GLOBAL_GET,
    GLOBAL_SET,

    // Constants
    I32_CONST,
    I64_CONST,
    F32_CONST,
    F64_CONST,

    // Numeric i32
    I32_EQZ, I32_EQ, I32_NE, I32_LT_S, I32_LT_U, I32_GT_S, I32_GT_U, I32_LE_S, I32_LE_U, I32_GE_S, I32_GE_U,
    I32_CLZ, I32_CTZ, I32_POPCNT,
    I32_ADD, I32_SUB, I32_MUL, I32_DIV_S, I32_DIV_U, I32_REM_S, I32_REM_U, I32_AND, I32_OR, I32_XOR, I32_SHL, I32_SHR_S, I32_SHR_U, I32_ROTL, I32_ROTR,

    // F64
    F64_ADD, F64_SUB, F64_MUL, F64_DIV,

    // ... can add others as needed
};

// Simplified IR Instruction
struct Instruction {
    Opcode opcode;
    // Operands
    std::variant<int32_t, int64_t, float, double, std::string> operand;
    // For block/loop/if, we might need a block type or jump target
    // For call, function index
    // For br, label index

    Instruction(Opcode op) : opcode(op), operand(0) {}
    Instruction(Opcode op, int32_t val) : opcode(op), operand(val) {}
    Instruction(Opcode op, int64_t val) : opcode(op), operand(val) {}
    Instruction(Opcode op, float val) : opcode(op), operand(val) {}
    Instruction(Opcode op, double val) : opcode(op), operand(val) {}
    Instruction(Opcode op, std::string val) : opcode(op), operand(val) {} // For symbolic resolution later
};

struct Function {
    std::string name; // Symbol
    std::vector<std::string> paramTypes; // "i32", "f64"
    std::vector<std::string> paramNames; // "$x", "" (empty if unnamed)
    std::vector<std::string> resultTypes;
    std::vector<std::string> localTypes;
    std::vector<std::string> localNames; // "$y"

    std::vector<Instruction> body;
};

struct Module {
    std::vector<Function> functions;
    // exports, imports, globals, etc.
};
