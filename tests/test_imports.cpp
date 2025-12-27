#include <iostream>
#include <vector>
#include <cassert>
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "MemoryStore.h"

// Simple host function: add two i32s
WasmValue hostAdd(std::vector<WasmValue>& args) {
    int32_t a = args[0].i32;
    int32_t b = args[1].i32;
    return WasmValue(a + b);
}

int main() {
    try {
        // 1. WAT with import
        std::string wat = R"(
(module
  (import "env" "add" (func $host_add (param i32 i32) (result i32)))
  (func $main (result i32)
    (call $host_add (i32.const 10) (i32.const 32))
  )
)
)";
        Lexer lexer(wat);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        Module module = parser.parse();

        // Check import parsed
        if (module.imports.size() != 1) {
            std::cerr << "FAIL: Import not parsed" << std::endl;
            return 1;
        }
        if (module.imports[0].module != "env" || module.imports[0].field != "add") {
             std::cerr << "FAIL: Import module/field mismatch" << std::endl;
             return 1;
        }

        MemoryStore store;
        Interpreter interp(module, store);

        // 2. Register Host Function
        interp.registerHostFunction("env", "add", hostAdd, {"i32", "i32"}, {"i32"});

        // 3. Run
        // Note: Lexer returns "$main" including $, so we call "$main".
        WasmValue res = interp.run("$main", {});

        if (res.i32 == 42) {
            std::cout << "SUCCESS: Import called correctly, result 42" << std::endl;
        } else {
            std::cerr << "FAIL: Result was " << res.i32 << ", expected 42" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
