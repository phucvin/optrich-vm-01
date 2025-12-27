#pragma once

#include "AST.hpp"
#include "MemoryStore.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <functional>
#include <iostream>

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
};

class Interpreter {
public:
    Interpreter(Module& mod, MemoryStore& store);

    void registerHostFunction(std::string name, HostFunction func, int arity);

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
