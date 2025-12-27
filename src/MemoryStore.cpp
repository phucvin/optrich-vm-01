#include "MemoryStore.h"

MemoryStore::MemoryStore() {
    // Reserve index 0 as null/invalid
    objects.emplace_back();
}

MemoryStore::Handle MemoryStore::alloc(int32_t size) {
    if (size < 0) throw std::runtime_error("Negative allocation size");
    objects.emplace_back(size);
    // Initialize with zeros? Wasm memory is zero-initialized.
    std::fill(objects.back().begin(), objects.back().end(), 0);
    return static_cast<Handle>(objects.size() - 1);
}

void MemoryStore::validate_access(Handle handle, int32_t offset, size_t size) {
    if (handle <= 0 || static_cast<size_t>(handle) >= objects.size()) {
        throw std::runtime_error("Invalid object handle access");
    }
    if (offset < 0 || static_cast<size_t>(offset + size) > objects[handle].size()) {
        throw std::runtime_error("Out of bounds object access");
    }
}
