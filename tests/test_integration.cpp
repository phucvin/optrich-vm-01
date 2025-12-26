#include <iostream>
#include "Parser.hpp"
#include "Interpreter.hpp"
#include "ObjectStore.hpp"

// Demo Host Functions
WasmValue host_alloc(ObjectStore* store, std::vector<WasmValue>& args) {
    int32_t size = args[0].i32;
    int32_t handle = store->alloc(size);
    return WasmValue(handle);
}

WasmValue host_write_i32(ObjectStore* store, std::vector<WasmValue>& args) {
    int32_t handle = args[0].i32;
    int32_t offset = args[1].i32;
    int32_t val = args[2].i32;
    store->write<int32_t>(handle, offset, val);
    return WasmValue();
}

WasmValue host_read_i32(ObjectStore* store, std::vector<WasmValue>& args) {
    int32_t handle = args[0].i32;
    int32_t offset = args[1].i32;
    int32_t val = store->read<int32_t>(handle, offset);
    return WasmValue(val);
}

WasmValue print_i32(std::vector<WasmValue>& args) {
    std::cout << "HOST PRINT: " << args[0].i32 << std::endl;
    return WasmValue();
}

int main() {
    // 1. Setup Object Store
    ObjectStore store;

    // 2. Load WAT Code
    // This WAT allocates a struct of 8 bytes (handle 'h'), writes 42 to offset 0,
    // reads it back, and returns it.
    std::string code = R"(
        (module
            (func $main (result i32)
                (local $h i32)
                ;; h = alloc(8)
                (local.set $h (call $env.alloc (i32.const 8)))

                ;; write(h, 0, 42)
                (call $env.write_i32 (local.get $h) (i32.const 0) (i32.const 42))

                ;; read(h, 0)
                (call $env.read_i32 (local.get $h) (i32.const 0))
            )
        )
    )";

    // 3. Parse
    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    Module mod = parser.parse();

    // 4. Setup Interpreter
    Interpreter vm(mod, store);

    // 5. Register Host Functions
    using namespace std::placeholders;
    // Note: The parser preserves '$' in identifiers, so we register with it.
    vm.registerHostFunction("$env.alloc", std::bind(host_alloc, &store, _1), 1);
    vm.registerHostFunction("$env.write_i32", std::bind(host_write_i32, &store, _1), 3);
    vm.registerHostFunction("$env.read_i32", std::bind(host_read_i32, &store, _1), 2);

    // 6. Run
    try {
        WasmValue res = vm.run("$main", {});
        std::cout << "Result: " << res.i32 << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
