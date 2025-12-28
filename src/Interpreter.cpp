#include "Interpreter.h"

Interpreter::Interpreter(Module& mod, MemoryStore& store) : module(mod), store(store) {
    // Build Symbol Tables
    for (size_t i = 0; i < module.functions.size(); ++i) {
        funcMap[module.functions[i].name] = i;
    }

    // Initialize Strings
    for (const auto& strDef : module.strings) {
        std::vector<uint8_t> data;
        int32_t len = strDef.value.length();

        // 4 bytes length
        data.push_back(len & 0xFF);
        data.push_back((len >> 8) & 0xFF);
        data.push_back((len >> 16) & 0xFF);
        data.push_back((len >> 24) & 0xFF);

        // String bytes
        for (char c : strDef.value) {
            data.push_back((uint8_t)c);
        }

        // Allocate readonly
        int32_t handle = store.alloc_readonly(data);
        stringHandles[strDef.name] = handle;
    }

    // Initialize Table
    if (!module.tables.empty()) {
        table.resize(module.tables[0].min, nullptr); // Support only one table for now
    }

    for (const auto& elem : module.elements) {
        int offset = elem.offset;
        for (size_t i = 0; i < elem.funcNames.size(); ++i) {
            std::string funcName = elem.funcNames[i];
            if (funcMap.find(funcName) != funcMap.end()) {
                size_t funcIndex = funcMap[funcName];
                if (offset + i < table.size()) {
                    table[offset + i] = &module.functions[funcIndex];
                }
            }
        }
    }
}

void Interpreter::registerHostFunction(std::string modName, std::string fieldName, HostFunction func,
                                       const std::vector<std::string>& params,
                                       const std::vector<std::string>& results) {
    // Scan module imports to see if this host function is needed
    int importIndex = 0;
    for (const auto& imp : module.imports) {
        if (imp.module == modName && imp.field == fieldName) {
            // Verify signature
            if (imp.paramTypes != params) {
                throw std::runtime_error("Import signature mismatch (params) for " + modName + "." + fieldName);
            }
            if (imp.resultTypes != results) {
                throw std::runtime_error("Import signature mismatch (results) for " + modName + "." + fieldName);
            }

            HostFuncEntry entry;
            entry.func = func;
            entry.arity = (int)params.size();
            entry.paramTypes = params;
            entry.resultTypes = results;

            // Link by alias if present
            if (!imp.alias.empty()) {
                hostFuncs[imp.alias] = entry;
            }
            hostFuncs[std::to_string(importIndex)] = entry;
        }
        importIndex++;
    }
}

WasmValue Interpreter::run(std::string funcName, std::vector<WasmValue> args) {
    if (funcMap.find(funcName) == funcMap.end()) {
        throw std::runtime_error("Function not found: " + funcName);
    }

    size_t funcIndex = funcMap[funcName];
    Function* startFunc = &module.functions[funcIndex];

    StackFrame frame;
    frame.func = startFunc;
    frame.pc = 0;
    frame.returnHeight = 0;

    if (args.size() != startFunc->paramTypes.size()) {
            throw std::runtime_error("Argument mismatch");
    }
    for (const auto& arg : args) {
        frame.locals.push_back(arg);
    }
    for (const auto& lType : startFunc->localTypes) {
            (void)lType;
            frame.locals.push_back(WasmValue((int32_t)0)); // Default init
    }

    callStack.push_back(frame);

    while (!callStack.empty()) {
        StackFrame& current = callStack.back();
        if (current.pc >= current.func->body.size()) {
            handleReturn();
            continue;
        }

        Instruction& instr = current.func->body[current.pc];
        current.pc++;

        execute(instr, current);
    }

    if (!valueStack.empty()) {
            return valueStack.back();
    }
    return WasmValue();
}

void Interpreter::push(WasmValue v) { valueStack.push_back(v); }

WasmValue Interpreter::pop() {
    if (valueStack.empty()) throw std::runtime_error("Stack underflow");
    WasmValue v = valueStack.back();
    valueStack.pop_back();
    return v;
}

