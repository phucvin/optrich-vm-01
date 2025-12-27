#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <list>
#include <sstream>

#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"
#include "MemoryStore.h"

namespace fs = std::filesystem;

// Host functions
WasmValue host_alloc(MemoryStore* store, std::vector<WasmValue>& args) {
    return WasmValue(store->alloc(args[0].i32));
}

WasmValue host_make_span(MemoryStore* store, std::vector<WasmValue>& args) {
    return WasmValue(store->make_span(args[0].i32, args[1].i32, args[2].i32));
}

WasmValue host_write_i32(MemoryStore* store, std::vector<WasmValue>& args) {
    store->write<int32_t>(args[0].i32, args[1].i32, args[2].i32);
    return WasmValue();
}

WasmValue host_read_i32(MemoryStore* store, std::vector<WasmValue>& args) {
    return WasmValue(store->read<int32_t>(args[0].i32, args[1].i32));
}

std::string readFile(const std::string& path) {
    std::ifstream t(path);
    if (!t.is_open()) throw std::runtime_error("Could not open file: " + path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

void registerStandardHostFunctions(Interpreter& vm, MemoryStore& store) {
    using namespace std::placeholders;
    vm.registerHostFunction("env", "alloc", std::bind(host_alloc, &store, _1), {"i32"}, {"i32"});
    vm.registerHostFunction("env", "make_span", std::bind(host_make_span, &store, _1), {"i32", "i32", "i32"}, {"i32"});
    vm.registerHostFunction("env", "write_i32", std::bind(host_write_i32, &store, _1), {"i32", "i32", "i32"}, {});
    vm.registerHostFunction("env", "read_i32", std::bind(host_read_i32, &store, _1), {"i32", "i32"}, {"i32"});
}

void runTest(const fs::path& mainPath) {
    try {
        MemoryStore store;
        // Keep modules alive!
        std::list<Module> moduleStore;
        std::map<std::string, std::unique_ptr<Interpreter>> interpreters;

        // 1. Load Main Module
        std::string mainCode = readFile(mainPath.string());
        Lexer mainLexer(mainCode);
        moduleStore.push_back(Parser(mainLexer.tokenize()).parse());
        Module& mainMod = moduleStore.back();

        // 2. Identify and Load Dependencies
        // We look at imports to find "lib_*.wat"
        for (const auto& imp : mainMod.imports) {
            if (imp.module == "env") continue; // Skip standard env

            // Check if we already loaded it
            if (interpreters.find(imp.module) != interpreters.end()) continue;

            // Look for testdata/lib_<module>.wat
            fs::path libPath = mainPath.parent_path() / ("lib_" + imp.module + ".wat");
            if (!fs::exists(libPath)) {
                throw std::runtime_error("Missing library: " + libPath.string());
            }

            std::string libCode = readFile(libPath.string());
            Lexer libLexer(libCode);
            moduleStore.push_back(Parser(libLexer.tokenize()).parse());
            Module& libMod = moduleStore.back();

            auto libVM = std::make_unique<Interpreter>(libMod, store);
            registerStandardHostFunctions(*libVM, store);
            interpreters[imp.module] = std::move(libVM);
        }

        // 3. Setup Main Interpreter
        Interpreter mainVM(mainMod, store);
        registerStandardHostFunctions(mainVM, store);

        // 4. Link Libraries to Main
        // For every import in Main that isn't env, we find the function in the loaded module
        for (const auto& imp : mainMod.imports) {
            if (imp.module == "env") continue;

            if (interpreters.find(imp.module) == interpreters.end()) {
                 throw std::runtime_error("Module not loaded: " + imp.module);
            }

            Interpreter* libVM = interpreters[imp.module].get();
            // We assume the function name in the lib is the same as the field name
            // Parser already stripped '$'
            std::string funcName = imp.field;

            // Register as host function
            mainVM.registerHostFunction(imp.module, imp.field,
                [libVM, funcName](std::vector<WasmValue>& args) {
                    return libVM->run(funcName, args);
                }, imp.paramTypes, imp.resultTypes);
        }

        // 5. Run Main
        // Capture stdout? The test harness run_tests.sh captures stdout.
        // We just print to stdout.

        WasmValue res = mainVM.run("main", {});
        std::cout << "Result: " << res.i32 << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    // If arguments provided, run specific file. Otherwise scan directory?
    // User requirement: "scans WAT files in testdata and execute files starts with main_"

    fs::path testDir = "testdata";
    if (argc > 1) testDir = argv[1];

    if (!fs::exists(testDir)) {
        std::cerr << "Directory not found: " << testDir << std::endl;
        return 1;
    }

    // Collect main files
    std::vector<fs::path> mainFiles;
    for (const auto& entry : fs::directory_iterator(testDir)) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind("main_", 0) == 0 && filename.substr(filename.length()-4) == ".wat") {
            mainFiles.push_back(entry.path());
        }
    }

    if (mainFiles.empty()) {
        std::cout << "No main_*.wat files found in " << testDir << std::endl;
        return 0;
    }

    bool allPassed = true;
    for (const auto& path : mainFiles) {
        // We need to verify the output against expected_stdout.
        // Since we are running in a single process, redirecting cout/cerr per test is tricky.
        // Alternative: Run one by one?
        // Actually, the user requirement says: "output of executing a main file should be then diff with the corresponding main_*.expected_stdout and report if not match"

        std::cout << "Running test: " << path.filename() << std::endl;

        // Strategy: Redirect cout to a stringstream for the duration of the test.
        std::streambuf* oldCout = std::cout.rdbuf();
        std::stringstream capture;
        std::cout.rdbuf(capture.rdbuf());

        runTest(path);

        std::cout.rdbuf(oldCout); // Restore

        std::string actual = capture.str();

        // Read expected
        fs::path expectedPath = path;
        expectedPath.replace_extension(".expected_stdout");
        std::string expected = "";
        if (fs::exists(expectedPath)) {
            expected = readFile(expectedPath.string());
        } else {
            std::cerr << "Warning: No expected output file for " << path << std::endl;
        }

        // Trim comparison?
        // Simple string compare. Ensure we handle newline differences if needed.
        if (actual == expected) {
            std::cout << "[PASS] " << path.filename() << std::endl;
        } else {
            std::cout << "[FAIL] " << path.filename() << std::endl;
            std::cout << "Expected:\n" << expected << "Actual:\n" << actual << std::endl;
            allPassed = false;
        }
    }

    return allPassed ? 0 : 1;
}
