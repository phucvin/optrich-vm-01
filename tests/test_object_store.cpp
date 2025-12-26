#include <iostream>
#include <cstring>
#include "ObjectStore.hpp"

int main() {
    ObjectStore store;

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

    return 0;
}
