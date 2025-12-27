#include <iostream>
#include <functional>
#include "Parser.h"
#include "Interpreter.h"
#include "MemoryStore.h"

// Host Functions (copied from test_integration.cpp for self-containment)
WasmValue host_alloc(MemoryStore* store, std::vector<WasmValue>& args) {
    int32_t size = args[0].i32;
    int32_t handle = store->alloc(size);
    return WasmValue(handle);
}

WasmValue host_write_i32(MemoryStore* store, std::vector<WasmValue>& args) {
    int32_t handle = args[0].i32;
    int32_t offset = args[1].i32;
    int32_t val = args[2].i32;
    store->write<int32_t>(handle, offset, val);
    return WasmValue();
}

WasmValue host_read_i32(MemoryStore* store, std::vector<WasmValue>& args) {
    int32_t handle = args[0].i32;
    int32_t offset = args[1].i32;
    int32_t val = store->read<int32_t>(handle, offset);
    return WasmValue(val);
}

// Helper to bridge calls
WasmValue bridge_call(Interpreter* targetVM, std::string funcName, std::vector<WasmValue>& args) {
    // Forward the call to the other VM
    // In a real system, we might need to handle context switching, memory mapping, etc.
    // Here we just recursively call run().
    return targetVM->run(funcName, args);
}

int main() {
    try {
        // 1. Setup Shared Object Store
        MemoryStore store;

        // 2. Define Library Module Code
        // Provides Point2D logic: new_point, get_x, get_y, add_points
        std::string libCode = R"(
            (module
                (func $new_point (param $x i32) (param $y i32) (result i32)
                    (local $h i32)
                    (local.set $h (call $env.alloc (i32.const 8)))
                    (call $env.write_i32 (local.get $h) (i32.const 0) (local.get $x))
                    (call $env.write_i32 (local.get $h) (i32.const 4) (local.get $y))
                    (local.get $h)
                )

                (func $get_x (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 0))
                )

                (func $get_y (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 4))
                )

                (func $add_points (param $p1 i32) (param $p2 i32) (result i32)
                    (local $x i32)
                    (local $y i32)
                    ;; x = p1.x + p2.x
                    (local.set $x (i32.add (call $get_x (local.get $p1)) (call $get_x (local.get $p2))))
                    ;; y = p1.y + p2.y
                    (local.set $y (i32.add (call $get_y (local.get $p1)) (call $get_y (local.get $p2))))
                    (call $new_point (local.get $x) (local.get $y))
                )
            )
        )";

        // 3. Define Main Module Code
        // Uses the library to do math
        std::string mainCode = R"(
            (module
                (func $run (result i32)
                    (local $p1 i32)
                    (local $p2 i32)
                    (local $p3 i32)
                    ;; p1 = new_point(10, 20)
                    (local.set $p1 (call $lib.new_point (i32.const 10) (i32.const 20)))
                    ;; p2 = new_point(1, 2)
                    (local.set $p2 (call $lib.new_point (i32.const 1) (i32.const 2)))
                    ;; p3 = add_points(p1, p2)
                    (local.set $p3 (call $lib.add_points (local.get $p1) (local.get $p2)))

                    ;; result = p3.x + p3.y
                    (i32.add (call $lib.get_x (local.get $p3)) (call $lib.get_y (local.get $p3)))
                )
            )
        )";

        // 4. Parse Modules
        Lexer libLexer(libCode);
        Module libMod = Parser(libLexer.tokenize()).parse();

        Lexer mainLexer(mainCode);
        Module mainMod = Parser(mainLexer.tokenize()).parse();

        // 5. Setup Interpreters
        Interpreter vm_lib(libMod, store);
        Interpreter vm_main(mainMod, store);

        // 6. Register Host Functions for Library
        // The library needs access to the raw object store primitives
        using namespace std::placeholders;
        vm_lib.registerHostFunction("$env.alloc", std::bind(host_alloc, &store, _1), 1);
        vm_lib.registerHostFunction("$env.write_i32", std::bind(host_write_i32, &store, _1), 3);
        vm_lib.registerHostFunction("$env.read_i32", std::bind(host_read_i32, &store, _1), 2);

        // 7. Register "Library" Functions for Main
        // The main module calls the library functions as if they were host functions
        // We bind the bridge_call to the specific function name in the library VM

        // $lib.new_point(i32, i32) -> i32
        vm_main.registerHostFunction("$lib.new_point",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$new_point", args); }, 2);

        // $lib.add_points(i32, i32) -> i32
        vm_main.registerHostFunction("$lib.add_points",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$add_points", args); }, 2);

        // $lib.get_x(i32) -> i32
        vm_main.registerHostFunction("$lib.get_x",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$get_x", args); }, 1);

        // $lib.get_y(i32) -> i32
        vm_main.registerHostFunction("$lib.get_y",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$get_y", args); }, 1);


        // 8. Execute Main
        std::cout << "Running Multi-Module Test..." << std::endl;
        WasmValue res = vm_main.run("$run", {});

        std::cout << "Result: " << res.i32 << std::endl;

        // Verify
        // p1 = (10, 20)
        // p2 = (1, 2)
        // p3 = (11, 22)
        // sum = 11 + 22 = 33
        if (res.i32 == 33) {
            std::cout << "SUCCESS" << std::endl;
            return 0;
        } else {
            std::cerr << "FAILURE: Expected 33, got " << res.i32 << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }
}
