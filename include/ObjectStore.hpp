#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <variant>
#include <cstring> // for memcpy

// Basic Wasm Values
// Note: In a real engine we might use a union or std::variant.
// For simplicity and standard compliance, we'll use a tagged union approach.
struct WasmValue {
    enum Type { I32, I64, F32, F64, VOID } type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };

    WasmValue() : type(VOID), i64(0) {}
    explicit WasmValue(int32_t v) : type(I32), i32(v) {}
    explicit WasmValue(int64_t v) : type(I64), i64(v) {}
    explicit WasmValue(float v) : type(F32), f32(v) {}
    explicit WasmValue(double v) : type(F64), f64(v) {}
};

class ObjectStore {
public:
    // Handle 0 is usually null/invalid, but let's just use 0-based index.
    // If we want to detect null, we can start from 1. Let's start from 1.
    using Handle = int32_t;

    ObjectStore() {
        // Reserve index 0 as null/invalid
        objects.emplace_back();
    }

    Handle alloc(int32_t size) {
        if (size < 0) throw std::runtime_error("Negative allocation size");
        objects.emplace_back(size);
        // Initialize with zeros? Wasm memory is zero-initialized.
        std::fill(objects.back().begin(), objects.back().end(), 0);
        return static_cast<Handle>(objects.size() - 1);
    }

    // Generic read/write helper
    template <typename T>
    T read(Handle handle, int32_t offset) {
        validate_access(handle, offset, sizeof(T));
        // Handle endianness? Assuming host is same as Wasm (Little Endian) for prototype.
        // x86/ARM are LE.
        T value;
        const uint8_t* src = objects[handle].data() + offset;
        std::memcpy(&value, src, sizeof(T));
        return value;
    }

    template <typename T>
    void write(Handle handle, int32_t offset, T value) {
        validate_access(handle, offset, sizeof(T));
        uint8_t* dst = objects[handle].data() + offset;
        std::memcpy(dst, &value, sizeof(T));
    }

private:
    std::vector<std::vector<uint8_t>> objects;

    void validate_access(Handle handle, int32_t offset, size_t size) {
        if (handle <= 0 || static_cast<size_t>(handle) >= objects.size()) {
            throw std::runtime_error("Invalid object handle access");
        }
        if (offset < 0 || static_cast<size_t>(offset + size) > objects[handle].size()) {
            throw std::runtime_error("Out of bounds object access");
        }
    }
};
