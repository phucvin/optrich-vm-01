#include "MemoryStore.h"

MemoryStore::MemoryStore() {
    // Reserve index 0 as null/invalid
    // We push an empty block with nullptr/size 0
    objects.push_back({{}, nullptr, 0, false});
}

MemoryStore::Handle MemoryStore::alloc(int32_t size) {
    if (size < 0) throw std::runtime_error("Negative allocation size");

    MemoryBlock block;
    block.storage.resize(size);
    // Initialize with zeros? Wasm memory is zero-initialized.
    std::fill(block.storage.begin(), block.storage.end(), 0);

    block.ptr = block.storage.data();
    block.size = static_cast<size_t>(size);
    block.readOnly = false;

    objects.push_back(std::move(block));
    return static_cast<Handle>(objects.size() - 1);
}

MemoryStore::Handle MemoryStore::alloc_readonly(const std::vector<uint8_t>& data) {
    MemoryBlock block;
    block.storage = data; // Copy data
    block.ptr = block.storage.data();
    block.size = data.size();
    block.readOnly = true;

    objects.push_back(std::move(block));
    return static_cast<Handle>(objects.size() - 1);
}

MemoryStore::Handle MemoryStore::make_span(Handle handle, int32_t offset, int32_t size) {
    if (handle <= 0 || static_cast<size_t>(handle) >= objects.size()) {
        throw std::runtime_error("Invalid object handle access");
    }

    MemoryBlock& original = objects[handle];

    // Bounds check for the span creation
    if (offset < 0 || size < 0) {
        throw std::runtime_error("Invalid offset or size for span");
    }
    if (static_cast<size_t>(offset) + static_cast<size_t>(size) > original.size) {
        throw std::runtime_error("Span creation out of bounds");
    }

    MemoryBlock span;
    // No storage allocation
    span.ptr = original.ptr + offset;
    span.size = static_cast<size_t>(size);
    span.readOnly = original.readOnly; // Inherit read-only status

    objects.push_back(std::move(span));
    return static_cast<Handle>(objects.size() - 1);
}

void MemoryStore::validate_access(Handle handle, int32_t offset, size_t size, bool forWrite) {
    if (handle <= 0 || static_cast<size_t>(handle) >= objects.size()) {
        throw std::runtime_error("Invalid object handle access");
    }
    MemoryBlock& block = objects[handle];
    if (forWrite && block.readOnly) {
        throw std::runtime_error("Write access to read-only memory denied");
    }
    if (offset < 0 || static_cast<size_t>(offset + size) > block.size) {
        throw std::runtime_error("Out of bounds object access");
    }
}
