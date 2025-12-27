#include "Interpreter.h"

Interpreter::Interpreter(Module& mod, MemoryStore& store) : module(mod), store(store) {
    // Build Symbol Tables
    for (size_t i = 0; i < module.functions.size(); ++i) {
        funcMap[module.functions[i].name] = i;
    }
}

void Interpreter::registerHostFunction(std::string name, HostFunction func, int arity) {
    hostFuncs[name] = {func, arity};
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
        default:
            break;
    }
}

int Interpreter::resolveLocal(const std::string& id, Function* func) {
    if (isdigit(id[0])) return std::stoi(id);

    // Check params
    for(size_t i=0; i<func->paramNames.size(); ++i) {
        if (func->paramNames[i] == id) return i;
    }
    // Check locals
    for(size_t i=0; i<func->localNames.size(); ++i) {
        if (func->localNames[i] == id) return func->paramNames.size() + i;
    }
    throw std::runtime_error("Unknown local: " + id);
}