void Interpreter::handleReturn() {
    bool hasResult = !callStack.back().func->resultTypes.empty();
    WasmValue res;
    if (hasResult) res = pop();

    size_t height = static_cast<size_t>(callStack.back().returnHeight);
    while (valueStack.size() > height) {
        valueStack.pop_back();
    }
    if (hasResult) push(res);

    callStack.pop_back();
}

void Interpreter::execute(Instruction& instr, StackFrame& frame) {
    switch (instr.opcode) {
        case Opcode::I32_CONST:
            push(WasmValue(std::get<int32_t>(instr.operand)));
            break;
        case Opcode::F64_CONST:
            push(WasmValue(std::get<double>(instr.operand)));
            break;
        case Opcode::STRING_CONST: {
            std::string alias = std::get<std::string>(instr.operand);
            if (stringHandles.find(alias) == stringHandles.end()) {
                throw std::runtime_error("Unknown string constant: " + alias);
            }
            push(WasmValue(stringHandles[alias]));
            break;
        }
        case Opcode::I32_ADD: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a + b));
            break;
        }
            case Opcode::I32_SUB: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a - b));
            break;
        }
        case Opcode::F64_ADD: {
            double b = pop().f64;
            double a = pop().f64;
            push(WasmValue(a + b));
            break;
        }
        case Opcode::F64_SUB: {
            double b = pop().f64;
            double a = pop().f64;
            push(WasmValue(a - b));
            break;
        }
        case Opcode::F64_MUL: {
            double b = pop().f64;
            double a = pop().f64;
            push(WasmValue(a * b));
            break;
        }
        case Opcode::F64_DIV: {
            double b = pop().f64;
            double a = pop().f64;
            push(WasmValue(a / b));
            break;
        }
        case Opcode::LOCAL_GET: {
            std::string id = std::get<std::string>(instr.operand);
            int idx = resolveLocal(id, frame.func);
            push(frame.locals[idx]);
            break;
        }
        case Opcode::LOCAL_SET: {
            std::string id = std::get<std::string>(instr.operand);
            int idx = resolveLocal(id, frame.func);
            frame.locals[idx] = pop();
            break;
        }
        case Opcode::CALL: {
            std::string funcName = std::get<std::string>(instr.operand);

            if (hostFuncs.count(funcName)) {
                auto& entry = hostFuncs[funcName];
                int arity = entry.arity;

                std::vector<WasmValue> args;
                for(int i=0; i<arity; ++i) args.push_back(WasmValue());
                for(int i=arity-1; i>=0; --i) args[i] = pop();

                WasmValue res = entry.func(args);
                if (res.type != WasmValue::VOID) push(res);

            } else if (funcMap.count(funcName)) {
                size_t idx = funcMap[funcName];
                Function* callee = &module.functions[idx];

                StackFrame newFrame;
                newFrame.func = callee;
                newFrame.pc = 0;
                newFrame.returnHeight = valueStack.size() - callee->paramTypes.size();

                size_t nargs = callee->paramTypes.size();
                std::vector<WasmValue> args(nargs);
                for(int i = nargs-1; i>=0; --i) {
                    args[i] = pop();
                }
                for(auto& a : args) newFrame.locals.push_back(a);

                for(const auto& lt : callee->localTypes) {
                    (void)lt;
                    newFrame.locals.push_back(WasmValue((int32_t)0));
                }

                callStack.push_back(newFrame);
            } else {
                throw std::runtime_error("Unknown function: " + funcName);
            }
            break;
        }
        case Opcode::CALL_INDIRECT: {
            // Operand is the type alias string
            std::string typeAlias = std::get<std::string>(instr.operand);
            Type* expectedType = resolveType(typeAlias);
            if (!expectedType) {
                throw std::runtime_error("Unknown type: " + typeAlias);
            }

            // Pop index
            int32_t idx = pop().i32;

            // Check bounds
            if (idx < 0 || idx >= (int32_t)table.size()) {
                throw std::runtime_error("undefined element: " + std::to_string(idx));
            }

            Function* callee = table[idx];
            if (!callee) {
                throw std::runtime_error("uninitialized element " + std::to_string(idx));
            }

            // Check signature
            if (callee->paramTypes != expectedType->paramTypes || callee->resultTypes != expectedType->resultTypes) {
                throw std::runtime_error("indirect call signature mismatch");
            }

            // Execute call (same as CALL)
            StackFrame newFrame;
            newFrame.func = callee;
            newFrame.pc = 0;
            newFrame.returnHeight = valueStack.size() - callee->paramTypes.size();

            size_t nargs = callee->paramTypes.size();
            std::vector<WasmValue> args(nargs);
            for(int i = nargs-1; i>=0; --i) {
                args[i] = pop();
            }
            for(auto& a : args) newFrame.locals.push_back(a);

            for(const auto& lt : callee->localTypes) {
                (void)lt;
                newFrame.locals.push_back(WasmValue((int32_t)0));
            }

            callStack.push_back(newFrame);
            break;
        }
        case Opcode::I32_EQ: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a == b ? 1 : 0));
            break;
        }
        case Opcode::I32_NE: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a != b ? 1 : 0));
            break;
        }
        case Opcode::I32_LT_S: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a < b ? 1 : 0));
            break;
        }
        case Opcode::I32_GT_S: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a > b ? 1 : 0));
            break;
        }
        case Opcode::I32_LE_S: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a <= b ? 1 : 0));
            break;
        }
        case Opcode::I32_GE_S: {
            int32_t b = pop().i32;
            int32_t a = pop().i32;
            push(WasmValue(a >= b ? 1 : 0));
            break;
        }
        case Opcode::BLOCK:
        case Opcode::LOOP:
        case Opcode::END:
            break;
        case Opcode::BR:
        case Opcode::BR_IF: {
            bool shouldJump = true;
            if (instr.opcode == Opcode::BR_IF) {
                int32_t cond = pop().i32;
                if (cond == 0) shouldJump = false;
            }

            if (shouldJump) {
                std::string label = std::get<std::string>(instr.operand);
                int scanPC = frame.pc - 1;
                bool found = false;
                while (scanPC >= 0) {
                    Instruction& scanInstr = frame.func->body[scanPC];
                    if (scanInstr.opcode == Opcode::LOOP &&
                        std::holds_alternative<std::string>(scanInstr.operand) &&
                        std::get<std::string>(scanInstr.operand) == label) {
                        frame.pc = scanPC;
                        found = true;
                        break;
                    }
                    scanPC--;
                }

                if (!found) {
                    scanPC = frame.pc - 1;
                    int blockPC = -1;
                    while (scanPC >= 0) {
                         Instruction& scanInstr = frame.func->body[scanPC];
                         if (scanInstr.opcode == Opcode::BLOCK &&
                            std::holds_alternative<std::string>(scanInstr.operand) &&
                            std::get<std::string>(scanInstr.operand) == label) {
                             blockPC = scanPC;
                             break;
                         }
                         scanPC--;
                    }

                    if (blockPC != -1) {
                        int depth = 0;
                        int forwardPC = blockPC;
                        while (forwardPC < (int)frame.func->body.size()) {
                            Instruction& fInstr = frame.func->body[forwardPC];
                            if (fInstr.opcode == Opcode::BLOCK || fInstr.opcode == Opcode::LOOP) {
                                depth++;
                            } else if (fInstr.opcode == Opcode::END) {
                                depth--;
                                if (depth == 0) {
                                    frame.pc = forwardPC + 1;
                                    found = true;
                                    break;
                                }
                            }
                            forwardPC++;
                        }
                    }
                }

                if (!found) {
                     throw std::runtime_error("Label not found: " + label);
                }
            }
            break;
        }
        default:
            break;
    }
}

int Interpreter::resolveLocal(const std::string& id, Function* func) {
    if (isdigit(id[0])) return std::stoi(id);

    for(size_t i=0; i<func->paramNames.size(); ++i) {
        if (func->paramNames[i] == id) return i;
    }
    for(size_t i=0; i<func->localNames.size(); ++i) {
        if (func->localNames[i] == id) return func->paramNames.size() + i;
    }
    throw std::runtime_error("Unknown local: " + id);
}

Type* Interpreter::resolveType(const std::string& alias) {
    for (auto& t : module.types) {
        if (t.alias == alias) return &t;
    }
    return nullptr;
}
