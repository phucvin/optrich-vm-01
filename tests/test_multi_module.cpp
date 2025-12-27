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
    return targetVM->run(funcName, args);
}

int main() {
    try {
        // 1. Setup Shared Object Store
        MemoryStore store;

        // 2. Define Library Module Code
        // Provides Point2D and Point3D logic
        std::string libCode = R"(
            (module
                (func $point2d_init (param $h i32) (param $x i32) (param $y i32)
                    (call $env.write_i32 (local.get $h) (i32.const 0) (local.get $x))
                    (call $env.write_i32 (local.get $h) (i32.const 4) (local.get $y))
                )

                (func $point2d_new (param $x i32) (param $y i32) (result i32)
                    (local $h i32)
                    (local.set $h (call $env.alloc (i32.const 8)))
                    (call $point2d_init (local.get $h) (local.get $x) (local.get $y))
                    (local.get $h)
                )

                (func $point2d_get_x (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 0))
                )

                (func $point2d_get_y (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 4))
                )

                (func $point2d_add (param $p1 i32) (param $p2 i32) (result i32)
                    (local $x i32)
                    (local $y i32)
                    (local.set $x (i32.add (call $point2d_get_x (local.get $p1)) (call $point2d_get_x (local.get $p2))))
                    (local.set $y (i32.add (call $point2d_get_y (local.get $p1)) (call $point2d_get_y (local.get $p2))))
                    (call $point2d_new (local.get $x) (local.get $y))
                )

                (func $point3d_new (param $x i32) (param $y i32) (param $z i32) (result i32)
                    (local $h i32)
                    (local.set $h (call $env.alloc (i32.const 12)))
                    ;; Reuse point2d initialization for x and y
                    (call $point2d_init (local.get $h) (local.get $x) (local.get $y))
                    (call $env.write_i32 (local.get $h) (i32.const 8) (local.get $z))
                    (local.get $h)
                )

                (func $point3d_get_z (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 8))
                )

                (func $point3d_add (param $p1 i32) (param $p2 i32) (result i32)
                    (local $x i32)
                    (local $y i32)
                    (local $z i32)
                    ;; Reuse point2d getters for x and y
                    (local.set $x (i32.add (call $point2d_get_x (local.get $p1)) (call $point2d_get_x (local.get $p2))))
                    (local.set $y (i32.add (call $point2d_get_y (local.get $p1)) (call $point2d_get_y (local.get $p2))))
                    (local.set $z (i32.add (call $point3d_get_z (local.get $p1)) (call $point3d_get_z (local.get $p2))))
                    (call $point3d_new (local.get $x) (local.get $y) (local.get $z))
                )
            )
        )";

        // 3. Define Main Module Code
        // Uses the library to do 3D math
        std::string mainCode = R"(
            (module
                (func $run (result i32)
                    (local $p1 i32)
                    (local $p2 i32)
                    (local $p3 i32)
                    ;; p1 = point3d_new(10, 20, 30)
                    (local.set $p1 (call $lib.point3d_new (i32.const 10) (i32.const 20) (i32.const 30)))
                    ;; p2 = point3d_new(1, 2, 3)
                    (local.set $p2 (call $lib.point3d_new (i32.const 1) (i32.const 2) (i32.const 3)))
                    ;; p3 = point3d_add(p1, p2)
                    (local.set $p3 (call $lib.point3d_add (local.get $p1) (local.get $p2)))

                    ;; result = p3.x + p3.y + p3.z
                    (i32.add
                        (i32.add (call $lib.point2d_get_x (local.get $p3)) (call $lib.point2d_get_y (local.get $p3)))
                        (call $lib.point3d_get_z (local.get $p3))
                    )
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
        using namespace std::placeholders;
        vm_lib.registerHostFunction("$env.alloc", std::bind(host_alloc, &store, _1), 1);
        vm_lib.registerHostFunction("$env.write_i32", std::bind(host_write_i32, &store, _1), 3);
        vm_lib.registerHostFunction("$env.read_i32", std::bind(host_read_i32, &store, _1), 2);

        // 7. Register "Library" Functions for Main
        // Point2D functions
        vm_main.registerHostFunction("$lib.point2d_new",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point2d_new", args); }, 2);
        vm_main.registerHostFunction("$lib.point2d_add",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point2d_add", args); }, 2);
        vm_main.registerHostFunction("$lib.point2d_get_x",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point2d_get_x", args); }, 1);
        vm_main.registerHostFunction("$lib.point2d_get_y",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point2d_get_y", args); }, 1);

        // Point3D functions
        vm_main.registerHostFunction("$lib.point3d_new",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point3d_new", args); }, 3);
        vm_main.registerHostFunction("$lib.point3d_add",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point3d_add", args); }, 2);
        vm_main.registerHostFunction("$lib.point3d_get_z",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("$point3d_get_z", args); }, 1);


        // 8. Execute Main
        std::cout << "Running Multi-Module Test..." << std::endl;
        WasmValue res = vm_main.run("$run", {});

        std::cout << "Result: " << res.i32 << std::endl;

        // Verify
        // p1 = (10, 20, 30)
        // p2 = (1, 2, 3)
        // p3 = (11, 22, 33)
        // sum = 11 + 22 + 33 = 66
        if (res.i32 == 66) {
            std::cout << "SUCCESS" << std::endl;
            return 0;
        } else {
            std::cerr << "FAILURE: Expected 66, got " << res.i32 << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }
}
