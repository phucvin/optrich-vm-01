#include <iostream>
#include <cmath> // abs
#include "Parser.h"
#include "Interpreter.h"
#include "MemoryStore.h"

// Re-use host functions or simple version
WasmValue h_alloc(MemoryStore* store, std::vector<WasmValue>& args) {
    return WasmValue(store->alloc(args[0].i32));
}
WasmValue h_write_f64(MemoryStore* store, std::vector<WasmValue>& args) {
    store->write<double>(args[0].i32, args[1].i32, args[2].f64);
    return WasmValue();
}
WasmValue h_read_f64(MemoryStore* store, std::vector<WasmValue>& args) {
    return WasmValue(store->read<double>(args[0].i32, args[1].i32));
}

int main() {
    MemoryStore store;

    // Array of 2 doubles (16 bytes)
    // index 0 at offset 0, index 1 at offset 8
    std::string code = R"(
        (module
            (import "env" "alloc" (func $env.alloc (param i32) (result i32)))
            (import "env" "write_f64" (func $env.write_f64 (param i32 i32 f64)))
            (import "env" "read_f64" (func $env.read_f64 (param i32 i32) (result f64)))

            (func $test_array (result f64)
                (local $arr i32)
                (local.set $arr (call $env.alloc (i32.const 16)))

                ;; arr[0] = 1.1
                (call $env.write_f64 (local.get $arr) (i32.const 0) (f64.const 1.1))
                ;; arr[1] = 2.2
                (call $env.write_f64 (local.get $arr) (i32.const 8) (f64.const 2.2))

                ;; return arr[0] + arr[1]
                (f64.add
                    (call $env.read_f64 (local.get $arr) (i32.const 0))
                    (call $env.read_f64 (local.get $arr) (i32.const 8))
                )
            )
        )
    )";

    // I need to implement f64.add in Interpreter!
    // I only did i32 ops.
    // I better check Interpreter.hpp to add F64 support for this test.

    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    Module mod = parser.parse();

    Interpreter vm(mod, store);

    using namespace std::placeholders;
    vm.registerHostFunction("env", "alloc", std::bind(h_alloc, &store, _1), {"i32"}, {"i32"});
    vm.registerHostFunction("env", "write_f64", std::bind(h_write_f64, &store, _1), {"i32", "i32", "f64"}, {});
    vm.registerHostFunction("env", "read_f64", std::bind(h_read_f64, &store, _1), {"i32", "i32"}, {"f64"});

    try {
        WasmValue res = vm.run("test_array", {});
        std::cout << "Result: " << res.f64 << std::endl;
        if (std::abs(res.f64 - 3.3) < 0.0001) return 0;
        else return 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
