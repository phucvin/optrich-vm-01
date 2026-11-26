# Garbage Collection Options

A core design question for adding GC to Optrich VM's handle/span memory model is: **because blocks are untyped raw byte buffers, how does the GC know which bytes inside a block are handles vs. ordinary numbers or text?** (For example, `testdata/main_polymorphism.wat` stores a handle inside another block using plain `write_i32`.)

This document outlines the main GC approaches for this model, span-specific considerations, and a deeper look at **Split Blocks** as a practical fit for Optrich VM.

---

## GC Approaches

### 1. Conservative Mark-and-Sweep (Boehm-style)
* **How it works:** Scan the value stack, locals, and every aligned 32-bit word of every allocated block. Any word matching a live handle ID marks that block (and for spans, its backing block). Sweep unmarked blocks.
* **Pros:** Requires no changes to the current untyped byte model or `i32` handles; simple to prototype.
* **Cons:**
  * **False retention:** Ordinary integers (`1`, `2`, `3`) can collide with valid handle IDs and keep blocks alive indefinitely (mitigated, but not eliminated, by tagging/scrambling handle IDs).
  * Not capability-safe on its own since handles remain forgeable `i32` values.

### 2. Precise Tracing GC
To achieve exact collection and unforgeable handles while retaining low-level byte layouts, the VM must distinguish handles from raw data:

* **2a. Split Blocks — `alloc(num_bytes, num_refs)`** *(detailed below)*
  * Every block has a raw byte region (never scanned by GC) and a separate handle-slot array (always scanned by GC).
