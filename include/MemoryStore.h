#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <variant>
#include <cstring> // for memcpy
#include <algorithm> // for std::fill

class MemoryStore {
public:
    // Handle 0 is usually null/invalid, but let's just use 0-based index.
    // If we want to detect null, we can start from 1. Let's start from 1.
    using Handle = int32_t;

    struct MemoryBlock {
        std::vector<uint8_t> storage; // Empty if it's a span/view
        uint8_t* ptr;
        size_t size;
        bool readOnly = false;
    };

    MemoryStore();

    Handle alloc(int32_t size);
    Handle alloc_readonly(const std::vector<uint8_t>& data);
    Handle make_span(Handle handle, int32_t offset, int32_t size);

    // Generic read/write helper
    template <typename T>
    T read(Handle handle, int32_t offset) {
        validate_access(handle, offset, sizeof(T));
        // Handle endianness? Assuming host is same as Wasm (Little Endian) for prototype.
        // x86/ARM are LE.
        T value;
        const uint8_t* src = objects[handle].ptr + offset;
        std::memcpy(&value, src, sizeof(T));
        return value;
    }

    template <typename T>
    void write(Handle handle, int32_t offset, T value) {
        validate_access(handle, offset, sizeof(T), true);
        uint8_t* dst = objects[handle].ptr + offset;
        std::memcpy(dst, &value, sizeof(T));
    }

private:
    std::vector<MemoryBlock> objects;

    void validate_access(Handle handle, int32_t offset, size_t size, bool forWrite = false);
};
