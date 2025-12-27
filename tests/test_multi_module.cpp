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

WasmValue host_make_span(MemoryStore* store, std::vector<WasmValue>& args) {
    int32_t handle = args[0].i32;
    int32_t offset = args[1].i32;
    int32_t size = args[2].i32;
    int32_t new_handle = store->make_span(handle, offset, size);
    return WasmValue(new_handle);
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
        // Updated for Point3D: [z (i32)] [Point2D (2*i32=8)]
        // z is at offset 0. Point2D starts at offset 4.
        std::string libCode = R"(
            (module
                (import "env" "alloc" (func $env.alloc (param i32) (result i32)))
                (import "env" "make_span" (func $env.make_span (param i32 i32 i32) (result i32)))
                (import "env" "write_i32" (func $env.write_i32 (param i32 i32 i32)))
                (import "env" "read_i32" (func $env.read_i32 (param i32 i32) (result i32)))

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

                (func $point2d_add (param $p1 i32) (param $p2 i32)
                    (local $x i32)
                    (local $y i32)
                    (local.set $x (i32.add (call $point2d_get_x (local.get $p1)) (call $point2d_get_x (local.get $p2))))
                    (local.set $y (i32.add (call $point2d_get_y (local.get $p1)) (call $point2d_get_y (local.get $p2))))
                    (call $point2d_init (local.get $p1) (local.get $x) (local.get $y))
                )

                ;; Helper to get Point2D handle from Point3D handle
                (func $point3d_as_point2d (param $p i32) (result i32)
                    (call $env.make_span (local.get $p) (i32.const 4) (i32.const 8))
                )

                (func $point3d_new (param $x i32) (param $y i32) (param $z i32) (result i32)
                    (local $h i32)
                    (local $p2d i32)
                    (local.set $h (call $env.alloc (i32.const 12)))

                    ;; Write z at offset 0
                    (call $env.write_i32 (local.get $h) (i32.const 0) (local.get $z))

                    ;; Get slice for Point2D
                    (local.set $p2d (call $point3d_as_point2d (local.get $h)))

                    ;; Initialize Point2D part
                    (call $point2d_init (local.get $p2d) (local.get $x) (local.get $y))

                    (local.get $h)
                )

                (func $point3d_get_z (param $p i32) (result i32)
                    (call $env.read_i32 (local.get $p) (i32.const 0))
                )

                (func $point3d_add (param $p1 i32) (param $p2 i32)
                    (local $z i32)
                    (local $p1_2d i32)
                    (local $p2_2d i32)

                    ;; Extract Point2D handles
                    (local.set $p1_2d (call $point3d_as_point2d (local.get $p1)))
                    (local.set $p2_2d (call $point3d_as_point2d (local.get $p2)))

                    ;; Reuse point2d_add for x and y updates on p1
                    (call $point2d_add (local.get $p1_2d) (local.get $p2_2d))

                    ;; Update z
                    (local.set $z (i32.add (call $point3d_get_z (local.get $p1)) (call $point3d_get_z (local.get $p2))))
                    (call $env.write_i32 (local.get $p1) (i32.const 0) (local.get $z))
                )
            )
        )";

        // 3. Define Main Module Code
        // Uses the library to do 3D math
        std::string mainCode = R"(
            (module
                (import "lib" "point3d_new" (func $lib.point3d_new (param i32 i32 i32) (result i32)))
                (import "lib" "point3d_add" (func $lib.point3d_add (param i32 i32)))
                (import "lib" "point3d_as_point2d" (func $lib.point3d_as_point2d (param i32) (result i32)))
                (import "lib" "point2d_get_x" (func $lib.point2d_get_x (param i32) (result i32)))
                (import "lib" "point2d_get_y" (func $lib.point2d_get_y (param i32) (result i32)))
                (import "lib" "point3d_get_z" (func $lib.point3d_get_z (param i32) (result i32)))

                (func $run (result i32)
                    (local $p1 i32)
                    (local $p2 i32)
                    (local $p1_2d i32)

                    ;; p1 = point3d_new(10, 20, 30)
                    (local.set $p1 (call $lib.point3d_new (i32.const 10) (i32.const 20) (i32.const 30)))
                    ;; p2 = point3d_new(1, 2, 3)
                    (local.set $p2 (call $lib.point3d_new (i32.const 1) (i32.const 2) (i32.const 3)))
                    ;; point3d_add(p1, p2) -> p1 is modified
                    (call $lib.point3d_add (local.get $p1) (local.get $p2))

                    ;; Need to get p1 as point2d to read x and y
                    (local.set $p1_2d (call $lib.point3d_as_point2d (local.get $p1)))

                    ;; result = p1.x + p1.y + p1.z
                    (i32.add
                        (i32.add (call $lib.point2d_get_x (local.get $p1_2d)) (call $lib.point2d_get_y (local.get $p1_2d)))
                        (call $lib.point3d_get_z (local.get $p1))
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
        vm_lib.registerHostFunction("env", "alloc", std::bind(host_alloc, &store, _1), {"i32"}, {"i32"});
        vm_lib.registerHostFunction("env", "make_span", std::bind(host_make_span, &store, _1), {"i32", "i32", "i32"}, {"i32"});
        vm_lib.registerHostFunction("env", "write_i32", std::bind(host_write_i32, &store, _1), {"i32", "i32", "i32"}, {});
        vm_lib.registerHostFunction("env", "read_i32", std::bind(host_read_i32, &store, _1), {"i32", "i32"}, {"i32"});

        // 7. Register "Library" Functions for Main
        // Point2D functions
        // Note: Main only imports what it needs.
        // But we must register them.

        // Point3D functions
        vm_main.registerHostFunction("lib", "point3d_new",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point3d_new", args); }, {"i32", "i32", "i32"}, {"i32"});
        vm_main.registerHostFunction("lib", "point3d_add",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point3d_add", args); }, {"i32", "i32"}, {});
        vm_main.registerHostFunction("lib", "point3d_get_z",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point3d_get_z", args); }, {"i32"}, {"i32"});
        vm_main.registerHostFunction("lib", "point3d_as_point2d",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point3d_as_point2d", args); }, {"i32"}, {"i32"});

        // Helper accessors for point2d (x, y) which we invoke on the span
        vm_main.registerHostFunction("lib", "point2d_get_x",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point2d_get_x", args); }, {"i32"}, {"i32"});
        vm_main.registerHostFunction("lib", "point2d_get_y",
            [&](std::vector<WasmValue>& args) { return vm_lib.run("point2d_get_y", args); }, {"i32"}, {"i32"});


        // 8. Execute Main
        std::cout << "Running Multi-Module Test..." << std::endl;
        WasmValue res = vm_main.run("run", {});

        std::cout << "Result: " << res.i32 << std::endl;

        // Verify
        // p1 = (10, 20, 30)
        // p2 = (1, 2, 3)
        // p1 after add = (11, 22, 33)
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
