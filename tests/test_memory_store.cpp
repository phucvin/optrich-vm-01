#include <iostream>
#include <cstring>
#include <vector>
#include <cassert>
#include "MemoryStore.h"

int main() {
    MemoryStore store;

    // Test alloc
    int32_t h = store.alloc(16);
    std::cout << "Allocated handle: " << h << std::endl;

    // Test write/read i32
    store.write<int32_t>(h, 0, 12345);
    int32_t val = store.read<int32_t>(h, 0);
    std::cout << "Read i32: " << val << std::endl;
    if (val != 12345) return 1;

    // Test write/read f64
    store.write<double>(h, 8, 3.14159);
    double val_f = store.read<double>(h, 8);
    std::cout << "Read f64: " << val_f << std::endl;
    if (val_f != 3.14159) return 1;

    // Test OOB
    try {
        store.read<int32_t>(h, 100);
        std::cerr << "Should have failed OOB" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Caught expected OOB: " << e.what() << std::endl;
    }

    // Test ReadOnly
    std::vector<uint8_t> data = {1, 2, 3, 4};
    auto h_ro = store.alloc_readonly(data);

    // Read should pass
    try {
        uint8_t v = store.read<uint8_t>(h_ro, 0);
        if (v == 1) std::cout << "Read RO passed\n";
    } catch (...) {
        std::cerr << "Read RO failed unexpected\n";
        return 1;
    }

    // Write should fail
    try {
        store.write<uint8_t>(h_ro, 0, 99);
        std::cerr << "Write RO should have failed\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected RO Write error: " << e.what() << "\n";
    }

    // Span from RO should inherit RO
    auto span = store.make_span(h_ro, 1, 2);
    try {
        store.write<uint8_t>(span, 0, 88);
        std::cerr << "Write RO Span should have failed\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected RO Span Write error\n";
    }

    return 0;
}
