#pragma once

#include "AST.h"
#include "MemoryStore.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <functional>
#include <iostream>

// Basic Wasm Values
// Note: In a real engine we might use a union or std::variant.
// For simplicity and standard compliance, we'll use a tagged union approach.
struct WasmValue {
    enum Type { I32, I64, F32, F64, VOID } type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };

    WasmValue() : type(VOID), i64(0) {}
    explicit WasmValue(int32_t v) : type(I32), i32(v) {}
    explicit WasmValue(int64_t v) : type(I64), i64(v) {}
    explicit WasmValue(float v) : type(F32), f32(v) {}
    explicit WasmValue(double v) : type(F64), f64(v) {}
};

struct StackFrame {
    Function* func;
    size_t pc; // program counter
    std::vector<WasmValue> locals;
    int returnHeight; // Stack height to return to
};

using HostFunction = std::function<WasmValue(std::vector<WasmValue>& args)>;

struct HostFuncEntry {
    HostFunction func;
    int arity;
    std::vector<std::string> paramTypes;
    std::vector<std::string> resultTypes;
};

class Interpreter {
public:
    Interpreter(Module& mod, MemoryStore& store);

    // New API for full module imports
    void registerHostFunction(std::string modName, std::string fieldName, HostFunction func,
                              const std::vector<std::string>& params,
                              const std::vector<std::string>& results);

    WasmValue run(std::string funcName, std::vector<WasmValue> args);

private:
    Module& module;
    MemoryStore& store;
    std::vector<StackFrame> callStack;
    std::vector<WasmValue> valueStack;
    std::unordered_map<std::string, size_t> funcMap;
    std::unordered_map<std::string, HostFuncEntry> hostFuncs;

    void push(WasmValue v);
    WasmValue pop();

    void handleReturn();

    void execute(Instruction& instr, StackFrame& frame);

    int resolveLocal(const std::string& id, Function* func);
};
