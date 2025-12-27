#include <iostream>
#include <cstring>
#include "MemoryStore.h"

int main() {
    MemoryStore store;

    try {
        // 1. Alloc handle A (100 bytes)
        MemoryStore::Handle hA = store.alloc(100);
        std::cout << "Allocated handle A: " << hA << std::endl;

        // 2. Create span B (offset 10, size 20)
        MemoryStore::Handle hB = store.make_span(hA, 10, 20);
        std::cout << "Allocated span B: " << hB << std::endl;

        // 3. Write to A at offset 10 -> Read from B at offset 0
        store.write<int32_t>(hA, 10, 42);
        int32_t valB = store.read<int32_t>(hB, 0);
        std::cout << "Read from B (expected 42): " << valB << std::endl;
        if (valB != 42) {
            std::cerr << "FAILED: B did not see write to A" << std::endl;
            return 1;
        }

        // 4. Write to B at offset 5 -> Read from A at offset 15
        store.write<int32_t>(hB, 5, 99);
        int32_t valA = store.read<int32_t>(hA, 15);
        std::cout << "Read from A (expected 99): " << valA << std::endl;
        if (valA != 99) {
            std::cerr << "FAILED: A did not see write to B" << std::endl;
            return 1;
        }

        // 5. Test Chaining: Create span C from B (offset 4, size 8)
        // B starts at A+10. C starts at B+4 = A+14.
        MemoryStore::Handle hC = store.make_span(hB, 4, 8);
        std::cout << "Allocated chained span C: " << hC << std::endl;

        // Write to C at offset 0 (A+14).
        // Overlaps with previous write at A+15 (bytes 15,16,17,18 vs 14,15,16,17).
        // Let's write simple byte at C+1 (A+15)
        store.write<uint8_t>(hC, 1, 200);
        uint8_t valA_byte = store.read<uint8_t>(hA, 15);
        std::cout << "Read from A (expected 200): " << (int)valA_byte << std::endl;
        if (valA_byte != 200) {
            std::cerr << "FAILED: A did not see write to C" << std::endl;
            return 1;
        }

        // 6. Test Bounds Check - Span Creation
        try {
            store.make_span(hA, 90, 20); // 90+20 = 110 > 100
            std::cerr << "FAILED: Should have caught OOB span creation" << std::endl;
            return 1;
        } catch (const std::exception& e) {
            std::cout << "Caught expected OOB span creation: " << e.what() << std::endl;
        }

        // 7. Test Bounds Check - Access
        try {
            store.read<int32_t>(hB, 18); // 18+4 = 22 > 20
            std::cerr << "FAILED: Should have caught OOB access on span" << std::endl;
            return 1;
        } catch (const std::exception& e) {
            std::cout << "Caught expected OOB access on span: " << e.what() << std::endl;
        }

        std::cout << "SUCCESS" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
