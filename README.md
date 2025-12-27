# Optrich VM 01

A custom WebAssembly-like runtime capable of running multi-module interactions with a shared memory store.

## Overview

Optrich VM is a lightweight, dependency-free C++ implementation of a WebAssembly interpreter. It is designed to understand the core concepts of Wasm execution, including:

*   **S-expression Parsing:** Reads standard WAT (WebAssembly Text) format.
*   **Stack-based Interpreter:** Executes instructions using a value stack and call stack.
*   **Host Functions:** Allows binding C++ functions to Wasm imports.
*   **Memory Store:** A custom object-based memory model (instead of linear memory) allowing typed access to allocated objects.

## Features

*   **Types:** i32, i64, f32, f64.
*   **Instructions:** Basic arithmetic (add, sub, mul, div), constants, control flow (call, return), and variable access (local.get, local.set).
*   **Structure:** Modules, Functions, Parameters, Locals, Results.
*   **Interoperability:** Register C++ functions to be called from Wasm.

## Architecture

The project is split into header files (`include/`) and source files (`src/`):

*   **`MemoryStore`:** Manages memory allocations. Unlike standard Wasm linear memory, this uses a handle-based system where `alloc` returns a handle ID, and `read/write` take (handle, offset).
*   **`Parser`:** recursive descent parser for WAT S-expressions.
*   **`Interpreter`:** The execution engine. It maintains the stack and executes opcodes.
*   **`AST`:** Definitions for Module, Function, Instruction, etc.
*   **`Lexer`:** Tokenizes the input string.

## Building and Running

### Prerequisites

*   C++17 compliant compiler (GCC/Clang)
*   Make

### Build

```bash
make
```

### Run Tests

You can run all tests using the provided script:

```bash
./run_tests.sh
```

## Example

```cpp
MemoryStore store;
std::string code = R"(
    (module
        (func $add (param $a i32) (param $b i32) (result i32)
            (i32.add (local.get $a) (local.get $b))
        )
    )
)";

Lexer lexer(code);
Module mod = Parser(lexer.tokenize()).parse();
Interpreter vm(mod, store);

WasmValue res = vm.run("$add", {WasmValue(10), WasmValue(20)});
std::cout << res.i32 << std::endl; // Output: 30
```
