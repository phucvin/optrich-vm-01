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
    CALL_INDIRECT, // New Opcode

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
    STRING_CONST,

    // Numeric i32
    I32_EQZ, I32_EQ, I32_NE, I32_LT_S, I32_LT_U, I32_GT_S, I32_GT_U, I32_LE_S, I32_LE_U, I32_GE_S, I32_GE_U,
    I32_CLZ, I32_CTZ, I32_POPCNT,
    I32_ADD, I32_SUB, I32_MUL, I32_DIV_S, I32_DIV_U, I32_REM_S, I32_REM_U, I32_AND, I32_OR, I32_XOR, I32_SHL, I32_SHR_S, I32_SHR_U, I32_ROTL, I32_ROTR,

    // F64
    F64_ADD, F64_SUB, F64_MUL, F64_DIV,
};

struct Instruction {
    Opcode opcode;
    std::variant<int32_t, int64_t, float, double, std::string> operand;

    Instruction() : opcode(Opcode::NOP), operand(0) {} // Default constructor
    Instruction(Opcode op);
    Instruction(Opcode op, int32_t val);
    Instruction(Opcode op, int64_t val);
    Instruction(Opcode op, float val);
    Instruction(Opcode op, double val);
    Instruction(Opcode op, std::string val);
};

struct Import {
    std::string module;
    std::string field;
    std::string alias;
    std::vector<std::string> paramTypes;
    std::vector<std::string> resultTypes;
};

struct Function {
    std::string name;
    std::vector<std::string> paramTypes;
    std::vector<std::string> paramNames;
    std::vector<std::string> resultTypes;
    std::vector<std::string> localTypes;
    std::vector<std::string> localNames;

    std::vector<Instruction> body;
};

struct StringDefinition {
    std::string name;
    std::string value;
};

struct Type {
    std::string name;
    std::vector<std::string> paramTypes;
    std::vector<std::string> resultTypes;
};

struct Table {
    std::string name; // Usually empty or "0" for MVP, but can be named
    uint32_t min;
    uint32_t max;
};

struct ElementSegment {
    uint32_t tableIndex;
    Instruction offset; // Expression to calculate offset (usually i32.const)
    std::vector<std::string> functionNames;
};

struct Module {
    std::vector<Import> imports;
    std::vector<Function> functions;
    std::vector<StringDefinition> strings;
    std::vector<Type> types;
    std::vector<Table> tables;
    std::vector<ElementSegment> elements;
};