* **2b. Per-Block Pointer Bitmap — `alloc_with_layout(size, ptr_mask)`**
  * Allocation specifies a bitmask marking which 4-byte/8-byte offsets hold handles (similar to Go's GC heap bitmaps). `read_handle`/`write_handle` only operate on handle-marked offsets.
  * Preserves arbitrary interleaved struct layouts while remaining 100% precise.
* **2c. CHERI-style 1-Bit Side Tags per Word**
  * Each 4-byte or 8-byte word has a 1-bit metadata tag (`is_handle`). `write_handle` sets the tag; raw byte/int writes clear it; `read_handle` traps if unset.
  * Keeps 100% untyped byte-buffer flexibility with no upfront layout declarations, exact GC, and unforgeable capabilities—at the cost of ~1.5–3% tag memory overhead and a tag update on writes.

### 3. Reference Counting (RC)
* **How it works:** Each backing block carries a `refcount`. Spans increment the backing block's `refcount` on `make_span` and decrement it when released.
* **Pros:** Deterministic, immediate reclamation with no stop-the-world GC pauses.
* **Cons:** Still requires handle precision (2a/2b/2c) to update counts on overwrite/frame exit; cannot reclaim cycles without a backup cycle collector; refcount traffic adds overhead in tight loops unless optimized with borrow scopes or deferred RC.

### 4. Arenas / Regions (Zero-GC or Hybrid)
* **How it works:** Allocations belong to an `Arena` (e.g., per-request or per-call scope) and are freed together in $O(1)$ (similar to Java Panama's `Arena`).
* **Pros:** Fastest allocation/reclamation, zero scanning overhead, and ideal for JIT check hoisting.
* **Cons:** Does not handle arbitrary long-lived object graphs on its own—best combined with a tracing GC for long-lived allocations.

---

## `make_span` Considerations (All GC Models)

1. **Retaining the backing block:** A span must reference its **root backing block** (`rootHandle` + `offset` + `size`), not just a raw `uint8_t*`. Dropping the original handle while a span remains reachable must keep the backing storage alive. Storing `rootHandle` also enables a **moving/compacting GC** later by updating `objects[root].ptr`.
2. **Large-buffer retention:** Holding a tiny 5-byte span into a 100 MB buffer keeps the entire 100 MB allocation alive (the same tradeoff as Go slices). Providing an explicit `clone_span` / copy helper lets callers detach small views when needed.

---

## Deep Dive: Option 2a — Split Blocks (`alloc(num_bytes, num_refs)`)

In **Split Blocks**, every allocated object has two disjoint regions:
1. **Raw bytes (`bytes`)** — for `i32`, `f64`, strings, flags, and numeric arrays. **Never scanned by GC.**
2. **Handle slots (`refs`)** — for references to other blocks/spans. **Always scanned by GC.**

Handles become a distinct `WasmValue::HANDLE` type on the VM stack (separate from `I32`), so integer arithmetic can never forge or corrupt a handle.

### Data Structure
```cpp
struct MemoryBlock {
    // Backing storage (empty if this block is a span)
    std::vector<uint8_t> bytes;
    std::vector<Handle>  refs;

    // Span view info (if rootHandle != 0, this is a span into rootHandle)
    Handle rootHandle = 0;
    size_t byteOffset = 0;
    size_t byteSize   = 0;
    size_t refOffset  = 0;
    size_t refCount   = 0;

    bool readOnly = false;
    bool marked   = false;
    bool alive    = true;
};
```
*(In a production runtime, `refs` and `bytes` can be packed back-to-back into a single contiguous allocation `[refs... | bytes...]` for cache locality and a single heap allocation per object.)*

### VM Operations
* `alloc(num_bytes, num_refs) -> handle`
  * Plain byte buffer or string: `alloc(N, 0)`
  * Array of object handles: `alloc(0, M)`
  * Struct with primitives + references: `alloc(N, M)`
* `read_i32(h, byte_off)` / `write_i32(h, byte_off, val)` — operates on `bytes` only.
* `read_ref(h, slot_idx) -> handle` / `write_ref(h, slot_idx, child_h)` — operates on `refs` only.
* `make_span(h, byte_off, byte_len)` (and optionally `make_ref_span(h, ref_off, ref_len)`) — creates a zero-copy view inheriting `readOnly` and referencing `rootHandle`.

### Example (`main_polymorphism.wat`)
Instead of a 12-byte raw buffer storing `[num_methods, method_idx, data_handle]` via `write_i32`, an interface becomes `alloc(8, 1)` (8 raw bytes + 1 handle slot):

```wat
;; 8 bytes for [num_methods, method_idx], 1 ref slot for [data]
(local.set $car_iface (call $alloc (i32.const 8) (i32.const 1)))
(call $write_i32 (local.get $car_iface) (i32.const 0) (i32.const 1)) ;; num_methods
(call $write_i32 (local.get $car_iface) (i32.const 4) (i32.const 1)) ;; method_idx
(call $write_ref (local.get $car_iface) (i32.const 0) (local.get $car_inst)) ;; data handle
```

### Mark-and-Sweep Algorithm
1. **Roots:** Collect every `WasmValue::HANDLE` from `valueStack`, active `StackFrame::locals`, and module `stringHandles` into a worklist.
2. **Mark:**
   ```cpp
   while (!worklist.empty()) {
       Handle h = worklist.back(); worklist.pop_back();
       if (h == 0 || objects[h].marked) continue;
       MemoryBlock& blk = objects[h];
       blk.marked = true;

       if (blk.rootHandle != 0) {
           // Span: keep backing block alive
           worklist.push_back(blk.rootHandle);
       } else {
           // Owning block: scan ONLY refs; skip bytes completely
           for (Handle child : blk.refs) {
               if (child != 0) worklist.push_back(child);
           }
       }
   }
   ```
3. **Sweep:** Iterate `objects[1..]`. For any `alive && !marked` block, clear `bytes`/`refs`, mark `alive = false`, and push its handle ID onto a `freeList` for reuse. Reset `marked = false` on surviving blocks.

### Strengths & Tradeoffs
* **Strengths:**
  * 100% precise GC and unforgeable capability handles without per-word tag bitmaps or type descriptors.
  * $O(1)$ GC scanning for raw data buffers (`alloc(N, 0)` has `refs.empty()`, so GC never touches the payload).
  * Straightforward compiler lowering: primitive fields map to byte offsets, reference fields map to ref slot indices.
* **Tradeoffs:**
  * Structs containing handles cannot be shallow-copied with a raw byte `memcpy` alone (requires copying both byte and ref regions via a VM helper).
  * Codegen tracks two offset spaces per struct (byte offsets vs. ref indices).
  * For spans over `refs`, marking `rootHandle` is simplest (keeps the whole parent alive), whereas tracing only the sliced ref range avoids retaining unrelated siblings at the cost of slightly more bookkeeping.
